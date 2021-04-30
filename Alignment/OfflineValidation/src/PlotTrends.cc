#include "Alignment/OfflineValidation/interface/PlotTrends.h"

using namespace std;
namespace fs = std::experimental::filesystem;

PlotTrends::PlotTrends(vector<TString> variables, vector<string> YaxisNames, TString PlotType)
{
  PlotType_ = PlotType;
  setVariables(variables, YaxisNames);
}

void PlotTrends::setVariables(vector<TString> variables, vector<string> YaxisNames)
{
  if (variables.size() != YaxisNames.size()) {
    cout << "ERROR: variables and YaxisNames need to correspond and have the same length" << endl;
    exit(EXIT_FAILURE);
  }
  variables_.clear();
  YaxisNames_.clear();
  for (size_t i = 0; i < variables.size(); i++) {
    variables_.push_back(variables[i]);
    YaxisNames_.push_back(YaxisNames[i]);
  }
}

TString PlotTrends::getName(TString structure, int layer, TString geometry) {
  geometry.ReplaceAll(" ", "_");
  TString name = geometry;
  if(structure != "")
    name += "_structure";
  if (layer != 0) {
    if (structure == "TID" || structure == "TEC")
      name += "_disc";
    else
      name += "_layer";
    name += layer;
  }

  return name;
};

/// DEPRECATED
/*! \fn lumifileperyear
 *  \brief Function to retrieve the file with luminosity per run/IOV
 *         The use of a lumi-per-IOV file is deprecated, but can still be useful for debugging
 */

TString PlotTrends::lumifileperyear(TString Year, string RunOrIOV) {
  TString LumiFile = std::getenv("CMSSW_BASE");
  LumiFile += "/src/Alignment/OfflineValidation/data/lumiper";
  if (RunOrIOV != "run" && RunOrIOV != "IOV") {
    cout << "ERROR: Please specify \"run\" or \"IOV\" to retrieve the luminosity run by run or for each IOV" << endl;
    exit(EXIT_FAILURE);
  }
  LumiFile += RunOrIOV;
  if (Year != "2016" && Year != "2017" && Year != "2018") {
    cout << "ERROR: Only 2016, 2017 and 2018 lumi-per-run files are available, please check!" << endl;
    exit(EXIT_FAILURE);
  }
  LumiFile += Year;
  LumiFile += ".txt";
  return LumiFile;
};

/*! \fn runlistfromlumifile
 *  \brief Get a vector containing the list of runs for which the luminosity is known.
 */

vector<int> PlotTrends::runlistfromlumifile(TString lumifile) {
  vector<pair<int, double>> lumiperRun;

  std::ifstream infile(lumifile.Data());
  int run;
  double runLumi;
  while (infile >> run >> runLumi) {
    lumiperRun.push_back(make_pair(run, runLumi));
  }
  size_t nRuns = lumiperRun.size();
  vector<int> xRunFromLumiFile;
  for (size_t iRun = 0; iRun < nRuns; iRun++)
    xRunFromLumiFile.push_back(lumiperRun[iRun].first);
  return xRunFromLumiFile;
}

/*! \fn checkrunlist
 *  \brief Check whether all runs of interest are present in the luminosity per run txt file and whether all IOVs analized have been correctly processed
 */

bool PlotTrends::checkrunlist(vector<int> runs, vector<int> IOVlist) {
  vector<int> missingruns;  //runs for which the luminosity is not found
  vector<int> lostruns;     //IOVs for which the DMR were not found
  bool problemfound = false;
  std::sort(runs.begin(), runs.end());
  std::sort(IOVlist.begin(), IOVlist.end());
  if (!IOVlist.empty())
    for (auto IOV : IOVlist) {
      if (find(runs.begin(), runs.end(), IOV) == runs.end()) {
        problemfound = true;
        lostruns.push_back(IOV);
      }
    }
  if (problemfound) {
    if (!lostruns.empty()) {
      cout << "WARNING: some IOVs where not found among the list of available DMRs" << endl
           << "List of missing IOVs:" << endl;
      for (int lostrun : lostruns)
        cout << to_string(lostrun) << " ";
      cout << endl;
    }
  }
  return problemfound;
}

