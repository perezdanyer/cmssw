#include "DAG.h"

#include <iostream>
#include <string>
#include <algorithm>
#include <map>

using namespace std;
namespace fs = boost::filesystem;
namespace pt = boost::property_tree; // used to read the config file

// Example of DAGMAN:
// ```
// JOB jobname1 condor.sub
// VARS jobname1 key=value1
//
// JOB jobname2 condor.sub
// VARS jobname2 key=value2
//
// JOB merge merge.sub
// VARS merge otherkey=value3
//
// PARENT jobname1 jobname2 CHILD merge
// ```

// `boost::optional` allows to safely initialiase an object
// example:
// ```
// optional<int> i = someFunction();
// if (i) cout << *i << endl;
// ```
// NB: std::optional exists in C++17 but there are issues when using Property Trees...

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

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

struct Job {

    const string name, exec;
    const fs::path subdir;
    pt::ptree tree;

    vector<string> parents;

    Job (string Name, string Exec, fs::path Subdir) :
        name(Name), exec(Exec), subdir(Subdir)
    {
        cout << "Declaring job " << name << " to be run with " << exec << endl;
    }
};

// Example:
// ```
// Job single1(...), single2(...);
// Job merge(...);
// merge.parents = {&single1, &single2};
// dag << single1 << single2 << merge;
// ```
ostream& operator<< (ostream& dag, const Job& job)
{
    dag << "JOB " << job.name << " condor.sub" << '\n'
        << "VARS " << job.name << " exec=" << job.exec << '\n';

    create_directories(job.subdir);
    fs::path p = job.subdir / "config.info";
    pt::write_info(p.c_str(), job.tree);
    // TODO: copy condor.sub?

    if (job.parents.size() > 0)
        dag << accumulate(job.parents.begin(), job.parents.end(), string("PARENT"),
                [](string instruction, string parent) { return instruction + ' ' + parent; })
            << " CHILD " << job.name << '\n';

    return dag;
}

vector<int> DAG::GetIOVsFromTree (const pt::ptree& tree)
{
    if (tree.count("IOVs")) {
        cout << "Getting IOVs" << endl;
        if (tree.count("IOV")) {
            cerr << "There shouldn't be both 'IOV' and 'IOVs' statements in the same block\n"; // TODO: be more precise
            exit(EXIT_FAILURE);
        }
        vector<int> IOVs = tree.get<vector<int>>("IOVs", tok_int);
        if (!is_sorted(IOVs.begin(), IOVs.end())) {
            cerr << "IOVs are not sorted\n";
            exit(EXIT_FAILURE);
        }
        return IOVs;
    }
    else if (tree.count("IOV")) {
        cout << "Getting single IOV" << endl;
        return {tree.get<int>("IOV")};
    }
    else
        return vector<int>();
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
        cerr << "No validation found!\n";
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

    cout << "Opening dag file" << endl;
    fs::path p = dir / "dag";
    dag.open(p.c_str());
}

void DAG::GCP ()
{
    if (!ptGCP) return;

    cout << "Generating GCP configuration files" << endl;
    for (pair<string,pt::ptree> it: *ptGCP) {

        string name = it.first;
        cout << "GCP: " << name << endl;

        vector<int> IOVs = GetIOVsFromTree(it.second);
        bool multiIOV = IOVs.size() > 1;

        if (multiIOV) cout << "Several IOVs were found";

        for (int IOV: IOVs) {
            cout << "Preparing validation for IOV " << IOV << endl;
            it.second.put<int>("IOV", IOV);

            fs::path subdir = dir / fs::path("GCP") / name;
            if (multiIOV) subdir /= fs::path(to_string(IOV));
            cout << "The validation will be performed in " << subdir << endl;

            string jobname = "GCP_" + name;
            if (multiIOV) jobname +=  '_' + to_string(IOV);

            Job job(jobname, "GCP", subdir);

            string twig_ref  = it.second.get<string>("reference"),
                   twig_test = it.second.get<string>("test");

            job.tree.put<string>("LFS", LFS.string());
            job.tree.put_child(twig_ref, alignments.get_child(twig_ref));
            job.tree.put_child(twig_test, alignments.get_child(twig_test));
            job.tree.put_child(name, it.second);
            if (job.tree.count("IOVs")) job.tree.erase("IOVs");

            cout << "Queuing job" << endl;
            dag << job;
        }
    }
}

pair<string, vector<int>> DAG::DMRsingle (string name, pt::ptree& tree)
{
    cout << "DMR: " << name << endl;

    vector<int> IOVs = GetIOVsFromTree(tree);
    bool multiIOV = IOVs.size() > 1;

    // aligns = alignments, but variable name has already been taken :p
    vector<string> aligns = tree.get<vector<string>>("alignments", tok_str);
    // first, remove preexisting alignments
    for (auto a = aligns.begin(); a != aligns.end(); /* nothing */) {
        bool preexisting = alignments.get_child(*a).count(name);
        if (preexisting) a->erase();
        else             ++a;
    }

    // then create the jobs
    for (int IOV: IOVs)
    for (string a: aligns) {
        tree.put<int>("IOV", IOV);

        fs::path subdir = dir / fs::path("DMR/single") / name;
        if (multiIOV) subdir /= fs::path(to_string(IOV));
        cout << "The validation will be performed in " << subdir << endl;

        string jobname = "DMRsingle_" + name;
        if (multiIOV) jobname +=  '_' + to_string(IOV);

        Job job(jobname, "DMR", subdir);
        job.tree.put<string>("LFS", LFS.string());
        job.tree.put_child("alignments." + a, alignments.get_child(a));
        job.tree.put_child(name, tree);
        if (job.tree.count("IOVs")) job.tree.erase("IOVs");

        // TODO: write the name of the outputs in alignments

        cout << "Queuing job" << endl;
        dag << job;
    }

    return { name, IOVs };
}

//void DAG::PrepareDMRmerge ()
//{
//    // TODO
//            string name = it.first;
//            cout << name << endl;
//
//            vector<string> singles = it.second.get<vector<string>>("single", tok_str);
//
//            for (string& single: singles) {
//
//                if (DMRsingles.find(single) == DMRsingles.end()) {
//                    // TODO: look it up in the alignments
//                    cerr << single << " could not be found\n";
//                    exit(EXIT_FAILURE);
//                }
//
//                // TODO: IOVs?
//            }
//}

void DAG::DMRtrend ()
{
    // TODO
}

void DAG::DMR ()
{
    //if (!ptDMR) return;

    //boost::optional<pt::ptree&> singles = ptDMR->get_child_optional("single"),
    //                            merges  = ptDMR->get_child_optional("merge"),
    //                            trends  = ptDMR->get_child_optional("trend");

    //map<int,vector<string>> singleJobs; // key = IOV, value = list of jobs

    //if (singles) {
    //    cout << "Generating DMR single configuration files" << endl;
    //    for (pair<string,pt::ptree>& it: *singles) {
    //        auto DMRsingle = DMRsingle(it.first, it.second);
    //        DMRsingles.insert( DMRsingle );
    //    }
    //}

    //if (merges) {
    //    cout << "Generating DMR merge configuration files" << endl;
    //    for (pair<string,pt::ptree> it: *merges) {
    //        auto DMRmerge = DMRmerge(it.first, it.second);
    //        DMRmerges.insert( DMRmerge );
    //    }
    //}

    //if (trends) {
    //    // TODO
    //}
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
