#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <TROOT.h>
#include <TCanvas.h>
#include <TLegend.h>
//#include <TH1.h> // TODO
#include <TGraph.h>

using namespace std;
namespace fs = boost::filesystem;
namespace pt = boost::property_tree;

////////////////////////////////////////////////////////////////////////////////
/// Functor to obtain delivered luminosity corresponding to a given range.
///
/// The constructor extracts the luminosity from an 2-column file (see constructor),
/// and the operator() returns the accumulated luminosity for any subrange.
///
/// Another overload of the operator() allows the conversion of a `TGraphErrors`
/// as a function of the run number to another `TGraphErrors` as a function of the
/// luminosity.
struct Run2Lumi {

    const int firstRun, //!< first run, starting at lumi = 0 on the x-axis of the trend
              lastRun;  //!< last run (excluded!), starting at the max lumi on the x-axis of the trend

private:
    map<int, float> runs; //!< couples of run and corresponding luminosity

public:

    ////////////////////////////////////////////////////////////////////////////////
    /// Constructor, takes as input a 2-column file that will populate the map `runs`
    /// as well as the global range.
    ///
    /// (Note: in principle, the global range could be extracted from the 2-column file,
    /// but the choice here is to have one standard lumi file, leaving as only freedom
    /// to the user the possibility to change the global range depending on the analysis.)
    Run2Lumi
        (fs::path file, //!< path to a 2-column file with 6-digit run number and lumi in /pb
         int first, //!< 6-digit run number (included)
         int last //!< 6-digit run number (excluded)
         );

    ////////////////////////////////////////////////////////////////////////////////
    /// Sums luminosity for [run1, run2[ and returns it in /fb.
    float operator() (int run1, int run2) const;

    ////////////////////////////////////////////////////////////////////////////////
    /// Sums luminosity for [firstRun, run[
    float operator() (int run) const;

    ////////////////////////////////////////////////////////////////////////////////
    /// Sums luminosity for [firstRun, lastRun[
    float operator() () const;

    ////////////////////////////////////////////////////////////////////////////////
    /// Converts a `TGraph` given as a function of the run number
    /// to another `TGraph` as a function of the luminosity
    /// in the global subrange.
    TGraph * operator() (TGraph * gIn) const;

    ////////////////////////////////////////////////////////////////////////////////
    /// Converts a `TH1` given as a function of the run number
    /// to another `TH1` as a function of the luminosity
    /// in the global subrange.
    ///
    /// Note: the run is expected to be on the low edge of each bin
    TH1 * operator() (TH1 * hIn) const;
};

////////////////////////////////////////////////////////////////////////////////
/// Standard canvas for TkAl trends as a function of delivered lumi.
///
/// The constructor and the destructor take care of the style,
/// The operator() is overloaded and should be given a trend as a function of 
/// the run number (either `TGraphErrors` or `TH1`).
/// The verticles lines (e.g. pixel templates) are given via a JSON file.
struct Trend {

    static TString CMS, //!< top loeft label
                   lumi; //!< top right label (not necessarily the lumi, just following the convention from `CMS_lumi.h`)

    static float fontsize;

    TCanvas c;
    const char * outputDir; //directory for plots
    TLegend lgd;

    const pt::ptree JSON; //!< contains coordinate for vertical lines
    const Run2Lumi& GetLumi; //!< functor to get luminosity for given subrange

    Trend //!< constructor, prepares canvas and frame
        (const char * name, //!< TCanvas name, also used for output PDF
	 const char * dir, //directory for plots
         const char * title, //!< TCanvas title, also used for output PDF (but not shown on the canvas)
         const char * ytitle, //!< y-axis title
         float ymin, //!< y-axis minimum
         float ymax, //!< y-axis maximum
         pt::ptree& json, //!< vertical lines from JSON
         const Run2Lumi& GetLumiFunctor //!< functor
         );

    ////////////////////////////////////////////////////////////////////////////////
    /// Operator overloading to plot a trend (given as a function of run number)
    /// as a function of the luminosity.
    void operator()
        (TObject * obj, //!< e.g. graph
         TString drawOpt, //!< e.g. option for `TGraph::Draw()`
         TString lgdOpt, //!< option for `TLegend::Draw()`
         bool fullRange = true //!< flag to force the graph to touch the right edge
         );

    ////////////////////////////////////////////////////////////////////////////////
    /// Destructor applies fine tuning of the plots, e.g. legend, labels, etc.
    /// and finally prints to PDF.
    ~Trend ();
};

template<typename T, typename ... Args> inline T * Get (Args ... args) { return dynamic_cast<T*>(gDirectory->Get(Form(args...))); } 