/*! \fn PixelUpdateLines
 *  \brief  Adds to the canvas vertical lines corresponding to the pixelupdateruns
 */
void PlotTrends::PixelUpdateLines(TCanvas *c, TString lumifile, bool showlumi, vector<int> pixelupdateruns) {
  vector<TPaveText *> labels;
  double lastlumi = 0.;
  c->cd();
  size_t index = 0;
  //Due to the way the coordinates within the Canvas are set, the following steps are required to draw the TPaveText:
  // Compute the gPad coordinates in TRUE normalized space (NDC)
  int ix1;
  int ix2;
  int iw = gPad->GetWw();
  int ih = gPad->GetWh();
  double x1p, y1p, x2p, y2p;
  gPad->GetPadPar(x1p, y1p, x2p, y2p);
  ix1 = (Int_t)(iw * x1p);
  ix2 = (Int_t)(iw * x2p);
  double wndc = TMath::Min(1., (double)iw / (double)ih);
  double rw = wndc / (double)iw;
  double x1ndc = (double)ix1 * rw;
  double x2ndc = (double)ix2 * rw;
  // Ratios to convert user space in TRUE normalized space (NDC)
  double rx1, ry1, rx2, ry2;
  gPad->GetRange(rx1, ry1, rx2, ry2);
  double rx = (x2ndc - x1ndc) / (rx2 - rx1);
  int ola = 0;

  for (int pixelupdaterun : pixelupdateruns) {
    double lumi = 0.;

    char YearsNames[5][5] = {"2016", "2017", "2018"};

    if (showlumi)
      lumi = getintegratedlumiuptorun(
          pixelupdaterun,
          lumifile);  //The vertical line needs to be drawn at the beginning of the run where the pixel update was implemented, thus only the integrated luminosity up to that run is required.
    else
      lumi = pixelupdaterun;

    // here to plot with one style runs from which pixel iov was otained

    //  below to plot with one style pixel uptade runs

    TLine *line = new TLine(lumi, c->GetUymin(), lumi, c->GetUymax());
    if (pixelupdaterun == 271866 || pixelupdaterun == 272008 || pixelupdaterun == 276315 || pixelupdaterun == 278271 ||
        pixelupdaterun == 280928 || pixelupdaterun == 290543 || pixelupdaterun == 294927 || pixelupdaterun == 297281 ||
        pixelupdaterun == 298653 || pixelupdaterun == 299443 || pixelupdaterun == 300389 || pixelupdaterun == 301046 ||
        pixelupdaterun == 302131 || pixelupdaterun == 303790 || pixelupdaterun == 303998 || pixelupdaterun == 304911 ||
        pixelupdaterun == 313041 || pixelupdaterun == 314881 || pixelupdaterun == 315257 || pixelupdaterun == 316758 ||
        pixelupdaterun == 317475 || pixelupdaterun == 317485 || pixelupdaterun == 317527 || pixelupdaterun == 317661 ||
        pixelupdaterun == 317664 || pixelupdaterun == 318227 || pixelupdaterun == 320377 || pixelupdaterun == 321831 ||
        pixelupdaterun == 322510 || pixelupdaterun == 322603 || pixelupdaterun == 323232 || pixelupdaterun == 324245) {
      line->SetLineColor(kBlack);
      line->SetLineStyle(3);  // it was 9, For public plots changed to 3
      line->Draw();
    }

    //  324245 those iovs are in both lists so I need to drwa both on top of each other
    //the problem is around these lines: the IOVs so small that they overlap
    //298653, 299061, 299443,
    //318887, 320377, 320674,

    double _sx;
    // Left limit of the TPaveText

    _sx = rx * (lumi - rx1) + x1ndc;
    // To avoid an overlap between the TPaveText a vertical shift is done when the IOVs are too close
    if (_sx < lastlumi) {
      //    index++;     // ola: I commented it out becaouse if I plot only the names of years I dint need to change the position of the label
    } else
      index = 0;

    // FirstRunOftheYear 272008,294927,315257
    //// first run of 2018 and run from which pixel template was obtained 314527 is very close
    // also first run of 2016 and 272022 are very close

    if (pixelupdaterun == 272008 || pixelupdaterun == 294927 || pixelupdaterun == 315257 || pixelupdaterun == 314527 ||
        pixelupdaterun == 272022) {
      TPaveText *box = new TPaveText(_sx + 0.0028, 0.865 - index * 0.035, _sx + 0.055, 0.895 - index * 0.035, "blNDC");
      if (pixelupdaterun == 272008 || pixelupdaterun == 294927 || pixelupdaterun == 315257) {
        box->SetFillColor(10);
        box->SetBorderSize(1);
        box->SetLineColor(kBlack);

        TText *textFirstRunOfYear = box->AddText(Form("%s", YearsNames[int(ola)]));  // Ola for public plots
        //  TText *textFirstRunOfYear = box->AddText(Form("%i", int(pixelupdaterun)));
        textFirstRunOfYear->SetTextSize(0.025);
        labels.push_back(box);
        ola = ola + 1;
      }

      if (pixelupdaterun == 294927) {
        line->SetLineColor(kBlack);
        line->SetLineStyle(1);
        line->SetLineWidth(4);
        line->Draw();
      }

      if (pixelupdaterun == 315257 || pixelupdaterun == 314527) {
        //if (pixelupdaterun == 315257 ){
        line->SetLineColor(kBlack);
        line->SetLineStyle(1);
        line->SetLineWidth(4);
        line->Draw();
        //TLine *line2 = new TLine(lumi, c->GetUymin(), lumi, c->GetUymax());

        //// line2->SetLineColor(kYellow+1);  //it is problematic because it should be the same and it is not
        //line2->SetLineColor(kRed + 1);
        //line2->SetLineStyle(1);  //
        //line2->Draw();
      }

      // the yellow line has to be thicker here to be not exactly at 0 and visible from below the y axis...

      if (pixelupdaterun == 272008 || pixelupdaterun == 272022) {
        line->SetLineColor(kBlack);
        line->SetLineStyle(1);
        line->SetLineWidth(4);
        // line->Draw();
        //TLine *line2 = new TLine(lumi, c->GetUymin(), lumi, c->GetUymax());

        //line2->SetLineColor(kYellow + 1);
        //line2->SetLineStyle(1);  //
        //line->SetLineWidth(10);
        //line2->Draw();
      }
    }

    //TLine *line11 = new TLine(lumi, c->GetUymin(), lumi, c->GetUymax());
    //if (pixelupdaterun == 272022 || pixelupdaterun == 276811 || pixelupdaterun == 279767 || pixelupdaterun == 283308 ||
    //    pixelupdaterun == 297057 || pixelupdaterun == 297503 || pixelupdaterun == 299061 || pixelupdaterun == 300157 ||
    //    pixelupdaterun == 300401 || pixelupdaterun == 301183 || pixelupdaterun == 302472 || pixelupdaterun == 303885 ||
    //    pixelupdaterun == 304292 || pixelupdaterun == 305898 || pixelupdaterun == 314527 || pixelupdaterun == 316766 ||
    //    pixelupdaterun == 317484 || pixelupdaterun == 317641 || pixelupdaterun == 318887 || pixelupdaterun == 320674 ||
    //    pixelupdaterun == 321833 || pixelupdaterun == 324245) {
    //  //315257
    //
    //  line11->SetLineColor(kYellow + 1);
    //  line11->SetLineStyle(1);  //
    //  line11->Draw();
    //}

    if (pixelupdaterun == 298653 || pixelupdaterun == 299061 || pixelupdaterun == 320377 || pixelupdaterun == 320674 ||
        pixelupdaterun == 324245) {
      //TLine *line2 = new TLine(lumi, c->GetUymin(), lumi, c->GetUymax());
      //
      //line2->SetLineColor(kYellow + 1);
      //line2->SetLineStyle(1);  //
      //line2->Draw();

      line->SetLineColor(kBlack);
      line->SetLineStyle(3);  // it was 9, For public plots changed to 3
      line->Draw();
    }

    //TLine *line33 = new TLine(lumi, c->GetUymin(), lumi, c->GetUymax());
    ////if (pixelupdaterun == 298653 || pixelupdaterun ==  299061 || pixelupdaterun ==  320377 || pixelupdaterun ==  320674){
    //if (pixelupdaterun == 299061 || pixelupdaterun == 301183 || pixelupdaterun == 304292 || pixelupdaterun == 314527 ||
    //    pixelupdaterun == 317484 || pixelupdaterun == 317641 || pixelupdaterun == 320674 || pixelupdaterun == 321833) {
    //  line33->SetLineColor(kRed + 1);
    //  line33->SetLineWidth(1);
    //  line33->SetLineStyle(1);
    //  line33->Draw();
    //
    //  if (pixelupdaterun == 299061 || pixelupdaterun == 317484 || pixelupdaterun == 320674) {
    //    TLine *line333 = new TLine(lumi, c->GetUymin(), lumi, c->GetUymax());
    //    line333->SetLineColor(kBlack);
    //    line333->SetLineWidth(1);
    //    line333->SetLineStyle(3);
    //    line333->Draw();
    //  }
    //}

    lastlumi = _sx + 0.075;

    //  gPad->RedrawAxis();
  }
  //Drawing in a separate loop to ensure that the labels are drawn on top of the lines
  for (auto label : labels) {
    label->Draw("same");
  }
  c->Update();
}

