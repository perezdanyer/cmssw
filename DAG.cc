#include <iostream>
#include <string>
#include <algorithm>
#include <map>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/info_parser.hpp>

#include "DAG.h"
#include "exceptions.h"

using namespace std;
namespace fs = boost::filesystem;
namespace pt = boost::property_tree; // used to read the config file

// `boost::optional` allows to safely initialiase an object
// example:
// ```
// optional<int> i = someFunction();
// if (i) cout << *i << endl;
// ```
// NB: std::optional exists in C++17 but there are issues when using Property Trees...

static const char * bold = "\e[1m", * normal = "\e[0m";

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// just a "translator" to get the IOVs from a space-separated string into a vector of integers
template<typename T> class Tokenise {

    T Cast (string s) const;

public:

    typedef string internal_type; // mandatory to define translator
    typedef vector<T> external_type; // id.

    boost::optional<external_type> get_value (internal_type s) const // method name is mandatory for translator
    {
        // pre-treatment :
        // - replace all '\n' by ' '
        // - remove spaces in front or at the end
        replace(s.begin(), s.end(), '\n', ' ');
        boost::trim(s);

        // tokenise
        boost::char_separator<char> sep{" "};
        boost::tokenizer<boost::char_separator<char>> tok{s,sep};
        // note: multiple spaces should (in principle...) be considered as a single space by boost...

        // put into a vector
        external_type results; 
        for (string t: tok) {
            T tt = boost::lexical_cast<T>(t);
            results.push_back(tt);
        }
        return results;
    }
};
#endif


namespace AllInOneConfig {

static const Tokenise<int   > tok_int;
static const Tokenise<string> tok_str;

////////////////////////////////////////////////////////////////////////////////
/// struct Job
///
/// Structure used to reproduce content for DAGMAN
///
/// Example of DAGMAN:
/// ```
/// JOB jobname1 condor.sub
/// VARS jobname1 key=value1
///
/// JOB jobname2 condor.sub
/// VARS jobname2 key=value2
///
/// JOB merge merge.sub
/// VARS merge otherkey=value3
///
/// PARENT jobname1 jobname2 CHILD merge
/// ```
struct Job {

    const string name, //!< job name as see by HTC
                 exec; //!< executable

    const fs::path dir, //!< directory to place the configs for the job
                   condor; //!< path to condor config
    pt::ptree tree; //!< local config (should NOT be a reference)

    vector<string> parents; //!< parents for which current job has to weight

