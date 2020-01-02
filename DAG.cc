#include <iostream>
#include <string>
#include <algorithm>
#include <map>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

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

    const string name, exec;
    const fs::path dir;
    pt::ptree tree;

    vector<string> parents;

    Job (string Name, string Exec, fs::path d) :
        name(Name), exec(Exec), dir(d)
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

    dag << "JOB " << job.name << " condor.sub" << '\n'
        << "VARS " << job.name << " exec=" << job.exec << '\n';

    create_directories(job.dir);
    fs::path p = job.dir / "config.info";
    pt::write_info(p.c_str(), job.tree);
    // TODO: copy condor.sub? or generate it using some HTC class?

    if (job.parents.size() > 0)
        dag << accumulate(job.parents.begin(), job.parents.end(), string("PARENT"),
               [](string instruction, string parent) { return instruction + ' ' + parent; })
            << " CHILD " << job.name << '\n';

    return dag;
}

vector<int> DAG::GetIOVsFromTree (const pt::ptree& tree)
{
    if (!tree.count("IOV")) return {1};

    vector<int> IOVs = tree.get<vector<int>>("IOV", tok_int);
    if (!is_sorted(IOVs.begin(), IOVs.end())) throw ConfigError("IOVs are not sorted");

    return IOVs;
}

DAG::DAG (string file)
{
    cout << "Starting validation" << endl;

    cout << "Reading the config " << file << endl;
    pt::ptree main_tree;
    pt::read_info(file, main_tree);

    dir = main_tree.get<string>("name");
    LFS = main_tree.get<string>("LFS");

    alignments = main_tree.get_child("alignments");

    if (!main_tree.count("validations")) {
        //throw pt::ptree_bad_path("No validation found", "validations"); // TODO
        throw ConfigError("No validation found");
        exit(EXIT_FAILURE);
    }

    ptGCP = main_tree.get_child_optional("validations.GCP");
    ptDMR = main_tree.get_child_optional("validations.DMR");

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

        for (int IOV: IOVs) {
            cout << "Preparing validation for IOV " << IOV << endl;
            it.second.put<int>("IOV", IOV);

            fs::path subdir = "GCP"; subdir /= name;
            if (multiIOV) subdir /= fs::path(to_string(IOV));
            cout << "The validation will be performed in " << subdir << endl;

            string jobname = "GCP_" + name;
            if (multiIOV) jobname +=  '_' + to_string(IOV);

            Job job(jobname, "GCP", dir / subdir);

            string twig_ref  = it.second.get<string>("reference"),
                   twig_test = it.second.get<string>("test");

            fs::path output = LFS / subdir;
            fs::create_directories(output);

            job.tree.put<string>("output", output.string());
            job.tree.put_child("alignments." + twig_ref , alignments.get_child(twig_ref ));
            job.tree.put_child("alignments." + twig_test, alignments.get_child(twig_test));
            job.tree.put_child("validation", it.second);
            job.tree.put<int>("validation.IOV", IOV);

            cout << "Queuing job" << endl;
            dag << job;
        }
    }
}

/*pair<string, vector<int>>*/ void DAG::DMRsingle (string name, pt::ptree& tree)
{
    cout << bold << "DMR single: " << name << normal << endl;

    vector<int> IOVs = GetIOVsFromTree(tree);
    bool multiIOV = IOVs.size() > 1;

    vector<string> geoms = tree.get<vector<string>>("alignments", tok_str);

    //// check if there is any preexisting statement
    //for (const auto& geom = geoms.begin(); geom != geoms.end(); /* nothing */) {
    //    bool preexisting = alignments.get_child(*geom).count(name);
    //    if (preexisting) { // TODO: exceptions
    //        cerr << *a << " seems to be already ready (mentioned twice in the config)\n";
    //        exit(EXIT_FAILURE);
    //    }
    //}

    // then create the jobs
    for (int IOV: IOVs)
    for (string geom: geoms) {

        fs::path subdir = "DMR/single/" + name;
        if (multiIOV) subdir /= fs::path(to_string(IOV));
        cout << "The validation will be performed in " << subdir << endl;

        fs::path output = LFS / subdir;
        fs::create_directories(output);
        alignments.put<string>(geom + ".files.DMR." + name, output.string());

        string jobname = "DMRsingle_" + name;
        if (multiIOV) jobname +=  '_' + to_string(IOV);

        Job job(jobname, "DMR", dir / subdir);
        job.tree.put_child("alignment", alignments.get_child(geom));
        job.tree.put_child("validation", tree);
        job.tree.erase("validation.alignments");
        job.tree.put("validation.IOV", IOV);

        // replace wildcard by IOV
        string dataset = job.tree.get<string>("validation.dataset");
        boost::replace_first(dataset, "*", to_string(IOV));
        job.tree.put("validation.dataset", dataset);

        dag << job;
    }

    //return { name, IOVs };
}

void DAG::DMRmerge (string name, pt::ptree& tree)
{
    cout << bold << "DMR merge: " << name << normal << endl;

    // TODO: IOVs?
    // - one list should be a subset of the other...
}

//void DAG::DMRtrend ()
//{
//    // TODO
//}

void DAG::DMR ()
{
    if (!ptDMR) {
        cout << "No block for DMR found." << endl;
        return;
    }

    boost::optional<pt::ptree&> singles = ptDMR->get_child_optional("single"),
                                merges  = ptDMR->get_child_optional("merge"),
                                trends  = ptDMR->get_child_optional("trend");

    //map<int,vector<string>> singleJobs; // key = IOV, value = list of jobs

    if (singles) {
        cout << "Generating DMR single configuration files" << endl;
        for (auto& it: *singles) {
            string name = it.first;
            auto subtree = it.second;
            DMRsingle(name, subtree);
        }
    }

    if (merges) {
        cout << "Generating DMR merge configuration files" << endl;
        for (pair<string,pt::ptree> it: *merges) {
            string name = it.first;
            auto subtree = it.second;
            DMRmerge(name, subtree);
        }
    }

    if (trends) {
        // TODO
    }
}

void DAG::close ()
{
    cout << "Closing dag file" << endl;
    dag.close();
}

int DAG::submit () const
{
    cout << "Submitting" << endl;
    return system("echo condor_submit_dag "); // TODO
}

}