/*! \fn getintegratedlumiuptorun
 *  \brief Returns the integrated luminosity up to the run of interest
 *         Use -1 to get integrated luminosity over the whole period
 */

double PlotTrends::getintegratedlumiuptorun(int ChosenRun, TString lumifile, double min) {
  double lumi = min;
  vector<pair<int, double>> lumiperRun;

  std::ifstream infile(lumifile.Data());
  int run;
  double runLumi;
  while (infile >> run >> runLumi) {
    lumiperRun.push_back(make_pair(run, runLumi));
  }
  std::sort(lumiperRun.begin(), lumiperRun.end());
  for (size_t iRun = 0; iRun < lumiperRun.size(); iRun++) {
    if (ChosenRun <= lumiperRun.at(iRun).first)
      break;
    lumi += (lumiperRun.at(iRun).second / lumiFactor);
  }
  return lumi;
}
/*! \fn scalebylumi
 *  \brief Scale X-axis of the TGraph and the error on that axis according to the integrated luminosity.
 */

void PlotTrends::scalebylumi(TGraphErrors *g, vector<pair<int, double>> lumiIOVpairs) {
  size_t N = g->GetN();
  vector<double> x, y, xerr, yerr;

  //TGraph * scale = new TGraph((lumifileperyear(Year,"IOV")).Data());
  size_t Nscale = lumiIOVpairs.size();

  size_t i = 0;
  while (i < N) {
    double run, yvalue;
    g->GetPoint(i, run, yvalue);
    size_t index = -1;
    for (size_t j = 0; j < Nscale; j++) {
      if (run == (lumiIOVpairs.at(j)
                      .first)) {  //If the starting run of an IOV is included in the list of IOVs, the index is stored
        index = j;
        continue;
      } else if (run > (lumiIOVpairs.at(j).first))
        continue;
    }
    if (lumiIOVpairs.at(index).second == 0 || index < 0.) {
      N = N - 1;
      g->RemovePoint(i);
    } else {
      double xvalue = 0.;
      for (size_t j = 0; j < index; j++)
        xvalue += lumiIOVpairs.at(j).second / lumiFactor;
      x.push_back(xvalue + (lumiIOVpairs.at(index).second / (lumiFactor * 2.)));
      if (yvalue <= DUMMY) {
        y.push_back(DUMMY);
        yerr.push_back(0.);
      } else {
        y.push_back(yvalue);
        yerr.push_back(g->GetErrorY(i));
      }
      xerr.push_back(lumiIOVpairs.at(index).second / (lumiFactor * 2.));
      i = i + 1;
    }
  }
  g->GetHistogram()->Delete();
  g->SetHistogram(nullptr);
  for (size_t i = 0; i < N; i++) {
    g->SetPoint(i, x.at(i), y.at(i));
    g->SetPointError(i, xerr.at(i), yerr.at(i));
  }
}

