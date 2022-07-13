#ifndef PREPAREPVTRENDS_H_
#define PREPAREPVTRENDS_H_

#include "TArrow.h"
#include "TAxis.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TFile.h"
#include "TGaxis.h"
#include "TGraph.h"
#include "TGraphAsymmErrors.h"
#include "TGraphErrors.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TList.h"
#include "TMath.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "TPad.h"
#include "TPaveText.h"
#include "TProcPool.h"
#include "TProfile.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TSystemDirectory.h"
#include "TSystemFile.h"
#include <TStopwatch.h>
#include <algorithm>
#include <bitset>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <vector>

/*!
 * \def some convenient I/O
 */
#define logInfo std::cout << "INFO: "
#define logWarning std::cout << "WARNING: "
#define logError std::cout << "ERROR!!! "

/*!
 * \def boolean to decide if it is in debug mode
 */
#define VERBOSE false

/*!
 * \def basically the y-values of a TGraph
 */
typedef std::map<TString, std::vector<double> > alignmentTrend;

/*!
 * \def number of workers
 */
const size_t nWorkers = 20;

namespace pv {
  enum view { dxyphi, dzphi, dxyeta, dzeta, pT, generic };

  /*! \fn closest
   *  \brief method to find first value that doesn not compare left
   */

  int closest(std::vector<int> const &vec, int value) {
    auto const it = std::lower_bound(vec.begin(), vec.end(), value);
    if (it == vec.end()) {
      return -1;
    }
    return *it;
  }

  const Int_t markers[8] = {kFullSquare,
                            kFullCircle,
                            kFullTriangleDown,
                            kOpenSquare,
                            kOpenCircle,
                            kFullTriangleUp,
                            kOpenTriangleDown,
                            kOpenTriangleUp};
  const Int_t colors[8] = {kBlack, kRed, kBlue, kGreen + 2, kMagenta, kViolet, kCyan, kYellow};

  /*! \struct biases
   *  \brief Structure biases
   *         Contains characterization of a single run PV bias plot
   *
   * @param m_mean:             mean value of the profile points
   * @param m_rms:              RMS value of the profle points
   * @param m_w_mean:           mean weighted on the errors
   * @param m_w_rms:            RMS weighted on the errors
   * @param m_min:              minimum of the profile
   * @param m_max:              maximum of the profile
   * @param m_chi2:             chi2 of a liner fit
   * @param m_ndf:              number of the dof of the linear fit
   * @param m_ks:               Kolmogorov-Smirnov score of comparison with flat line
   */

  struct biases {
    // contructor
    biases(double mean, double rms, double wmean, double wrms, double min, double max, double chi2, int ndf, double ks) {
      m_mean = mean;
      m_rms = rms;
      m_w_mean = wmean;
      m_w_rms = wrms;
      m_min = min;
      m_max = max;
      m_chi2 = chi2;
      m_ndf = ndf;
      m_ks = ks;
    }

    // empty constructor
    biases() { init(); }

    /*! \fn init
     *  \brief initialising all members one by one
     */

    void init() {
      m_mean = 0;
      m_rms = 0.;
      m_min = +999.;
      m_max = -999.;
      m_w_mean = 0.;
      m_w_rms = 0.;
      m_chi2 = -1.;
      m_ndf = 0.;
      m_ks = 9999.;
    }

    double getMean() { return m_mean; }
    double getWeightedMean() { return m_w_mean; }
    double getRMS() { return m_rms; }
    double getWeightedRMS() { return m_w_rms; }
    double getMin() { return m_min; }
    double getMax() { return m_max; }
    double getChi2() { return m_chi2; }
    double getNDF() { return m_ndf; }
    double getNormChi2() { return double(m_chi2) / double(m_ndf); }
    double getChi2Prob() { return TMath::Prob(m_chi2, m_ndf); }
    double getKSScore() { return m_ks; }

  private:
    double m_mean;
    double m_min;
    double m_max;
    double m_rms;
    double m_w_mean;
    double m_w_rms;
    double m_chi2;
    int m_ndf;
    double m_ks;
  };

