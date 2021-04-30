
#ifndef PLOT_TRENDS_H 
#define PLOT_TRENDS_H

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <iomanip>
#include <fstream>
#include <experimental/filesystem>
#include "TPad.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TMultiGraph.h"
#include "TH1.h"
#include "THStack.h"
#include "TROOT.h"
#include "TFile.h"
#include "TColor.h"
#include "TLegend.h"
#include "TLegendEntry.h"
#include "TMath.h"
#include "TRegexp.h"
#include "TPaveLabel.h"
#include "TPaveText.h"
#include "TStyle.h"
#include "TLine.h"
#include "Alignment/OfflineValidation/plugins/ColorParser.C"

/*!
 * \def Dummy value in case a DMR would fail for instance
 */
#define DUMMY -999.
/*!
 * \def Scale factor value to have luminosity expressed in fb^-1
 */
#define lumiFactor 1000.

/*! \struct Point
 *  \brief Structure Point
 *         Contains parameters of Gaussian fits to DMRs
 *  
 * @param run:             run number (IOV boundary)
 * @param scale:           scale for the measured quantity: cm->μm for DMRs, 1 for normalized residuals
 * @param mu:              mu/mean from Gaussian fit to DMR/DrmsNR
 * @param sigma:           sigma/standard deviation from Gaussian fit to DMR/DrmsNR
 * @param muplus:          mu/mean for the inward pointing modules
 * @param muminus:         mu/mean for outward pointing modules
 * @param sigmaplus:       sigma/standard for inward pointing modules 
 * @param sigmaminus: //!< sigma/standard for outward pointing modules
 */

///**************************
///*  Function declaration  *
///**************************

class PlotTrends {
 private:
  std::vector<TString> variables_;
  std::vector<std::string> YaxisNames_;
  TString PlotType_;
 public:
  PlotTrends(vector<TString> variables, std::vector<std::string> YaxisNames, TString PlotType);
  void setVariables(vector<TString> variables, std::vector<std::string> YaxisNames);
  TString getName(TString structure, int layer, TString geometry);
  TH1F *ConvertToHist(TGraphErrors *g);
  std::vector<int> runlistfromlumifile(TString lumifile =
				  "/afs/cern.ch/work/a/acardini/Alignment/MultiIOV/CMSSW_10_5_0_pre2/src/Alignment/"
                                    "OfflineValidation/data/lumiperFullRun2.txt");
  bool checkrunlist(std::vector<int> runs, std::vector<int> IOVlist = {});
  TString lumifileperyear(TString Year = "2018", std::string RunOrIOV = "IOV");
  void scalebylumi(TGraphErrors *g, std::vector<pair<int, double>> lumiIOVpairs);
  std::vector<pair<int, double>> lumiperIOV(std::vector<int> IOVlist, TString Year = "2018");
  double getintegratedlumiuptorun(int run, TString lumifile, double min = 0.);
  void PixelUpdateLines(TCanvas *c,
			TString lumifile =
                          "/afs/cern.ch/work/a/acardini/Alignment/MultiIOV/CMSSW_10_5_0_pre2/src/Alignment/"
			"OfflineValidation/data/lumiperFullRun2.txt",
			bool showlumi = false,
			std::vector<int> pixelupdateruns = {314881, 316758, 317527, 318228, 320377});
  void PlotDMRTrends(
		     std::vector<int> IOVlist,
		     TString Variable = "median",
		     std::vector<std::string> labels = {"MB"},
		     TString Year = "2018",
		     std::string myValidation = "/afs/cern.ch/cms/CAF/CMSALCA/ALCA_TRACKERALIGN/data/commonValidation/results/acardini/DMRs/",
		     std::vector<std::string> geometries = {"GT", "SG", "MP pix LBL", "PIX HLS+ML STR fix"},
		     std::vector<Color_t> colours = {kBlue, kRed, kGreen, kCyan},
		     TString outputdir =
		     "/afs/cern.ch/cms/CAF/CMSALCA/ALCA_TRACKERALIGN/data/commonValidation/alignmentObjects/acardini/DMRsTrends/",
		     bool pixelupdate = false,
		     std::vector<int> pixelupdateruns = {314881, 316758, 317527, 318228, 320377},
		     bool showlumi = false,
		     TString lumifile =
		     "/afs/cern.ch/work/a/acardini/Alignment/MultiIOV/CMSSW_10_5_0_pre2/src/Alignment/OfflineValidation/data/"
		     "lumiperFullRun2.txt",
		     std::vector<std::pair<int, double>> lumiIOVpairs = {std::make_pair(0, 0.), std::make_pair(0, 0.)},
		     TString structure  = "",
		     int layer = 0);
};

#endif