/*! \fn lumiperIOV
 *  \brief Retrieve luminosity per IOV
 */

vector<pair<int, double>> PlotTrends::lumiperIOV(vector<int> IOVlist, TString lumifile) {
  size_t nIOVs = IOVlist.size();
  vector<pair<int, double>> lumiperIOV;
  vector<pair<int, double>> lumiperRun;

  std::ifstream infile(lumifile.Data());
  int run;
  double runLumi;
  while (infile >> run >> runLumi) {
    lumiperRun.push_back(make_pair(run, runLumi));
  }
  vector<int> xRunFromLumiFile;
  vector<int> missingruns;
  for (size_t iRun = 0; iRun < lumiperRun.size(); iRun++)
    xRunFromLumiFile.push_back(lumiperRun[iRun].first);
  for (int run : IOVlist) {
    if (find(xRunFromLumiFile.begin(), xRunFromLumiFile.end(), run) == xRunFromLumiFile.end()) {
      missingruns.push_back(run);
      lumiperRun.push_back(make_pair(run, 0.));
    }
  }
  std::sort(lumiperRun.begin(), lumiperRun.end());

  if (!missingruns.empty()) {
    cout << "WARNING: some IOVs are missing in the run/luminosity txt file: " << lumifile << endl
         << "List of missing runs:" << endl;
    for (int missingrun : missingruns)
      cout << to_string(missingrun) << " ";
    cout << endl;
    cout << "NOTE: the missing runs are added in the code with luminosity = 0 (they are not stored in the input file), "
            "please check that the IOV numbers are correct!"
         << endl;
  }

  size_t i = 0;
  size_t index = 0;
  double lumiInput = 0.;
  double lumiOutput = 0.;

  // WIP

  while (i <= nIOVs) {
    double run = 0;
    double lumi = 0.;
    if (i != nIOVs)
      run = IOVlist.at(i);
    else
      run = 0;
    for (size_t j = index; j < lumiperRun.size(); j++) {
      //cout << run << " - " << lumiperRun.at(j).first << " - lumi added: " << lumi << endl;
      if (run == lumiperRun.at(j).first) {
        index = j;
        break;
      } else
        lumi += lumiperRun.at(j).second;
    }
    if (i == 0)
      lumiperIOV.push_back(make_pair(0, lumi));
    else
      lumiperIOV.push_back(make_pair(IOVlist.at(i - 1), lumi));
    ++i;
  }
  for (size_t j = 0; j < lumiperRun.size(); j++)
    lumiInput += lumiperRun.at(j).second;
  for (size_t j = 0; j < lumiperIOV.size(); j++)
    lumiOutput += lumiperIOV.at(j).second;
  if (abs(lumiInput - lumiOutput) > 0.5) {
    cout << "ERROR: luminosity retrieved for IOVs does not match the one for the runs." << endl
         << "Please check that all IOV first runs are part of the run-per-lumi file!" << endl;
    cout << "Total lumi from lumi-per-run file: " << lumiInput << endl;
    cout << "Total lumi saved for IOVs: " << lumiOutput << endl;
    cout << "Size of IOVlist " << IOVlist.size() << endl;
    cout << "Size of lumi-per-IOV list " << lumiperIOV.size() << endl;
    //for (size_t j = 0; j < lumiperIOV.size(); j++)
    //  cout << (j==0 ? 0 : IOVlist.at(j-1)) << " == " << lumiperIOV.at(j).first << " " <<lumiperIOV.at(j).second <<endl;
    //for (auto pair : lumiperIOV) cout << pair.first << " ";
    //cout << endl;
    //for (auto IOV : IOVlist) cout << IOV << " ";
    //cout << endl;
    exit(EXIT_FAILURE);
  }
  cout << "final lumi= " << lumiOutput << endl;
  //for debugging
  //for (size_t j = 0; j < lumiperIOV.size(); j++)
  //  cout << lumiperIOV.at(j).first << " " <<lumiperIOV.at(j).second <<endl;

  return lumiperIOV;
}