  /*! \struct wrappedTrends
   *  \brief Structure wrappedTrends
   *         Contains the ensemble vs run number of the alignmentTrend characterization
   *
   * @param mean:             alignmentTrend of the mean value of the profile points
   * @param low:              alignmentTrend of the lowest value of the profle points
   * @param high:             alignmentTrend of the highest value of the profile points
   * @param lowerr:           alignmentTrend of the difference between the lowest value and the mean of the profile points
   * @param higherr:          alignmentTrend of the difference between the highest value and the mean of the profile points
   * @param chi2:             alignmentTrend of the chi2 value of a linear fit to the profile points
   * @param KS:               alignmentTrend of the Kolmogorow-Smirnov score of the comarison of the profile points to a flat line
   */

  struct wrappedTrends {
    /*! \fn wrappedTrends
     *  \brief Constructor of structure wrappedTrends, initialising all members from DMRs directly (with split)
     */

    wrappedTrends(alignmentTrend mean,
                  alignmentTrend low,
                  alignmentTrend high,
                  alignmentTrend lowerr,
                  alignmentTrend higherr,
                  alignmentTrend chi2,
                  alignmentTrend KS) {
      logInfo << "pv::wrappedTrends c'tor" << std::endl;

      m_mean = mean;
      m_low = low;
      m_high = high;
      m_lowerr = lowerr;
      m_higherr = higherr;
      m_chi2 = chi2;
      m_KS = KS;
    }

    alignmentTrend getMean() const { return m_mean; }
    alignmentTrend getLow() const { return m_low; }
    alignmentTrend getHigh() const { return m_high; }
    alignmentTrend getLowErr() const { return m_lowerr; }
    alignmentTrend getHighErr() const { return m_higherr; }
    alignmentTrend getChi2() const { return m_chi2; }
    alignmentTrend getKS() const { return m_KS; }

  private:
    alignmentTrend m_mean;
    alignmentTrend m_low;
    alignmentTrend m_high;
    alignmentTrend m_lowerr;
    alignmentTrend m_higherr;
    alignmentTrend m_chi2;
    alignmentTrend m_KS;
  };

  /*! \struct bundle
   *  \brief Structure bundle
   *         Contains the ensemble of all the information to build the graphs alignmentTrends
   *
   * @param nObjects                     int, number of alignments to be considered
   * @param dataType                     TString, type of the data to be displayed (time, lumi)
   * @param dataTypeLabel                TString, x-axis label
   */

  struct bundle {
    bundle(int nObjects,
           const TString &dataType,
           const TString &dataTypeLabel,
           const bool &useRMS) {
      m_nObjects = nObjects;
      m_datatype = dataType.Data();
      m_datatypelabel = dataTypeLabel.Data();
      m_useRMS = useRMS;

      logInfo << "pv::bundle c'tor: " << dataTypeLabel << " member: " << m_datatypelabel << std::endl;

      logInfo << m_axis_types << std::endl;

    }

    int getNObjects() const { return m_nObjects; }
    const char *getDataType() const { return m_datatype; }
    const char *getDataTypeLabel() const { return m_datatypelabel; }
    bool isUsingRMS() const { return m_useRMS; }
    void printAll() {
      logInfo << "dataType      " << m_datatype << std::endl;
      logInfo << "dataTypeLabel " << m_datatypelabel << std::endl;
    }

  private:
    int m_nObjects;
    const char *m_datatype;
    const char *m_datatypelabel;
    std::bitset<2> m_axis_types;
    bool m_useRMS;
  };

}  // namespace pv


struct outTrends {

