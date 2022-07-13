#include <cstdlib>
#include <iostream>
#include <vector>

#include <TString.h>

#include "exceptions.h"
#include "toolbox.h"
#include "Options.h"

#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>

#include <Alignment/OfflineValidation/interface/GeometryComparisonPlotter.h>
#include <Alignment/OfflineValidation/scripts/visualizationTracker.C>
#include <Alignment/OfflineValidation/macros/makeArrowPlots.C>
//#include "GeometryComparisonPlotter.h"

// for debugging
#include <TObject.h>

using namespace std;
using namespace AllInOneConfig;

namespace pt = boost::property_tree;
namespace fs = boost::filesystem;

void comparisonScript(
        pt::ptree GCPoptions, 
        TString inFile,  //="mp1510_vs_mp1509.Comparison_commonTracker.root", // TODO: get ROOT file
        TString outDir = "outputDir/",
        TString alignmentName = "Alignment",
        TString referenceName = "Ideal")
{
  // the output directory is created if it does not exist
  fs::create_directories(outDir.Data());

  TString modulesToPlot = "all";
  TString transDir = outDir+"/Translations";
  TString rotDir = outDir+"/Rotations";
  fs::create_directories(transDir.Data());
  fs::create_directories(rotDir.Data());

  bool plotOnlyGlobal   = GCPoptions.get_child_optional("plotOnlyGlobal") ? GCPoptions.get<bool>("plotOnlyGlobal") : false;
  bool plotPng          = GCPoptions.get_child_optional("plotPng") ? GCPoptions.get<bool>("plotPng") : false;
  bool makeProfilePlots = GCPoptions.get_child_optional("makeProfilePlots") ? GCPoptions.get<bool>("makeProfilePlots") : true;

  // Plot Translations
  GeometryComparisonPlotter* trans = new GeometryComparisonPlotter(
      inFile, transDir, modulesToPlot, alignmentName, referenceName, plotOnlyGlobal, makeProfilePlots, 0);
  // x and y contain the couples to plot
  // -> every combination possible will be performed
  // /!\ always give units (otherwise, unexpected bug from root...)
  vector<TString> x{"r", "phi", "z"};
  vector<TString> y{"dr", "dz", "rdphi", "dx", "dy"};
  vector<TString> xmean{"x", "y", "z", "r"};

  trans->SetBranchUnits("x", "cm");
  trans->SetBranchUnits("y", "cm");
  trans->SetBranchUnits("z", "cm");  //trans->SetBranchMax("z", 100); trans->SetBranchMin("z", -100);
  trans->SetBranchUnits("r", "cm");
  trans->SetBranchUnits("phi", "rad");
  trans->SetBranchUnits("dx", "#mum");  //trans->SetBranchMax("dx", 10); trans->SetBranchMin("dx", -10);
  trans->SetBranchUnits("dy", "#mum");  //trans->SetBranchMax("dy", 10); trans->SetBranchMin("dy", -10);
  trans->SetBranchUnits("dz", "#mum");
  trans->SetBranchUnits("dr", "#mum");
  trans->SetBranchUnits("rdphi", "#mum rad");

  trans->SetBranchSF("dx", 10000);
  trans->SetBranchSF("dy", 10000);
  trans->SetBranchSF("dz", 10000);
  trans->SetBranchSF("dr", 10000);
  trans->SetBranchSF("rdphi", 10000);


  trans->SetGrid(1, 1);
  trans->MakePlots(x, y, GCPoptions);  // default output is pdf, but png gives a nicer result, so we use it as well
  // remark: what takes the more time is the creation of the output files,
  //         not the looping on the tree (because the code is perfect, of course :p)
  if (plotPng) {
    trans->SetPrintOption("png");
    trans->MakePlots(x, y, GCPoptions);
  }

  trans->MakeTables(xmean, y, GCPoptions);

  // Plot Rotations
  GeometryComparisonPlotter* rot = new GeometryComparisonPlotter(
      inFile, rotDir, modulesToPlot, alignmentName, referenceName, plotOnlyGlobal, makeProfilePlots, 2);
  // x and y contain the couples to plot
  // -> every combination possible will be performed
  // /!\ always give units (otherwise, unexpected bug from root...)
  vector<TString> b{"dalpha", "dbeta", "dgamma"};

  rot->SetBranchUnits("z", "cm");
  rot->SetBranchUnits("r", "cm");
  rot->SetBranchUnits("phi", "rad");
  rot->SetBranchUnits("dalpha", "mrad");
  rot->SetBranchUnits("dbeta", "mrad");
  rot->SetBranchUnits("dgamma", "mrad");

  rot->SetBranchSF("dalpha", 1000);
  rot->SetBranchSF("dbeta", 1000);
  rot->SetBranchSF("dgamma", 1000);

  rot->SetGrid(1, 1);
  rot->SetPrintOption("pdf");
  rot->MakePlots(x, b, GCPoptions);
  if (plotPng) {
    rot->SetPrintOption("png");
    rot->MakePlots(x, b, GCPoptions);
  }
}