/*! \fn ConvertToHist
 *  \brief A TH1F is constructed using the points and the errors collected in the TGraphErrors
 */

TH1F *PlotTrends::ConvertToHist(TGraphErrors *g) {
  size_t N = g->GetN();
  double *x = g->GetX();
  double *y = g->GetY();
  double *xerr = g->GetEX();
  vector<float> bins;
  bins.push_back(x[0] - xerr[0]);
  for (size_t i = 1; i < N; i++) {
    if ((x[i - 1] + xerr[i - 1]) > (bins.back() + pow(10, -6)))
      bins.push_back(x[i - 1] + xerr[i - 1]);
    if ((x[i] - xerr[i]) > (bins.back() + pow(10, -6)))
      bins.push_back(x[i] - xerr[i]);
  }
  bins.push_back(x[N - 1] + xerr[N - 1]);
  TString histoname = "histo_";
  histoname += g->GetName();
  TH1F *histo = new TH1F(histoname, g->GetTitle(), bins.size() - 1, bins.data());
  for (size_t i = 0; i < N; i++) {
    histo->Fill(x[i], y[i]);
    histo->SetBinError(histo->FindBin(x[i]), pow(10, -6));
  }
  return histo;
}

/*! \fn PlotDMRTrends
 *  \brief Plot the DMR trends.
 */

