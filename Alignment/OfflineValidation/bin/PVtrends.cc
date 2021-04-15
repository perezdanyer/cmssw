#include <cstdlib>
#include <string>
#include <iostream>
#include <numeric>
#include <functional>
#include <unistd.h>

#include "exceptions.h"
#include "toolbox.h"
#include "Options.h"

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>

#include "TString.h"
#include "TColor.h"

#include "Alignment/OfflineValidation/interface/CompareAlignments.h"
#include "Alignment/OfflineValidation/interface/TkAlStyle.h"
#include "Alignment/OfflineValidation/interface/PlotTrends.h"
#include "Alignment/OfflineValidation/interface/PreparePVTrends.h"

using namespace std;
using namespace AllInOneConfig;
namespace fs = experimental::filesystem;

namespace pt = boost::property_tree;

int trends(int argc, char* argv[]) {

  // parse the command line
  cout << "line 34" << flush;

  Options options;
  options.helper(argc, argv);
  options.parser(argc, argv);


  cout << "line 41" << flush;
  
  //Read in AllInOne json config
  pt::ptree main_tree;
  pt::read_json(options.config, main_tree);
  
  pt::ptree alignments = main_tree.get_child("alignments");
  pt::ptree validation = main_tree.get_child("validation");

  cout << "line 50" << flush;
  
  //Read all configure variables and set default for missing keys
  bool doRMS = validation.get_child_optional("doRMS") ? validation.get<bool>("doRMS") : true;
  bool pixelupdate = validation.get_child_optional("pixelupdate") ? validation.get<bool>("pixelupdate") : true;
  bool showlumi = validation.get_child_optional("showlumi") ? validation.get<bool>("showlumi") : true;
  TString Year = validation.get_child_optional("Year") ? validation.get<string>("Year") : "Run2";
  string myValidation = validation.get<string>("output");
  TString outputdir = validation.get<string>("output");
  TString lumiInputFile = validation.get_child_optional("lumiInputFile") ? validation.get<string>("lumiInputFile") : "lumiperFullRun2_delivered.txt";
  
  vector<int> IOVlist;
  if (validation.get_child_optional("IOV")) {
    for (const pair<string, pt::ptree>& childTree : validation.get_child("IOV")) {
      IOVlist.push_back(childTree.second.get_value<int>());
      cout << childTree.second.get_value<int>() << flush;
    }
  }
  
  vector<string> labels{""};
  if (validation.get_child_optional("label")) {
    labels.clear();
    for (const pair<string, pt::ptree>& childTree : validation.get_child("label")) {
      labels.push_back(childTree.second.get_value<string>());
    }
  }
  
  vector<TString> Variables;
  if (validation.get_child_optional("Variable")) {
    for (const pair<string, pt::ptree>& childTree : validation.get_child("Variable")) {
      Variables.push_back(childTree.second.get_value<string>());
    }
  }
  
  vector<int> pixelupdateruns{271866, 272008, 276315, 278271, 280928, 290543, 294927, 297281, 298653, 299443, 300389, 301046, 302131, 303790, 303998, 304911, 313041, 314881, 315257, 316758, 317475, 317485, 317527, 317661, 317664, 318227, 320377, 321831, 322510, 322603, 323232, 324245};
  //
  ////Set configuration string for CompareAlignments class
  TString filesAndLabels;

  //for (const pair<string, pt::ptree>& childTree : alignments) {
  //  fs::create_directory(childTree.second.get<string>("input"));
  //  for (const pair<string, pt::ptree>& childTreeFiles : alignments.get_child("files"))
  //    fs::create_symlink(childTreeFiles.second.get_value<string>(), childTree.second.get<string>("input"));
  //}

  vector<string> geometries;
  vector<Color_t> colors{kBlue, kRed, kGreen};
  for (const pair<string, pt::ptree>& childTree : alignments) {
    cout << childTree.second.get<string>("file") << flush;
    cout << childTree.second.get<string>("title") << flush;
    cout << childTree.second.get<int>("color") << flush;
    filesAndLabels += childTree.second.get<string>("file") + "=" +
                      childTree.second.get<string>("title") + ",";
    geometries.push_back(childTree.second.get<string>("title"));
    //colors.push_back(childTree.second.get<int>("color"));
  }

  TString LumiFile = getenv("CMSSW_BASE");
  if (lumiInputFile.BeginsWith("/"))
    LumiFile = lumiInputFile;
  else {
    LumiFile += "/src/Alignment/OfflineValidation/data/";
    LumiFile += lumiInputFile;
  }
  fs::path pathToLumiFile = LumiFile.Data();
  if (!(fs::exists(pathToLumiFile))) {
    cout << "ERROR: lumi-per-run file (" << LumiFile.Data() << ") not found!" << endl << "Please check!" << endl;
    exit(EXIT_FAILURE);
  }
  if (!LumiFile.Contains(Year)) {
    cout << "WARNING: lumi-per-run file and year do not match, luminosity on the x-axis and labels might not match!"
         << endl;
  }

  vector<TString> variables{"dxy_phi_vs_run",
                            "dxy_eta_vs_run",
                            "dz_phi_vs_run",
                            "dz_eta_vs_run"};
  vector<string> YaxisNames{"Bias of d_{xy}(#phi) vs run number",
                            "Bias of d_{xy}(#eta) vs run number",
                            "Bias of d_{z}(#phi) vs run number",
                            "Bias of d_{z}(#eta) vs run number"};

  cout << "line 123" << flush;

  cout << filesAndLabels << flush;
  cout << LumiFile << flush;

  PreparePVTrends prepareTrends;
  prepareTrends.MultiRunPVValidation(filesAndLabels, doRMS, LumiFile);

  PlotTrends plotter(variables, YaxisNames);

  vector<pair<int, double>> lumiIOVpairs = plotter.lumiperIOV(IOVlist, LumiFile);
  sort(IOVlist.begin(), IOVlist.end());
  IOVlist.erase(unique(IOVlist.begin(), IOVlist.end()), IOVlist.end());

  for (const auto &Variable : Variables) {
    plotter.PlotDMRTrends(IOVlist, Variable, labels, Year, myValidation, {"Alignment during data taking", "End of year re reconstruction", "Legacy reprocessing"}, colors, outputdir, pixelupdate, pixelupdateruns, showlumi, LumiFile, lumiIOVpairs, "", 0);
  }


  return EXIT_SUCCESS;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
int main(int argc, char* argv[]) { return exceptions<trends>(argc, argv); }
#endif
