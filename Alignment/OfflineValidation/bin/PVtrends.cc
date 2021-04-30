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

  Options options;
  options.helper(argc, argv);
  options.parser(argc, argv);
  
  //Read in AllInOne json config
  pt::ptree main_tree;
  pt::read_json(options.config, main_tree);
  
  pt::ptree alignments = main_tree.get_child("alignments");
  pt::ptree validation = main_tree.get_child("validation");
  
  //Read all configure variables and set default for missing keys
  string myValidation = main_tree.get<std::string>("output");
  TString outputdir = main_tree.get<std::string>("output");
  bool doRMS = validation.get_child_optional("doRMS") ? validation.get<bool>("doRMS") : true;
  bool pixelupdate = validation.get_child_optional("pixelupdate") ? validation.get<bool>("pixelupdate") : true;
  bool showlumi = validation.get_child_optional("showlumi") ? validation.get<bool>("showlumi") : true;
  TString Year = validation.get_child_optional("Year") ? validation.get<string>("Year") : "Run2";
  TString lumiInputFile = validation.get_child_optional("lumiInputFile") ? validation.get<string>("lumiInputFile") : "lumiperFullRun2_delivered.txt";
  
  //vector<int> IOVlist;
  //if (validation.get_child_optional("IOV")) {
  //  for (const pair<string, pt::ptree>& childTree : validation.get_child("IOV")) {
  //    IOVlist.push_back(childTree.second.get_value<int>());
  //    cout << childTree.second.get_value<int>() << flush;
  //  }
  //}
  
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

  vector<string> names_of_alignments;
  vector<string> geometries;
  vector<Color_t> colors{kBlue, kRed, kGreen};
  for (const pair<string, pt::ptree>& childTree : alignments) {
    //cout << childTree.second.get<string>("title") << flush;
    //cout << childTree.second.get<int>("color") << flush;
    names_of_alignments.push_back(childTree.first.c_str());
    geometries.push_back(childTree.second.get<string>("title"));
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
  vector<string> YaxisNames{"of d_{xy}(#phi) [#mum]",
                            "of d_{xy}(#eta) [#mum]",
                            "of d_{z}(#phi) [#mum]",
                            "of d_{z}(#eta) [#mum]"};

  PreparePVTrends prepareTrends(outputdir, names_of_alignments, geometries);
  prepareTrends.MultiRunPVValidation(doRMS, LumiFile);
  vector<int> new_iov_list = prepareTrends.getIncludedRuns();

 //for(const auto &iov: IOVlist) {
 //  if(std::find(new_iov_list.begin(), new_iov_list.end(), iov) == new_iov_list.end())
 //    cout << iov << " not included in trend plot" << std::endl;
 //}

  PlotTrends plotter(variables, YaxisNames, "PV");

  vector<pair<int, double>> lumiIOVpairs = plotter.lumiperIOV(new_iov_list, LumiFile);
  sort(new_iov_list.begin(), new_iov_list.end());
  new_iov_list.erase(unique(new_iov_list.begin(), new_iov_list.end()), new_iov_list.end());

  for (const auto &Variable : Variables) {
    //plotter.PlotDMRTrends(new_iov_list, Variable, labels, Year, myValidation, {"Alignment during data-taking", "End-of-year re-reconstruction", "Legacy reprocessing"}, colors, outputdir, pixelupdate, pixelupdateruns, showlumi, LumiFile, lumiIOVpairs, "", 0);
    plotter.PlotDMRTrends(new_iov_list, Variable, labels, Year, myValidation, geometries, colors, outputdir, pixelupdate, pixelupdateruns, showlumi, LumiFile, lumiIOVpairs, "", 0);
  }


  return EXIT_SUCCESS;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
int main(int argc, char* argv[]) { return exceptions<trends>(argc, argv); }
#endif