void PlotTrends::PlotDMRTrends(vector<int> IOVlist,
                   TString Variable,
                   vector<string> labels,
                   TString Year,
                   string myValidation,
                   vector<string> geometries,
                   vector<Color_t> colours,
                   TString outputdir,
                   bool pixelupdate,
                   vector<int> pixelupdateruns,
                   bool showlumi,
                   TString lumifile,
		   vector<pair<int, double>> lumiIOVpairs,
		   TString structure,
                   int layer) {
  gErrorIgnoreLevel = kWarning;
  checkrunlist(pixelupdateruns, {});

  TString filename = myValidation + "PVtrends";
  for (const auto &label : labels) {
    if(label != "") {
      filename += "_";
      filename += label;
    }
  }
  filename += ".root";
  TFile *in = new TFile(filename);
  TString structname = structure;
  //structname.ReplaceAll("_y", "");
  //For debugging purposes we still might want to have a look at plots for a variable without errors, once ready for the PR those variables will be removed and the iterator will start from 0
  for (size_t i = 0; i < variables_.size(); i++) {
    TString variable = variables_.at(i);
    if (variable.Contains("plus") || variable.Contains("minus") || variable.Contains("delta")) {
      if (structname == "TEC" || structname == "TID")
        continue;  //Lorentz drift cannot appear in TEC and TID. These structures are skipped when looking at outward and inward pointing modules.
    }
    TCanvas *c = new TCanvas("dummy", "", 2000, 800);

    vector<Color_t>::iterator colour = colours.begin();

    TMultiGraph *mg = new TMultiGraph(structure, structure);
    THStack *mh = new THStack(structure, structure);
    size_t igeom = 0;
    for (const string &geometry : geometries) {
      //TString name = getName(structure, layer, geometry);
      TString name = Variable + "_" + getName(structure, layer, geometry);
      TGraphErrors *g = dynamic_cast<TGraphErrors *>(in->Get(name + "_" + variables_.at(i)));
      g->SetName(name + "_" + variables_.at(i));
      g->SetLineWidth(1);
      g->SetLineColor(*colour);
      g->SetFillColorAlpha(*colour, 0.2);
      //if (i >= 8) {
      //  g->SetLineWidth(1);
      //  g->SetLineColor(*colour);
      //  g->SetFillColorAlpha(*colour, 0.2);
      //}
      vector<vector<double>> vectors;
      //if(showlumi&&i<8)scalebylumi(dynamic_cast<TGraph*>(g));
      if (showlumi)
        scalebylumi(g, lumiIOVpairs);
      g->SetLineColor(*colour);
      g->SetMarkerColor(*colour);
      TH1F *h = ConvertToHist(g);
      h->SetLineColor(*colour);
      h->SetMarkerColor(*colour);
      h->SetMarkerSize(0);
      h->SetLineWidth(1);

      mg->Add(g, "2");
      mh->Add(h, "E");
      //if (i < 8) {
      //  mg->Add(g, "PL");
      //  mh->Add(h, "E");
      //} else {
      //  mg->Add(g, "2");
      //  mh->Add(h, "E");
      //}
      ++colour;
      ++igeom;
    }

    gStyle->SetOptTitle(0);
    gStyle->SetPadLeftMargin(0.08);
    gStyle->SetPadRightMargin(0.05);
    gPad->SetTickx();
    gPad->SetTicky();
    gStyle->SetLegendTextSize(0.025);

    double max = 10.;
    double min = -10.;
    if (Variable == "DrmsNR") {
      if (variable.Contains("delta")) {
        max = 0.15;
        min = -0.1;
      } else {
        max = 1.2;
        min = 0.6;
      }
    }
    else if (Variable == "RMS") {
      max = 90;
      min = 0;
    }

    double range = max - min;
    if (((variable == "sigma" || variable == "sigmaplus" || variable == "sigmaminus" ||
          variable == "sigmadeltamu") &&
         range >= 2)) {
      mg->SetMaximum(4);
      mg->SetMinimum(-2);
    } else {
      mg->SetMaximum(max + range * 0.1);
      mg->SetMinimum(min - range * 0.3);
    }

    mg->Draw("a");
    mg->Draw("a2");
    //if (i < 8) {
    //  mg->Draw("a");
    //} else {
    //  mg->Draw("a2");
    //}
    std::string name_of_y_axis;
    if (PlotType_ == "PV") {
      if(Variable == "mean")
	name_of_y_axis = "Mean " + YaxisNames_.at(i);
      else if (Variable == "RMS")
	name_of_y_axis = "RMS " + YaxisNames_.at(i);
      else
	name_of_y_axis = YaxisNames_.at(i);
    }
    char *Ytitle = (char *)name_of_y_axis.c_str();
    mg->GetYaxis()->SetTitle(Ytitle);
    mg->GetXaxis()->SetTitle(showlumi ? "Integrated lumi [1/fb]" : "IOV number");
    mg->GetXaxis()->CenterTitle(true);
    mg->GetYaxis()->CenterTitle(true);
    mg->GetYaxis()->SetTitleOffset(.5);
    mg->GetYaxis()->SetTitleSize(.05);
    mg->GetXaxis()->SetTitleSize(.04);
    if (showlumi)
      mg->GetXaxis()->SetLimits(0., mg->GetXaxis()->GetXmax());

    c->Update();
    TLegend *legend = c->BuildLegend(0.08, 0.1, 0.25, 0.3);
    // TLegend *legend = c->BuildLegend(0.15,0.18,0.15,0.18);
    int Ngeom = geometries.size();
    if (Ngeom >= 6)
      legend->SetNColumns(2);
    else if (Ngeom >= 9)
      legend->SetNColumns(3);
    else
      legend->SetNColumns(1);
    TString structtitle;
    if (PlotType_ == "DMR") {
      structtitle = "#bf{";
      if (structure.Contains("PIX") && !(structure.Contains("_y")))
	structtitle += structure + " (x)";
      else if (structure.Contains("_y")) {
	TString substring(structure(0, 4));
	structtitle += substring + " (y)";
      } else
	structtitle += structure;
      if (layer != 0) {
	if (structure == "TID" || structure == "TEC" || structure == "FPIX" || structure == "FPIX_y")
	  structtitle += "  disc ";
	else
	  structtitle += "  layer ";
	structtitle += layer;
      }
      structtitle += "}";
    }
    PixelUpdateLines(c, lumifile, showlumi, pixelupdateruns);

    //TLine *lineOla2 = new TLine();
    //lineOla2->SetLineColor(kBlack);
    //lineOla2->SetLineStyle(1);
    //lineOla2->SetLineWidth(4);
    //legend->AddEntry(lineOla2, "First run of the year", "l");

    TLine *lineOla = new TLine();
    lineOla->SetLineColor(kBlack);
    lineOla->SetLineStyle(3);
    legend->AddEntry(lineOla, "Pixel calibration update", "l");
    //
    //TLine *lineOla3 = new TLine();
    //lineOla3->SetLineColor(kYellow + 1);
    //lineOla3->SetLineStyle(1);
    //legend->AddEntry(lineOla3, "Base Run for pixel template", "l");
    //
    //TLine *lineOla4 = new TLine();
    //lineOla4->SetLineColor(kRed + 1);
    //lineOla4->SetLineStyle(1);
    //legend->AddEntry(lineOla4, "Problematic base runs", "l");

    legend->Draw();

    double LumiTot = getintegratedlumiuptorun(-1, lumifile);
    LumiTot = LumiTot / lumiFactor;
    if (variable == "sigma" || variable == "sigmaplus" || variable == "sigmaminus" || variable == "sigmadeltamu" ||
        variable == "deltamu" || variable == "deltamusigmadeltamu") {
      TLine *line = new TLine(0., 0., LumiTot, 0.);
      line->SetLineColor(kMagenta);
      line->Draw();
    }

    if (Variable == "median") {
      if (variable == "musigma" || variable == "muminus" || variable == "mu" || variable == "muminussigmaminus" ||
          variable == "muplus" || variable == "muplussigmaplus") {
        TLine *line = new TLine(0., 0., LumiTot, 0.);
        line->SetLineColor(kMagenta);
        line->Draw();
      }
    }

    if (Variable == "DrmsNR") {
      if (variable == "musigma" || variable == "muminus" || variable == "mu" || variable == "muminussigmaminus" ||
          variable == "muplus" || variable == "muplussigmaplus") {
        TLine *line = new TLine(0., 1., LumiTot, 1.);
        line->SetLineColor(kMagenta);
        line->Draw();
      }
    }

    TPaveText *CMSworkInProgress = new TPaveText(
        0, mg->GetYaxis()->GetXmax() + range * 0.02, 2.5, mg->GetYaxis()->GetXmax() + range * 0.12, "nb");

    CMSworkInProgress->AddText("#scale[1.1]{CMS} #it{Preliminary}");  //ola changed it for public plots
    CMSworkInProgress->SetTextAlign(12);
    CMSworkInProgress->SetTextSize(0.04);
    CMSworkInProgress->SetFillColor(10);
    //CMSworkInProgress->Draw();
    TPaveText *TopRightCorner = new TPaveText(0.85 * (mg->GetXaxis()->GetXmax()),
                                              mg->GetYaxis()->GetXmax() + range * 0.08,
                                              0.95 * (mg->GetXaxis()->GetXmax()),
                                              mg->GetYaxis()->GetXmax() + range * 0.18,
                                              "nb");
    if (Year == "Run2")
      TopRightCorner->AddText("#bf{CMS} #it{Work in progress} (2016+2017+2018 pp collisions)");
    else
      TopRightCorner->AddText("#bf{CMS} #it{Work in progress} (" + Year + " pp collisions)");
    TopRightCorner->SetTextAlign(32);
    TopRightCorner->SetTextSize(0.04);
    TopRightCorner->SetFillColor(10);
    TopRightCorner->Draw();
    if (PlotType_ == "DMR") {
      TPaveText *structlabel = new TPaveText(0.85 * (mg->GetXaxis()->GetXmax()),
					     mg->GetYaxis()->GetXmin() + range * 0.02,
					     0.85 * (mg->GetXaxis()->GetXmax()),
					     mg->GetYaxis()->GetXmin() + range * 0.12,
					     "nb");
      structlabel->AddText(structtitle.Data());
      structlabel->SetTextAlign(32);
      structlabel->SetTextSize(0.04);
      structlabel->SetFillColor(10);
      structlabel->Draw();
    }

    legend->Draw();
    mh->Draw("nostack same");
    c->Update();
    TString structandlayer = getName(structure, layer, "");
    TString printfile = outputdir;
    if (!(outputdir.EndsWith("/")))
      outputdir += "/";
    for (const auto &label : labels) {
      if(label != "") {
	printfile += label;
	printfile += "_";
      }
    }
    printfile += Variable;
    printfile += "_";
    printfile += variable + structandlayer;
    c->SaveAs(printfile + ".pdf");
    c->SaveAs(printfile + ".eps");
    c->SaveAs(printfile + ".png");
    c->Destructor();
  }
  
  
  in->Close();
}