    Job (string Name, string Exec, fs::path d, fs::path c) :
        name(Name), exec(Exec), dir(d), condor(c)
    {
        cout << "Declaring job " << name << " to be run with " << exec << endl;
    }
};

////////////////////////////////////////////////////////////////////////////////
/// Overloaded operator<<
///
/// Example:
/// ```
/// Job single1(...), single2(...);
/// Job merge(...);
/// merge.parents = {&single1, &single2};
/// dag << single1 << single2 << merge;
/// ```
ostream& operator<< (ostream& dag, const Job& job)
{
    cout << "Queuing job " << job.name << endl;

    fs::path newcondor = job.dir / "condor.sub";
    dag << "JOB " << job.name << ' ' << fs::absolute(newcondor).string() << '\n'
        << "VARS " << job.name << " exec=" << job.exec << '\n';

    fs::path newconfig = job.dir / "config.info";
    pt::write_info(newconfig.c_str(), job.tree);

    fs::copy(job.condor, newcondor);

    if (job.parents.size() > 0)
        dag << accumulate(job.parents.begin(), job.parents.end(), string("PARENT"),
               [](string instruction, string parent) { return instruction + ' ' + parent; })
            << " CHILD " << job.name << '\n';

    dag << '\n';

    return dag;
}

vector<int> DAG::GetIOVsFromTree (const pt::ptree& tree)
{
    if (!tree.count("IOV")) return {1};

    vector<int> IOVs = tree.get<vector<int>>("IOV", tok_int);
    if (!is_sorted(IOVs.begin(), IOVs.end())) throw ConfigError("IOVs are not sorted");

    return IOVs;
}

////////////////////////////////////////////////////////////////////////////////
/// function DAG::DAG
///
/// Constructor, making basic checks and opening file to write DAGMAN
DAG::DAG
    (string file) //!< name of the INFO config file
{
    cout << "Starting validation" << endl;

    cout << "Reading the config " << file << endl;
    pt::ptree tree;
    pt::read_info(file, tree);

    dir = tree.get<string>("name");
    LFS = tree.get<string>("LFS");
    condor = tree.get<string>("condor");

    alignments = tree.get_child("alignments");

    if (!tree.count("validations")) {
        //throw pt::ptree_bad_path("No validation found", "validations"); // TODO
        throw ConfigError("No validation found");
        exit(EXIT_FAILURE);
    }

    ptGCP = tree.get_child_optional("validations.GCP");
    ptDMR = tree.get_child_optional("validations.DMR");

    if (!ptGCP && !ptDMR) {
        cerr << "No known validation found!\n";
        exit(EXIT_FAILURE);
    }

    if (fs::exists(dir))  {
        cerr << dir << " already exists\n";
        exit(EXIT_FAILURE);
    }
    cout << "Creating directory " << dir << endl;
    fs::create_directory(dir);

    cout << "Adding .gitignore" << endl;
    fs::path pi = dir / ".gitignore";
    ofstream gitignore;
    gitignore.open(pi.c_str());
    gitignore << "*\n";
    gitignore.close();

    cout << "Copy INFO file as backup" << endl;
    fs::copy(file, dir / "config.info");

    cout << "Opening dag file" << endl;
    fs::path p = dir / "dag";
    dag.open(p.c_str());
}

////////////////////////////////////////////////////////////////////////////////
/// function DAG::GCP
///
/// loops over the different blocks for the different GCPs 
/// and loops over IOVs as well
void DAG::GCP ()
{
    if (!ptGCP) {
        cout << "No block for GCP found." << endl;
        return;
    }

    cout << "Generating GCP configuration files" << endl;
    for (pair<string,pt::ptree> it: *ptGCP) {

        string name = it.first;
        cout << bold << "GCP: " << name << normal << endl;

        vector<int> IOVs = GetIOVsFromTree(it.second);
        bool multiIOV = IOVs.size() > 1;

        if (multiIOV) cout << "Several IOVs were found";

        fs::path config = it.second.count("condor") ? it.second.get<string>("condor") : condor;

        for (int IOV: IOVs) {
            cout << "Preparing validation for IOV " << IOV << endl;
            it.second.put<int>("IOV", IOV);

            fs::path subdir = "GCP"; subdir /= name;
            if (multiIOV) subdir /= fs::path(to_string(IOV));
            cout << "The validation will be performed in " << subdir << endl;
            fs::create_directories(dir / subdir);

            string jobname = "GCP_" + name;
            if (multiIOV) jobname +=  '_' + to_string(IOV);

            Job job(jobname, "GCP", dir / subdir, config);

            string twig_ref  = it.second.get<string>("reference"),
                   twig_test = it.second.get<string>("test");

            fs::path output = LFS / subdir;
            fs::create_directories(output);

            job.tree.put<string>("output", output.string());
            job.tree.put_child("alignments." + twig_ref , alignments.get_child(twig_ref ));
            job.tree.put_child("alignments." + twig_test, alignments.get_child(twig_test));
            job.tree.put_child("validation", it.second);
            job.tree.put<int>("validation.IOV", IOV);

            dag << job;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/// method DMRsingle
///
/// For one given block of DMR.single,
/// creates one job for each geometry and each IOV
void DAG::DMRsingle (string name, pt::ptree& tree)
{
    cout << bold << "DMR single: " << name << normal << endl;

    vector<int> IOVs = GetIOVsFromTree(tree);
    bool multiIOV = IOVs.size() > 1;

    vector<string> geoms = tree.get<vector<string>>("alignments", tok_str);

    fs::path config = tree.count("condor") ? tree.get<string>("condor") : condor;

    // create the jobs
    for (string geom: geoms) {

        fs::path subdir = "DMR/single/" + name + '/' + geom;
        fs::path output = LFS / subdir;
        
        for (int IOV: IOVs) {

            // light files 
            fs::path subsubdir = subdir;
            if (multiIOV) subsubdir /= fs::path(to_string(IOV));
            cout << "The validation will be performed in " << subsubdir << endl;
            fs::create_directories(dir / subsubdir);

            // heavy files
            fs::path outputIOV = output;
            if (multiIOV) outputIOV /= fs::path(to_string(IOV));
            cout << "The heavy files will be stored in " << outputIOV << endl;
            fs::create_directories(outputIOV);

            // job
            string jobname = "DMRsingle_" + name + '_' + geom;
            if (multiIOV) jobname +=  '_' + to_string(IOV);
            // NB: the name should be changed with care, since it is used later on
            //     to define the hierarchy of jobs in the DAGMAN

            // local config
            Job job(jobname, "DMRsingle", dir / subsubdir, config);
            job.tree.put_child("alignment", alignments.get_child(geom));
            job.tree.put("alignment.output", outputIOV.string());
            job.tree.put_child("validation", tree);
            job.tree.erase("validation.alignments"); // TODO: check (should not be there, but it seems that it is not removed)
            job.tree.put("validation.IOV", IOV);

            // replace wildcard by IOV
            string dataset = job.tree.get<string>("validation.dataset");
            boost::replace_first(dataset, "*", to_string(IOV));
            job.tree.put("validation.dataset", dataset);

            // "submit"
            dag << job;
        }

        // write output path for merge job
        string key = geom + ".files.DMR." + name;
        string value = output.string() + (multiIOV ? "/*" : "");
        alignments.put<string>(key, value);
    }
}

void DAG::DMRmerge (string name, pt::ptree& tree)
{
    cout << bold << "DMR merge: " << name << normal << endl;

    cout << "Getting the single jobs from which the merge job depends:" << flush;
    vector<string> singles = tree.get<vector<string>>("singles", tok_str);
    // e.g. if one would like to compare Data vs. MC, then different datasets are necessary
    for (string single: singles) cout << ' ' << single;
    cout << endl;

    cout << "Getting the respective lists of IOVs (and sorting them by size)" << endl;
    vector<vector<int>> IOVss;
    for (string single: singles) {
        pt::ptree single_tree = ptDMR->get_child("single." + single);
        vector<int> IOVs = GetIOVsFromTree(single_tree);
        //vector<int> IOVs = tree.get<vector<int>>("validations.DMR.single." + single + ".IOV", tok_int);
        IOVss.push_back(IOVs);
    }
    sort(IOVss.begin(), IOVss.end(), [](auto a, auto b) { return a.size() > b.size(); });

    cout << "Checking the IOV lists" << endl;
    for (size_t i = IOVss.size()-1; i > 0; --i) {

        // first test: if there is only one IOV, just go with it
        vector<int>& test = IOVss.at(i);
        if (test.size() == 1) continue;

        // if the test is a subset of ref, then everything is fine as well
        vector<int>& ref = IOVss.at(i-1);
        if (all_of(test.begin(), test.end(), [&ref](int IOV) { return find(ref.begin(), ref.end(), IOV) != ref.end(); })) continue;

        // otherwise, complain
        throw ConfigError("Inconsistent IOV lists.");
    }

    vector<int>& IOVs = IOVss.front();
    bool multiIOV = IOVs.size() > 1;

    fs::path config = tree.count("condor") ? fs::path(tree.get<string>("condor")) : condor;

    for (int globalIOV: IOVs) {

        fs::path subdir = "DMR/merge/" + name;
        if (multiIOV) subdir /= fs::path(to_string(globalIOV));
        cout << "The validation will be performed in " << subdir << endl;
        fs::create_directories(dir / subdir);

        fs::path output = LFS / subdir;
        cout << "The heavy files will be stored in " << output << endl;
        fs::create_directories(output);

        string jobname = "DMRmerge_" + name;
        if (multiIOV) jobname +=  '_' + to_string(globalIOV);

        Job job(jobname, "DMRmerge", dir / subdir, config);
        job.tree.put_child("alignments", alignments);
        job.tree.put_child("validation", tree);
        job.tree.put<string>("validation.output", output.string());

        cout << "Looping over single and alignments:" << flush;
        for (string single: singles) {
            cout << ' ' << single;
            auto localIOVs = GetIOVsFromTree(ptDMR->get_child("single." + single));

            // getting "local" IOV, i.e. from the single job
            int localIOV = localIOVs.front();
            bool localMultiIOV = localIOVs.size() > 1;
            if (localMultiIOV) {
                auto it = localIOVs.rbegin();
                while (it != localIOVs.rend() && *it > globalIOV) ++it;
                localIOV = *it;
            }
            
            cout << " (";
            for (auto it: alignments) {
                string ali = it.first;
                cout << ali << ' ' << flush;

                string key = "files.DMR." + single;
                boost::optional<string> input = it.second.get_optional<string>(key);
                if (!input) continue;

                boost::replace_first(*input, "*", to_string(localIOV));
                cout << bold << __LINE__ << ' '  << input << normal << endl;
                job.tree.put<string>("alignments." + ali + "." + key, *input);

                string parentjob = "DMRsingle_" + single + '_' + ali;
                if (localMultiIOV) parentjob += '_' + to_string(localIOV);
                job.parents.push_back(parentjob);
            }
            cout << "\b)" << flush;;
        }
        cout << endl;

        dag << job;
    }

    // TODO: write output of merge job in alignments.*.files.DMR.merge ?
}

//void DAG::DMRtrend (string name, pt::ptree& tree)
//{
//    cout << bold << "DMR trend: " << name << normal << endl;
//    // TODO
//}

////////////////////////////////////////////////////////////////////////////////
/// function DAG::DMR
/// 
/// Configure "offline validation", measuring the local performance.
/// This public method delegates the configuration of the single, merge and trend
/// jobs to private methods.
void DAG::DMR ()
{
    if (!ptDMR) {
        cout << "No block for DMR found." << endl;
        return;
    }

    // TODO: make a loop since all three blocks are very similar?

    boost::optional<pt::ptree&> singles = ptDMR->get_child_optional("single");
    if (singles) {
        cout << "Generating DMR single configuration files" << endl;
        for (auto& it: *singles) {
            string name = it.first;
            auto subtree = it.second;
            DMRsingle(name, subtree);
        }
        // TODO: intermediate config files (can help debugging + provides templates for another run of validation)
        //fs::path output = dir / "DMR/single.info";
        //pt::write_info(output.c_str(), alignments);
    }

    boost::optional<pt::ptree&> merges = ptDMR->get_child_optional("merge");
    if (merges) {
        cout << "Generating DMR merge configuration files" << endl;
        for (auto& it: *merges) {
            string name = it.first;
            auto subtree = it.second;
            DMRmerge(name, subtree);
        }
    }

    //boost::optional<pt::ptree&> trends = ptDMR->get_child_optional("trend");
    //if (trends) {
    //    cout << "Generating DMR trend configuration files" << endl;
    //    for (auto& it: *trends) {
    //        string name = it.first;
    //        auto subtree = it.second;
    //        DMRtrend(name, subtree);
    //    }
    //}
}

////////////////////////////////////////////////////////////////////////////////
/// function DAG::close
///
/// Close the DAG man and submit (unless dry option is true)
void DAG::close ()
{
    cout << "Closing dag file" << endl;
    dag.close();
}

////////////////////////////////////////////////////////////////////////////////
/// function DAG::submit
/// 
/// Submit the DAGMAN
int DAG::submit () const
{
    cout << "Submitting DAGMAN" << endl;
    return system("echo condor_submit_dag "); // TODO
}

}