void vizualizationScript(
        TString inFile,
        TString outDir,
        TString alignmentName, 
        TString referenceName)
{
  TString outputFileName = outDir+"/Visualization"; 
  fs::create_directories(outputFileName.Data());
  //title
  std::string line1 = alignmentName.Data();
  std::string line2 = referenceName.Data();
  //set subdetectors to see
  int subdetector1 = 1;
  int subdetector2 = 2;
  //translation scale factor
  int sclftr = 50;
  //rotation scale factor
  int sclfrt = 1;
  //module size scale factor
  float sclfmodulesizex = 1;
  float sclfmodulesizey = 1;
  float sclfmodulesizez = 1;
  //beam pipe radius
  float piperadius = 2.25;
  //beam pipe xy coordinates
  float pipexcoord = 0;
  float pipeycoord = 0;
  //beam line xy coordinates
  float linexcoord = 0;
  float lineycoord = 0;
  runVisualizer(inFile,
                outputFileName.Data(),
                line1,
                line2,
                subdetector1,
                subdetector2,
                sclftr,
                sclfrt,
                sclfmodulesizex,
                sclfmodulesizey,
                sclfmodulesizez,
                piperadius,
                pipexcoord,
                pipeycoord,
                linexcoord,
                lineycoord );

}

int GCP(int argc, char* argv[]) {
  /*
  TObject* printer = new TObject();
  printer->Info("GCPvalidation", "Hello!");
  // Hack to push through messages even without -v running
  // Very ugly coding, to run with std::cout -> run with -v option (GCP cfg.json -v)
  */


  // parse the command line
  Options options;
  options.helper(argc, argv);
  options.parser(argc, argv);

  std::cout << " ----- GCP validation plots -----" << std::endl;
  std::cout << " --- Digesting configuration" << std::endl;

  pt::ptree main_tree;
  pt::read_json(options.config, main_tree);

  pt::ptree alignments = main_tree.get_child("alignments");
  pt::ptree validation = main_tree.get_child("validation");

  
  pt::ptree GCPoptions = validation.get_child("GCP");

  // Read default ranges
  pt::ptree default_range;
  TString cmssw_base=std::getenv("CMSSW_BASE");
  TString default_range_path = cmssw_base + "/src/Alignment/OfflineValidation/test/GCP_defaultRange.json";
  pt::read_json(default_range_path.Data(), default_range);

  // If useDefaultRange, update ranges if not defined in GCPoptions
  bool useDefaultRange  = GCPoptions.get_child_optional("useDefaultRange") ? GCPoptions.get<bool>("useDefaultRange") : false;
  if (useDefaultRange) {
      BOOST_FOREACH(pt::ptree::value_type &v, default_range.get_child("")){
          if (GCPoptions.count(v.first) < 1) {
              GCPoptions.put(v.first, v.second.data());
          }
      }
  }

  pt::ptree comAl = alignments.get_child("comp");
  pt::ptree refAl = alignments.get_child("ref");

  // Read the options
  TString inFile = main_tree.get<std::string>("output") + "/GCPtree.root";
  TString outDir = main_tree.get<std::string>("output");
  TString modulesToPlot = "all";
  TString alignmentName = comAl.get<std::string>("title");
  TString referenceName = refAl.get<std::string>("title");

  std::cout << " --- Running comparison script" << std::endl;
  // Compare script
  comparisonScript(
          GCPoptions,
          inFile,
          outDir,
          alignmentName,
          referenceName);


  std::cout << " --- Running visualization script" << std::endl;
  // Visualization script
  vizualizationScript(
        inFile,
        outDir,
        alignmentName, 
        referenceName);

  std::cout << " --- Running arrow plot script" << std::endl;
  // Arrow plot
  TString arrowDir = outDir+"/ArrowPlots"; 
  makeArrowPlots(
        inFile.Data(), 
        arrowDir.Data());

  // TODO
  // - comments à la doxygen
  return EXIT_SUCCESS;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
int main(int argc, char* argv[]) { return exceptions<GCP>(argc, argv); }
#endif
