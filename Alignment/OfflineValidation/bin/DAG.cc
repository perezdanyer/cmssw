#include <cstdlib>
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

    const string name; //!< job name as see by HTC

    const fs::path exec, //!< executable
                   dir, //!< directory to place the configs for the job
                   condor; //!< path to condor config
    pt::ptree tree; //!< local config (should NOT be a reference)

    vector<string> parents; //!< parents for which current job has to weight

    Job (string Name, fs::path Exec, fs::path d, fs::path c) :
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

    fs::path newexec = job.exec.filename();
    fs::create_symlink(fs::absolute(job.exec), job.dir / newexec);

    fs::path newconfig = job.dir / "validation.info";
    pt::write_info(newconfig.c_str(), job.tree);

    fs::path newcondor = job.dir / "condor.sub";
    fs::copy(job.condor, newcondor);

    dag << "JOB " << job.name << ' ' << newcondor.filename().string() << " DIR " << fs::absolute(job.dir).string() << '\n'
        << "VARS " << job.name << " exec=" << newexec << '\n';

    if (job.parents.size() > 0)
        dag << accumulate(job.parents.begin(), job.parents.end(), string("PARENT"),
               [](string instruction, string parent) { return instruction + ' ' + parent; })
            << " CHILD " << job.name << '\n';

    dag << '\n';

    return dag;
}

vector<int> DAG::GetIOVsFromTree (const pt::ptree& tree) const
{
    if (!tree.count("IOV")) return {1};

    vector<int> IOVs = tree.get<vector<int>>("IOV", tok_int);
    if (!is_sorted(IOVs.begin(), IOVs.end())) throw ConfigError("IOVs are not sorted");

    return IOVs;
}

void DAG::GitIgnore () const
{
    cout << "Adding .gitignore" << endl;
    fs::path p = dir / ".gitignore";
    ofstream gitignore;
    gitignore.open(p.c_str());
    gitignore << "*\n";
    gitignore.close();
}

void DAG::BoostSoftLinks () const
{
    cout << "Creating soft links to the boost libraries" << endl;
    static const fs::path boostDir = CMSSW_RELEASE_BASE / "external" / SCRAM_ARCH / "lib";
    cout << boostDir << endl;
    if (!fs::exists(boostDir)) throw ConfigError("Unable to find the directory containing the boost libraries");
    static const vector<string> boostLibs {"filesystem", "program_options", "system", "thread", "signals", "date_time"};
    for (string boostLib: boostLibs) {
        fs::path p = "libboost_" + boostLib + ".so";
        cout << p << endl;
        fs::create_symlink(boostDir / p, dir / p);
    }
}

////////////////////////////////////////////////////////////////////////////////
/// function DAG::DAG
///
/// Constructor, making basic checks and opening file to write DAGMAN
DAG::DAG
    (string file) : //!< name of the INFO config file
        CMSSW_RELEASE_BASE(getenv("CMSSW_RELEASE_BASE")),
        CMSSW_BASE(getenv("CMSSW_BASE")),
        SCRAM_ARCH(getenv("SCRAM_ARCH"))
{
    cout << bold << "Starting validation" << normal << endl;

    cout << "Reading the config " << file << endl;
    pt::ptree tree;
    pt::read_info(file, tree);

    cout << "Getting working and LFS directories" << endl;
    dir = tree.get<string>("name");
    if (fs::exists(dir)) throw ConfigError("Output directory already exists");
    LFS = tree.get<string>("LFS");
    if (LFS.filename().string() != dir.filename().string()) {
        LFS /= dir.filename();
        cout << "LFS redefined as " << LFS << endl;
    }
    if (LFS == dir) throw ConfigError("The working directory and the Large File storage should not be the same.");

    cout << "Getting default config for HTC jobs" << endl;
    condor = tree.count("condor") ? tree.get<string>("condor") : (CMSSW_BASE / "src/Alignment/OfflineValidation/bin/.default.sub");
    // TODO: maybe a better solution would be to find the file in the same directory?
    //       const_assert to check that this file exists at compilation time?

    cout << "Creating directory " << dir << endl;
    fs::create_directory(dir);
    fs::copy(file, dir / "config.info");
    GitIgnore();
    //BoostSoftLinks(); // TODO: remove?

    cout << "Extracting alignments and validations from config file" << endl;
    alignments = tree.get_child("alignments");
    if (!tree.count("validations")) throw ConfigError("No validation block found.");
    ptGCP = tree.get_child_optional("validations.GCP");
    ptDMR = tree.get_child_optional("validations.DMR");
    if (!ptGCP && !ptDMR) throw ConfigError("No known validation found.");

    cout << "Opening dag file" << endl;
    fs::path p = dir / "dag";
    dag.open(p.c_str());
}