  /*! \struct outTrends
  *  \brief Structure outTrends
  *         Contains the ensemble of all the alignmentTrends built by the functor
  *
  * @param m_index                     int, to keep track of which chunk of data has been processed
  * @param m_runs                      std::vector, list of the run processed in this section
  * @param m_dxyPhiMeans               alignmentTrend of the mean values of the profile dxy vs phi
  * @param m_dxyPhiChi2                alignmentTrend of chi2 of the linear fit per profile dxy vs phi
  * @param m_dxyPhiKS                  alignmentTrend of Kolmogorow-Smirnov score of comparison of dxy vs phi profile with flat line
  * @param m_dxyPhiHi                  alignmentTrend of the highest value of the profile dxy vs phi
  * @param m_dxyPhiLo                  alignmentTrend of the lowest value of the profile dxy vs phi
  * @param m_dxyEtaMeans               alignmentTrend of the mean values of the profile dxy vs eta
  * @param m_dxyEtaChi2                alignmentTrend of chi2 of the linear fit per profile dxy vs eta
  * @param m_dxyEtaKS                  alignmentTrend of Kolmogorow-Smirnov score of comparison of dxy vs eta profile with flat line
  * @param m_dxyEtaHi                  alignmentTrend of the highest value of the profile dxy vs eta
  * @param m_dxyEtaLo                  alignmentTrend of the lowest value of the profile dxy vs eta
  * @param m_dzPhiMeans                alignmentTrend of the mean values of the profile dz vs phi
  * @param m_dzPhiChi2                 alignmentTrend of chi2 of the linear fit per profile dz vs phi
  * @param m_dzPhiKS                   alignmentTrend of Kolmogorow-Smirnov score of comparison of dz vs phi profile with flat line
  * @param m_dzPhiHi                   alignmentTrend of the highest value of the profile dz vs phi
  * @param m_dzPhiLo                   alignmentTrend of the lowest value of the profile dz vs phi
  * @param m_dzEtaMeans                alignmentTrend of the mean values of the profile dz vs eta
  * @param m_dzEtaChi2                 alignmentTrend of chi2 of the linear fit per profile dz vs eta
  * @param m_dzEtaKS                   alignmentTrend of Kolmogorow-Smirnov score of comparison of dz vs eta profile with flat line
  * @param m_dzEtaHi                   alignmentTrend of the highest value of the profile dz vs eta
  * @param m_dzEtaLo                   alignmentTrend of the lowest value of the profile dz vs eta
  */
  // empty constructor

  outTrends() { init(); }

  int m_index;
  std::vector<double> m_runs;
  alignmentTrend m_dxyPhiMeans;
  alignmentTrend m_dxyPhiChi2;
  alignmentTrend m_dxyPhiKS;
  alignmentTrend m_dxyPhiHi;
  alignmentTrend m_dxyPhiLo;
  alignmentTrend m_dxyEtaMeans;
  alignmentTrend m_dxyEtaChi2;
  alignmentTrend m_dxyEtaKS;
  alignmentTrend m_dxyEtaHi;
  alignmentTrend m_dxyEtaLo;
  alignmentTrend m_dzPhiMeans;
  alignmentTrend m_dzPhiChi2;
  alignmentTrend m_dzPhiKS;
  alignmentTrend m_dzPhiHi;
  alignmentTrend m_dzPhiLo;
  alignmentTrend m_dzEtaMeans;
  alignmentTrend m_dzEtaChi2;
  alignmentTrend m_dzEtaKS;
  alignmentTrend m_dzEtaHi;
  alignmentTrend m_dzEtaLo;
  
  void init() {
    m_index = -1;
    m_runs.clear();
    
    m_dxyPhiMeans.clear();
    m_dxyPhiChi2.clear();
    m_dxyPhiKS.clear();
    m_dxyPhiHi.clear();
    m_dxyPhiLo.clear();
    
    m_dxyEtaMeans.clear();
    m_dxyEtaChi2.clear();
    m_dxyEtaKS.clear();
    m_dxyEtaHi.clear();
    m_dxyEtaLo.clear();
    
    m_dzPhiMeans.clear();
    m_dzPhiChi2.clear();
    m_dzPhiKS.clear();
    m_dzPhiHi.clear();
    m_dzPhiLo.clear();
    
    m_dzEtaMeans.clear();
    m_dzEtaChi2.clear();
    m_dzEtaKS.clear();
    m_dzEtaHi.clear();
    m_dzEtaLo.clear();
   }
};


class PreparePVTrends {
 public:

  void MultiRunPVValidation(TString namesandlabels = "",
                          bool useRMS = true,
                          TString lumiInputFile = "");

  static pv::biases getBiases(TH1F *hist);
  static TH1F *DrawConstantWithErr(TH1F *hist, Int_t iter, Double_t theConst);
  static outTrends processData(size_t iter,
				   std::vector<int> intersection,
				   const Int_t nDirs_,
				   const char *dirs[10],
				   TString LegLabels[10],
				   bool useRMS);
  std::vector<int> list_files(const char *dirname = ".", const char *ext = ".root");
  void outputGraphs(const pv::wrappedTrends &allInputs,
                  const std::vector<double> &ticks,
                  const std::vector<double> &ex_ticks,
                  TGraphErrors *&g_mean,
                  TGraphErrors *&g_chi2,
                  TGraphErrors *&g_KS,
                  TGraphErrors *&g_low,
                  TGraphErrors *&g_high,
                  TGraphAsymmErrors *&g_asym,
                  const pv::bundle &mybundle,
                  const pv::view &theView,
                  const TString &label);
  
};

#endif  // PREPAREPVTRENDS_H_
