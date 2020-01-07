// STL

#include <fstream>
#include <vector>

// Boost

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
    boost::filesystem::path dir, LFS, condor; //!< path to directories where files are stored (dir for local config and plots, LSF for heavy root files)
    std::ofstream dag; //!< file to which the hierarchy of the job is written, submitted at the end of the program

    std::vector<int> GetIOVsFromTree (const boost::property_tree::ptree& tree);

    void DMRsingle (std::string name, boost::property_tree::ptree& tree);
    void DMRmerge (std::string name, boost::property_tree::ptree& tree);
    //void DMRtrend (std::string name, boost::property_tree::ptree& tree);

public:
    DAG (std::string file); 

    void GCP (); 
    void DMR ();
    // TODO: PV, Zµµ, MTS, etc.

    void close ();
    int submit () const;
};

}
