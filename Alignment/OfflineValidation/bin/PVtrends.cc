#include <cstdlib>
#include <string>
#include <tuple>
#include <iostream>
#include <numeric>
#include <functional>
#include <unistd.h>

#include <TFile.h>
#include <TGraph.h>
#include <TH1.h>

#include "exceptions.h"
#include "toolbox.h"
#include "Options.h"

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>

#include "TString.h"
#include "TColor.h"

#include "Alignment/OfflineValidation/interface/PreparePVTrends.h"
#include "Alignment/OfflineValidation/interface/Trend.h"

using namespace std;
using namespace AllInOneConfig;

static const char * bold = "\e[1m", * normal = "\e[0m";

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
  pt::ptree lines = main_tree.get_child("lines");
  
  //Read all configure variables and set default for missing keys
  string outputdir = main_tree.get<string>("output");
  bool doRMS = validation.get_child_optional("doRMS") ? validation.get<bool>("doRMS") : true;
  TString lumiInputFile = validation.get_child_optional("lumiInputFile") ? validation.get<string>("lumiInputFile") : "lumiperFullRun2_delivered.txt";

  TString LumiFile = getenv("CMSSW_BASE");
  if (lumiInputFile.BeginsWith("/"))
    LumiFile = lumiInputFile;
  else {
    LumiFile += "/src/Alignment/OfflineValidation/data/";
    LumiFile += lumiInputFile;
  }
  fs::path pathToLumiFile = LumiFile.Data();
  if (!fs::exists(pathToLumiFile)) {
    cout << "ERROR: lumi-per-run file (" << LumiFile.Data() << ") not found!" << endl << "Please check!" << endl;
    exit(EXIT_FAILURE);
  }

  vector<string> alignment_dirs;
  vector<string> alignment_titles;
  vector<string> alignment_files;
  for (const pair<string, pt::ptree>& childTree : alignments) {
    fs::path alignment_dir = childTree.first.c_str();
    fs::create_directory(alignment_dir);
    alignment_dirs.push_back(childTree.first.c_str());
    alignment_titles.push_back(childTree.second.get<string>("title"));
    alignment_files.push_back(childTree.second.get<string>("file"));
  }

  for(size_t i=0; i<alignment_dirs.size(); i++) {
    for (const pair<string, pt::ptree>& childTree : validation.get_child("IOV")) {
      int iov = childTree.second.get_value<int>();
      TString file = alignment_files[i];
      fs::path target_dir = Form("%s/PVValidation_%s_%d.root", file.ReplaceAll("{}", to_string(iov)).Data(), alignment_dirs[i].data(), iov);
      fs::path output_dir = Form("./%s/PVValidation_%s_%d.root",alignment_dirs[i].data(), alignment_dirs[i].data(), iov);
      if(!fs::exists(output_dir))
	fs::create_symlink(target_dir, output_dir);
    }
  }

  PreparePVTrends prepareTrends(outputdir, alignment_dirs, alignment_titles);
  prepareTrends.MultiRunPVValidation(doRMS, LumiFile);

  int firstRun = validation.get_child_optional("firstRun") ? validation.get<int>("firstRun") : 272930;
  int lastRun = validation.get_child_optional("lastRun") ? validation.get<int>("lastRun") : 325175;
  
  const Run2Lumi GetLumi(LumiFile.Data(), firstRun, lastRun);
  
  fs::path pname = Form("%s/PVtrends.root", outputdir.data());
  assert(fs::exists(pname));
  
  vector<string> variables{"dxy_phi_vs_run",
                            "dxy_eta_vs_run",
                            "dz_phi_vs_run",
                            "dz_eta_vs_run"};
  
  
  vector<string> titles{"of impact parameter in transverse plane as a function of azimuth angle",
  		       "of impact parameter in transverse plane as a function of pseudorapidity",
                       "of impact parameter along z-axis as a function of azimuth angle",
                       "of impact parameter along z-axis as a function of pseudorapidity"};
  
  vector<string> ytitles{"of d_{xy}(#phi)   [#mum]",
                        "of d_{xy}(#eta)   [#mum]",
                        "of d_{z}(#phi)   [#mum]",
                        "of d_{z}(#eta)   [#mum]"};
  
  auto f = TFile::Open(pname.c_str());
  for(size_t i=0; i<variables.size(); i++) {
  
    Trend mean(Form("mean_%s", variables[i].data()), outputdir.data(), Form("mean %s", titles[i].data()),
  	       Form("mean %s", ytitles[i].data()), -7., 10., lines, GetLumi), 
          RMS (Form("RMS_%s", variables[i].data()), outputdir.data(), Form("RMS %s", titles[i].data()), 
  	       Form("RMS %s", ytitles[i].data()), 0., 35., lines, GetLumi);
    mean.lgd.SetHeader("p_{T} (track) > 3 GeV");
    RMS .lgd.SetHeader("p_{T} (track) > 3 GeV");
  
    for (const pair<string, pt::ptree>& childTree : alignments) {
      TString gname = childTree.second.get<string>("title");
      gname.ReplaceAll(" ", "_");
    
      auto gMean = Get<TGraph>("mean_%s_%s", gname.Data(), variables[i].data());
      auto hRMS  = Get<TH1   >( "RMS_%s_%s", gname.Data(), variables[i].data());
      assert(gMean != nullptr);
      assert(hRMS  != nullptr);
      
      TString gtitle = childTree.second.get<string>("title");
      gMean->SetTitle(gtitle); // for the legend
      gMean->SetMarkerSize(0.7);
      int color = childTree.second.get<int>("color");
      int style = childTree.second.get<int>("style");
      gMean->SetMarkerColor(color);
      gMean->SetMarkerStyle(style);
  
      hRMS ->SetTitle(gtitle); // for the legend
      hRMS ->SetMarkerSize(0.7);
      hRMS ->SetMarkerColor(color);
      hRMS ->SetMarkerStyle(style);
    
      bool fullRange = gname.Contains("data-taking") || gname.Contains("Legacy");
      mean(gMean, "PZ", "p", fullRange);
      RMS (hRMS , ""  , "p", fullRange);
    }
  }
  
  f->Close();
  cout << bold << "Done" << normal << endl;
  
  return EXIT_SUCCESS;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
int main(int argc, char* argv[]) { return exceptions<trends>(argc, argv); }
#endif
