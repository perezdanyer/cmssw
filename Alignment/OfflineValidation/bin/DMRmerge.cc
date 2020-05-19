#include <cstdlib>
#include <string>
#include <iostream>
#include <numeric>
#include <functional>

#include "exceptions.h"
#include "toolbox.h"
#include "Options.h"

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>

#include <TString.h>

#include <Alignment/OfflineValidation/interface/CompareAlignments.h>
#include <Alignment/OfflineValidation/interface/PlotAlignmentValidation.h>
#include <Alignment/OfflineValidation/interface/TkAlStyle.h>

using namespace std;
using namespace AllInOneConfig;

namespace pt = boost::property_tree;
namespace fs = boost::filesystem;

std::string getVecTokenized(pt::ptree& tree, const std::string& name, const std::string& token){
    std::string s;

    for(std::pair<std::string, pt::ptree> childTree : tree.get_child(name)){
        s+= childTree.second.get_value<std::string>() + token;
    }

    return s.substr(0, s.size()-token.size());
}

int merge (int argc, char * argv[]){
    // parse the command line
    Options options;
    options.helper(argc, argv);
    options.parser(argc, argv);

    //Read in AllInOne json config
    pt::ptree main_tree;
    pt::read_json(options.config, main_tree);

    pt::ptree alignments = main_tree.get_child("alignments");
    pt::ptree validation = main_tree.get_child("validation");

    //Set configuration string for CompareAlignments class
    TString filesAndLabels;

    for(std::pair<std::string, pt::ptree> childTree : alignments){
        filesAndLabels += childTree.second.get<std::string>("file") + "/DMR.root=" + childTree.second.get<std::string>("title") + "|" + childTree.second.get<std::string>("style") + "|" + childTree.second.get<std::string>("color") + ",";
    }

    filesAndLabels.Remove(filesAndLabels.Length()-1);

    //Do file comparisons
    CompareAlignments comparer;
   // comparer.doComparison(filesAndLabels);

    //Create plots
    TkAlStyle::legendheader = "";
    TkAlStyle::legendoptions = getVecTokenized(validation, "legendoptions", " ");

    TkAlStyle::set(INTERNAL, NONE, "", validation.get<std::string>("customrighttitle"));

    gStyle->SetTitleH(0.07);
    gStyle->SetTitleW(1.00);
    gStyle->SetTitleFont(132);

    PlotAlignmentValidation plotter(false);

    for(std::pair<std::string, pt::ptree> childTree : alignments){
        plotter.loadFileList((childTree.second.get<std::string>("file") + "/DMR.root").c_str(), childTree.second.get<std::string>("title"), childTree.second.get<int>("color"), childTree.second.get<int>("style"));
    }

    std::cout << getVecTokenized(validation, "curves", ",");

    plotter.setOutputDir(main_tree.get<std::string>("output"));
    plotter.useFitForDMRplots(false);
    plotter.setTreeBaseDir();
    plotter.plotDMR(getVecTokenized(validation, "methods", ","), validation.get<int>("minimum"), ""); //;getVecTokenized(validation, "curves", ","));
    plotter.plotSurfaceShapes("TrackerOfflineValidation");
    plotter.plotChi2("result.root");

    vector<int> moduleids; // = {.oO[moduleid]Oo.};
    for (auto moduleid : moduleids) {
        plotter.residual_by_moduleID(moduleid);
    }

    return EXIT_SUCCESS; 
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
int main (int argc, char * argv[])
{
    return exceptions<merge>(argc, argv);
}
#endif
