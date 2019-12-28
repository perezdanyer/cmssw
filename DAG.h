// STL

#include <fstream>
#include <vector>
#include <algorithm>

// Boost

#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/filesystem.hpp>

namespace AllInOneConfig {

////////////////////////////////////////////////////////////////////////////////
/// class DAG
///
/// Generates:
///  - condor configs, including dagman
///  - modified configs for each step
///
/// The class is mainly define to split the code in several function, only for readability.
///
/// Documentation about Boost Property Trees
///  - https://www.boost.org/doc/libs/1_70_0/doc/html/property_tree.html
///  - https://theboostcpplibraries.com/boost.propertytree
class DAG {

    boost::property_tree::ptree alignments; //!< subtree containing the alignments
    boost::optional<boost::property_tree::ptree> ptGCP, ptDMR; //!< validation blocks
    boost::filesystem::path dir, LFS; //!< path to directories where files are stored (dir for local config and plots, LSF for heavy root files)
    std::ofstream dag; //!< file to which the hierarchy of the job is written, submitted at the end of the program

    std::vector<int> GetIOVsFromTree (const boost::property_tree::ptree& tree);

    //struct Queue {
    //    string type;
    //    vector<int> IOVs;
    //    void Single (); // job for one geometry & one IOV
    //    void Merge (); // merge job for several geometries but one IOV
    //    void Trend (); // merge job for several IOVs

    //    Queue (string Type) :
    //        type(Type)
    //    { }
    //};
    //Queue DMRjobs;

    std::pair<std::string, std::vector<int>> DMRsingle 
                    (std::string name, boost::property_tree::ptree& tree);
    void DMRmerge ();
    void DMRtrend ();

public:
    DAG //!< constructor
        (std::string file); //!< name of the INFO config file

    void GCP (); //!< configure Geometry Comparison Plotter
    void DMR (); //!< configure "offline validation", measuring the local performance
    // TODO: PV, Zµµ, MTS, etc.

    void close (); //!< close the DAG man
    int submit () const; //!< submit the DAG man
};

}
