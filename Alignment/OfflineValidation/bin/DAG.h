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

    const boost::filesystem::path CMSSW_RELEASE_BASE, CMSSW_BASE, SCRAM_ARCH;
    // NB: currently implemented using the standard `getenv`
    //     -> using PO is also possible (implementation is commented out in `Options.cc`)

    boost::property_tree::ptree alignments; //!< subtree containing the alignments, modified along the different steps
    boost::optional<boost::property_tree::ptree> ptGCP, //!< GCP block
                                                 ptDMR; //!< DMR validation blocks
    // TODO: add blocks for other validations
    boost::filesystem::path dir, LFS, condor; //!< path to directories where files are stored (dir for local config and plots, LSF for heavy root files)
    std::ofstream dag; //!< file to which the hierarchy of the job is written, submitted at the end of the program

    std::vector<int> GetIOVsFromTree (const boost::property_tree::ptree& tree) const;

    void GitIgnore () const;
    void BoostSoftLinks () const;

    boost::filesystem::path CopyExe (boost::filesystem::path Exec, boost::filesystem::path OutDir) const;

    void DMRsingle (std::string name, boost::property_tree::ptree validation);
    void DMRmerge  (std::string name, boost::property_tree::ptree validation);
    void DMRtrend  (std::string name, boost::property_tree::ptree validation);

public:
    DAG (std::string file); 

    void GCP (); 
    void DMR ();
    // TODO: add methods for other validations

    void close ();
    int submit (bool dry = false) const;
};

}