fs::path DAG::CopyExe (fs::path Exec, fs::path OutDir) const
{
    cout << "Copying the executable" << endl;
    fs::create_directories(OutDir);
    static const fs::path execDir = CMSSW_BASE / "bin" / SCRAM_ARCH;
    cout << "Copying " << Exec << " from " << execDir << " to " << OutDir << endl;
    if (!fs::exists(execDir)) throw ConfigError("Unable to find the directory containing the executables");
    fs::path original(execDir / Exec),
             exec(OutDir / Exec);
    fs::copy(original, exec);
    return exec;
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

    fs::path exec = CopyExe("GCP", dir / "GCP");

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
            fs::create_directories(LFS / subdir);

            string jobname = "GCP_" + name;
            if (multiIOV) jobname +=  '_' + to_string(IOV);

            Job job(jobname, exec, dir / subdir, config);

            string twig_ref  = it.second.get<string>("reference"),
                   twig_test = it.second.get<string>("test");

            job.tree.put<string>("output", (LFS / subdir).string());
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
void DAG::DMRsingle (string name, pt::ptree validation)
{
    cout << bold << "DMR single: " << name << normal << endl;

    vector<int> IOVs = GetIOVsFromTree(validation);
    bool multiIOV = IOVs.size() > 1;

    vector<string> geoms = validation.get<vector<string>>("alignments", tok_str);
    validation.erase("alignments");

    fs::path config = validation.count("condor") ? validation.get<string>("condor") : condor;
    validation.erase("condor");

    fs::path exec = CopyExe("DMRsingle", dir / "DMR/single");

    // create the jobs
    for (string geom: geoms) {

        fs::path subdir = "DMR/single/" + name + '/' + geom;
        
        for (int IOV: IOVs) {

            validation.put("IOV", IOV);

            // light files 
            fs::path subsubdir = subdir;
            if (multiIOV) subsubdir /= fs::path(to_string(IOV));
            cout << "The validation will be performed in " << subsubdir << endl;
            fs::create_directories(dir / subsubdir);
            fs::create_directories(LFS / subsubdir);

            // job
            string jobname = "DMRsingle_" + name + '_' + geom;
            if (multiIOV) jobname +=  '_' + to_string(IOV);
            // NB: the name should be changed with care, since it is used later on
            //     to define the hierarchy of jobs in the DAGMAN

            // alignment
            pt::ptree alignment = alignments.get_child(geom);
            alignment.erase("files");
            alignment.put("output", (LFS / subsubdir).string());

            // local config
            Job job(jobname, exec, dir / subsubdir, config);
            job.tree.put_child("alignment", alignment);
            job.tree.put_child("validation", validation);

            // replace wildcard by IOV
            string dataset = job.tree.get<string>("validation.dataset");
            boost::replace_first(dataset, "*", to_string(IOV));
            job.tree.put("validation.dataset", dataset);

            // "submit"
            dag << job;
        }

        // write output path in config (will be used for merge job)
        string key = geom + ".files.DMR.single." + name;
        string value = (LFS / subdir).string() + (multiIOV ? "/*" : "");
        alignments.put<string>(key, value);
    }
}

void DAG::DMRmerge (string name, pt::ptree validation)
{
    cout << bold << "DMR merge: " << name << normal << endl;

    cout << "Getting the single jobs from which the merge job depends:" << flush;
    vector<string> singles = validation.get<vector<string>>("singles", tok_str);
    // e.g. if one would like to compare Data vs. MC, then different datasets are necessary
    for (string single: singles) cout << ' ' << single;
    cout << endl;

    cout << "Getting the respective lists of IOVs (and sorting them by size)" << endl;
    vector<vector<int>> IOVss;
    for (string single: singles) {
        pt::ptree single_tree = ptDMR->get_child("single." + single);
        vector<int> IOVs = GetIOVsFromTree(single_tree);
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

    fs::path config = validation.count("condor") ? fs::path(validation.get<string>("condor")) : condor,
             subdir = "DMR/merge/" + name;

    fs::path exec = CopyExe("DMRmerge", dir / "DMR/merge");

    for (int globalIOV: IOVs) {

        fs::path subsubdir = subdir;
        if (multiIOV) subsubdir /= fs::path(to_string(globalIOV));
        cout << "The validation will be performed in " << subsubdir << endl;
        fs::create_directories(dir / subsubdir);
        fs::create_directories(LFS / subsubdir);

        string jobname = "DMRmerge_" + name;
        if (multiIOV) jobname +=  '_' + to_string(globalIOV);

        validation.put<string>("output", (LFS / subsubdir).string());

        Job job(jobname, exec, dir / subsubdir, config);
        job.tree.put_child("alignments", alignments);
        job.tree.put_child("validation", validation);

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

                string key = "files.DMR.single." + single;
                boost::optional<string> input = it.second.get_optional<string>(key);
                if (!input) continue;

                cout << ali << ' ' << flush;

                boost::replace_first(*input, "*", to_string(localIOV));
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

    // write output path for trend job
    for (string single: singles) 
    for (auto it: alignments) {
        string ali = it.first;

        string key = "files.DMR.single." + single;
        boost::optional<string> input = it.second.get_optional<string>(key);
        if (!input) continue;

        string value = (LFS / subdir).string() + (multiIOV ? "/*" : "");
        alignments.put<string>(ali + ".files.DMR.merge." + name, value);
    }
}

void DAG::DMRtrend (string name, pt::ptree validation)
{
    cout << bold << "DMR trend: " << name << normal << endl;

    // TODO
}

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

    using DMRstep = void (DAG::*)(string, pt::ptree);
    vector<pair<string, DMRstep>> steps = {{ "single", &DAG::DMRsingle},
                                           { "merge" , &DAG::DMRmerge }/*,
                                           { "trend" , &DAG::DMRtrend } */};

    for (auto step: steps) {
        string name = step.first;

        // getting the step block (if existing)
        boost::optional<pt::ptree&> block = ptDMR->get_child_optional(step.first);
        if (!block) continue;

        // calling corresponding private method / subroutine
        cout << "Generating DMR " << step.first << " configuration files" << endl;
        DMRstep f = step.second;
        for (auto& it: *block) (this->*f)(/* name = */ it.first, /* subtree = */ it.second);

        // copying INFO file as back-up (can be used for future "preexisting" validations)
        fs::path output = dir / ("DMR/" + name + "/alignments.info");
        pt::write_info(output.c_str(), alignments);
        // note: there is here a weakness, because it assumes that the directory has been created in the subroutine
    }
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
int DAG::submit (bool dry) const
{
    cout << "Submitting DAGMAN" << endl;

    string instruction = "condor_submit_dag -batch-name "
                        + dir.filename().string();

    if (dry) instruction += " -no_submit";

    fs::path p = dir / "dag";
    instruction += ' ' + p.string();

    cout << instruction << endl;
    return system(instruction.c_str());
}

}
