#ifndef OUTTRENDS_H_
#define OUTTRENDS_H_

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
 * \def basically the y-values of a TGraph
 */
typedef std::map<TString, std::vector<double> > alignmentTrend;


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

#if defined(__ROOTCLING__)
#pragma link C++ class std::map<TString, std::vector<double> > +;
#pragma link C++ class outTrends +;

#endif

#endif  // OUTTRENDS_H_
