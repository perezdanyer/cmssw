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
#include "Alignment/OfflineValidation/macros/FitPVResiduals_2.C"
#include <Alignment/OfflineValidation/interface/TkAlStyle.h>

using namespace std;
using namespace AllInOneConfig;

namespace pt = boost::property_tree;

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

    //Read all configure variables and set default for missing keys
    bool doMaps = validation.get_child_optional("doMaps") ? validation.get<bool>("doMaps") : false;
    bool stdResiduals = validation.get_child_optional("stdResiduals") ? validation.get<bool>("stdResiduals") : true;
    bool autoLimits = validation.get_child_optional("autoLimits") ? validation.get<bool>("autoLimits") : false;


    int m_dxyPhiMax = validation.get_child_optional("m_dxyPhiMax") ? validation.get<int>("m_dxyPhiMax") : 40;
    int m_dzPhiMax = validation.get_child_optional("m_dzPhiMax") ? validation.get<int>("m_dzPhiMax") : 40;
    int m_dxyEtaMax = validation.get_child_optional("m_dxyEtaMax") ? validation.get<int>("m_dxyEtaMax") : 40;
    int m_dzEtaMax = validation.get_child_optional("m_dzEtaMax") ? validation.get<int>("m_dzEtaMax") : 40;
    int m_dxyPhiNormMax = validation.get_child_optional("m_dxyPhiNormMax") ? validation.get<int>("m_dxyPhiNormMax") : 0.5;
    int m_dzPhiNormMax = validation.get_child_optional("m_dzPhiNormMax") ? validation.get<int>("m_dzPhiNormMax") : 0.5;
    int m_dxyEtaNormMax = validation.get_child_optional("m_dxyEtaNormMax") ? validation.get<int>("m_dxyEtaNormMax") : 0.5;
    int m_dzEtaNormMax = validation.get_child_optional("m_dzEtaNormMax") ? validation.get<int>("m_dzEtaNormMax") : 0.5;
    int w_dxyPhiMax = validation.get_child_optional("w_dxyPhiMax") ? validation.get<int>("w_dxyPhiMax") : 150;
    int w_dzPhiMax = validation.get_child_optional("w_dzPhiMax") ? validation.get<int>("w_dzPhiMax") : 150;
    int w_dxyEtaMax = validation.get_child_optional("w_dxyEtaMax") ? validation.get<int>("w_dxyEtaMax") : 150;
    int w_dzEtaMax = validation.get_child_optional("w_dzEtaMax") ? validation.get<int>("w_dzEtaMax") : 1000;
    int w_dxyPhiNormMax = validation.get_child_optional("w_dxyPhiNormMax") ? validation.get<int>("w_dxyPhiNormMax") : 1.8;
    int w_dzPhiNormMax = validation.get_child_optional("w_dzPhiNormMax") ? validation.get<int>("w_dzPhiNormMax") : 1.8;
    int w_dxyEtaNormMax = validation.get_child_optional("w_dxyEtaNormMax") ? validation.get<int>("w_dxyEtaNormMax") : 1.8;
    int w_dzEtaNormMax = validation.get_child_optional("w_dzEtaNormMax") ? validation.get<int>("w_dzEtaNormMax") : 1.8;

    //Set configuration string for CompareAlignments class
    TString filesAndLabels;

    for(std::pair<std::string, pt::ptree> childTree : alignments){
        filesAndLabels += childTree.second.get<std::string>("file") + "/PV.root=" + childTree.second.get<std::string>("title") + "|" + childTree.second.get<std::string>("color") + "|" + childTree.second.get<std::string>("style") + " , ";
    }

    filesAndLabels.Remove(filesAndLabels.Length()-3);

    //Do file comparisons
    CompareAlignments comparer(main_tree.get<std::string>("output"));
    comparer.doComparison(filesAndLabels);

    //Create plots
    // initialize the plot y-axis ranges
    thePlotLimits->init(m_dxyPhiMax,         // mean of dxy vs Phi
                        m_dzPhiMax,          // mean of dz  vs Phi
                        m_dxyEtaMax,         // mean of dxy vs Eta
                        m_dzEtaMax,          // mean of dz  vs Eta
                        m_dxyPhiNormMax,     // mean of dxy vs Phi (norm)
                        m_dzPhiNormMax,      // mean of dz  vs Phi (norm)
                        m_dxyEtaNormMax,     // mean of dxy vs Eta (norm)
                        m_dzEtaNormMax,      // mean of dz  vs Eta (norm)
                        w_dxyPhiMax,         // width of dxy vs Phi
                        w_dzPhiMax,          // width of dz  vs Phi
                        w_dxyEtaMax,         // width of dxy vs Eta
                        w_dzEtaMax,          // width of dz  vs Eta
                        w_dxyPhiNormMax,     // width of dxy vs Phi (norm)
                        w_dzPhiNormMax,      // width of dz  vs Phi (norm)
                        w_dxyEtaNormMax,     // width of dxy vs Eta (norm)
                        w_dzEtaNormMax       // width of dz  vs Eta (norm)
		      );

    FitPVResiduals("",stdResiduals,doMaps,"",autoLimits);
    

    return EXIT_SUCCESS; 
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
int main (int argc, char * argv[])
{
    return exceptions<merge>(argc, argv);
}
#endif





