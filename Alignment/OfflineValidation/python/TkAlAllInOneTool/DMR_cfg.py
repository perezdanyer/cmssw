import FWCore.ParameterSet.Config as cms
import FWCore.PythonUtilities.LumiList as LumiList

from FWCore.ParameterSet.VarParsing import VarParsing

import json

##Define process
process = cms.Process("OfflineValidator")

##Argument parsing
options = VarParsing()
options.register("config", "", VarParsing.multiplicity.singleton, VarParsing.varType.string , "AllInOne config")

options.parseArguments()

##Set validation mode
valiMode = "StandAlone"

##Read in AllInOne config in JSON format
with open(options.config, "r") as configFile:
    config = json.load(configFile)

##Read filenames from given TXT file
readFiles = []

with open(config["validation"]["dataset"], "r") as datafiles:
    for fileName in datafiles.readlines():
        readFiles.append(fileName.replace("\n", ""))

##Get good lumi section
if "goodlumi" in config["validation"]:
    goodLumiSecs = cms.untracked.VLuminosityBlockRange(LumiList.LumiList(filename = config["validation"]["goodlumi"]).getCMSSWString().split(','))

else:
    goodLumiSecs = cms.untracked.VLuminosityBlockRange()

##Define input source
process.source = cms.Source("PoolSource",
                            fileNames = cms.untracked.vstring(readFiles),
                         #   lumisToProcess = goodLumiSecs,
)

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(config["validation"].get("maxevents", 2000000))
)

##Bookeeping
process.options = cms.untracked.PSet(
   wantSummary = cms.untracked.bool(False),
   Rethrow = cms.untracked.vstring("ProductNotFound"),
   fileMode  =  cms.untracked.string('NOMERGE'),
)

process.load("FWCore.MessageLogger.MessageLogger_cfi")
process.MessageLogger.destinations = ['cout', 'cerr']
process.MessageLogger.cerr.FwkReport.reportEvery = 1000
process.MessageLogger.statistics.append('cout')

##Basic modules
process.load("RecoVertex.BeamSpotProducer.BeamSpot_cff")
process.load("Configuration.Geometry.GeometryDB_cff")
process.load('Configuration.StandardSequences.Services_cff')
process.load("Configuration.StandardSequences.MagneticField_cff")

##Track fitting
import Alignment.CommonAlignment.tools.trackselectionRefitting as trackselRefit
process.seqTrackselRefit = trackselRefit.getSequence(process, 
                                                     config["validation"]["trackcollection"],
                                                     isPVValidation = False, 
                                                     TTRHBuilder = config["validation"].get("tthrbuilder", "WithAngleAndTemplate"),
                                                     usePixelQualityFlag = False,
                                                     openMassWindow = False,
                                                     cosmicsDecoMode = True,
                                                     cosmicsZeroTesla = False,
                                                     momentumConstraint = None,
                                                     cosmicTrackSplitting = False,
                                                     use_d0cut = True,
)

#Global tag
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, config["alignment"].get("globaltag", "106X_dataRun2_v27"))

##Load conditions if wished
if "conditions" in config["alignment"]:
    for condition in ["alignment"]["conditions"]:
        setattr(process, "conditionsIn{}".format(condition), CalibTracker.Configuration.Common.PoolDBESSource_cfi.poolDBESSource.clone(
             connect = cms.string(config["alignment"]["conditions"][condition]["connect"]),
             toGet = cms.VPSet(
                        cms.PSet(
                                 record = cms.string(condition),
                                 tag = cms.string(config["alignment"]["conditions"][condition]["tag"])
                        )
                     )
            )
        )

        setattr(process, "prefer_conditionsIn{}".format(condition), cms.ESPrefer("PoolDBESSource", "conditionsIn{}".format(condition)))


##Filter good events
process.oneGoodVertexFilter = cms.EDFilter("VertexSelector",
                                           src = cms.InputTag("offlinePrimaryVertices"),
                                           cut = cms.string("!isFake && ndof > 4 && abs(z) <= 15 && position.Rho <= 2"),
                                           filter = cms.bool(True),
)
process.FilterGoodEvents=cms.Sequence(process.oneGoodVertexFilter)

process.noScraping= cms.EDFilter("FilterOutScraping",
                                 src=cms.InputTag(config["validation"]["trackcollection"]),
                                 applyfilter = cms.untracked.bool(True),
                                 debugOn = cms.untracked.bool(False),
                                 numtrack = cms.untracked.uint32(10),
                                 thresh = cms.untracked.double(0.25),
)

##Offline validation analyzer
process.TrackerOfflineValidation = cms.EDAnalyzer("TrackerOfflineValidation",
    useInDqmMode              = cms.bool(True if valiMode == "DQM" else False), 
    moduleDirectoryInOutput   = cms.string("Alignment/Tracker" if valiMode == "DQM" else ""), 
    Tracks                    = cms.InputTag("FinalTrackRefitter"),
    trajectoryInput           = cms.string('FinalTrackRefitter'),  # Only needed in DQM mode
    localCoorHistosOn         = cms.bool(False),
    moduleLevelHistsTransient = config["validation"].get("moduleLevelHistsTransient", cms.bool(False if valiMode == "DQM" else True)), 
    moduleLevelProfiles       = config["validation"].get("moduleLevelProfiles", cms.bool(False if valiMode == "DQM" else True)),
    localCoorProfilesOn       = cms.bool(False),
    stripYResiduals           = cms.bool(config["validation"].get("stripYResiduals", False)),
    useFwhm                   = cms.bool(True),
    useFit                    = cms.bool(False),  # Unused in DQM mode, where it has to be specified in TrackerOfflineValidationSummary
    useCombinedTrajectory     = cms.bool(False),
    useOverflowForRMS         = cms.bool(False),
    maxTracks                 = cms.uint64(config["validation"].get("maxtracks", 0)),
    chargeCut                 = cms.int32(config["validation"].get("chargecut", 0)),
    # Normalized X Residuals, normal local coordinates (Strip)
    TH1NormXResStripModules = cms.PSet(
        Nbinx = cms.int32(100), xmin = cms.double(-5.0), xmax = cms.double(5.0)
    ),

    # X Residuals, normal local coordinates (Strip)
    TH1XResStripModules = cms.PSet(
        Nbinx = cms.int32(100), xmin = cms.double(-0.5), xmax = cms.double(0.5)
    ),

    # Normalized X Residuals, native coordinates (Strip)
    TH1NormXprimeResStripModules = cms.PSet(
        Nbinx = cms.int32(120), xmin = cms.double(-3.0), xmax = cms.double(3.0)
    ),

    # X Residuals, native coordinates (Strip)
    TH1XprimeResStripModules = cms.PSet(
        Nbinx = cms.int32(5000), xmin = cms.double(-0.05), xmax = cms.double(0.05)
    ),

    # Normalized Y Residuals, native coordinates (Strip -> hardly defined)
    TH1NormYResStripModules = cms.PSet(
        Nbinx = cms.int32(120), xmin = cms.double(-3.0), xmax = cms.double(3.0)
    ),
    # -> very broad distributions expected
    TH1YResStripModules = cms.PSet(
        Nbinx = cms.int32(5000), xmin = cms.double(-11.0), xmax = cms.double(11.0)
    ),

    # Normalized X residuals normal local coordinates (Pixel)
    TH1NormXResPixelModules = cms.PSet(
        Nbinx = cms.int32(120), xmin = cms.double(-3.0), xmax = cms.double(3.0)
    ),
    # X residuals normal local coordinates (Pixel)
    TH1XResPixelModules = cms.PSet(
        Nbinx = cms.int32(100), xmin = cms.double(-0.5), xmax = cms.double(0.5)
    ),
    # Normalized X residuals native coordinates (Pixel)
    TH1NormXprimeResPixelModules = cms.PSet(
        Nbinx = cms.int32(120), xmin = cms.double(-3.0), xmax = cms.double(3.0)
    ),
    # X residuals native coordinates (Pixel)
    TH1XprimeResPixelModules = cms.PSet(
        Nbinx = cms.int32(500), xmin = cms.double(-0.05), xmax = cms.double(0.05)
    ),
    # Normalized Y residuals native coordinates (Pixel)
    TH1NormYResPixelModules = cms.PSet(
        Nbinx = cms.int32(120), xmin = cms.double(-3.0), xmax = cms.double(3.0)
    ),
    # Y residuals native coordinates (Pixel)
    TH1YResPixelModules = cms.PSet(
        Nbinx = cms.int32(5000), xmin = cms.double(-0.05), xmax = cms.double(0.05)
    ),
    # X Residuals vs reduced local coordinates (Strip)
    TProfileXResStripModules = cms.PSet(
        Nbinx = cms.int32(34), xmin = cms.double(-1.02), xmax = cms.double(1.02)
    ),
    # X Residuals vs reduced local coordinates (Strip)
    TProfileYResStripModules = cms.PSet(
        Nbinx = cms.int32(34), xmin = cms.double(-1.02), xmax = cms.double(1.02)
    ),
    # X Residuals vs reduced local coordinates (Pixel)
    TProfileXResPixelModules = cms.PSet(
        Nbinx = cms.int32(17), xmin = cms.double(-1.02), xmax = cms.double(1.02)
    ),
    # X Residuals vs reduced local coordinates (Pixel)
    TProfileYResPixelModules = cms.PSet(
        Nbinx = cms.int32(17), xmin = cms.double(-1.02), xmax = cms.double(1.02)
    ),
)

##Define sequences depending on validation mode
if valiMode == "StandAlone":
    ##Output file

    process.TFileService = cms.Service("TFileService",
            fileName = cms.string("{}/DMR.root".format(config["output"])),
            closeFileFast = cms.untracked.bool(True),
    )

    seqTrackerOfflineValidation = cms.Sequence(process.TrackerOfflineValidation)

if valiMode == "DQM":
    TrackerOfflineValidationSummary = cms.EDAnalyzer("TrackerOfflineValidationSummary",
        moduleDirectoryInOutput = cms.string("Alignment/Tracker"),
        useFit = cms.bool(False),
        stripYDmrs = cms.bool(False),
        minEntriesPerModuleForDmr = cms.uint32(100),
   
        # DMR (distribution of median of residuals per module) of X coordinate (Strip)
        TH1DmrXprimeStripModules = cms.PSet(
            Nbinx = cms.int32(50), xmin = cms.double(-0.005), xmax = cms.double(0.005)
        ),
   
        # DMR (distribution of median of residuals per module) of Y coordinate (Strip)
        TH1DmrYprimeStripModules = cms.PSet(
            Nbinx = cms.int32(50), xmin = cms.double(-0.005), xmax = cms.double(0.005)
        ),
   
        # DMR (distribution of median of residuals per module) of X coordinate (Pixel)
        TH1DmrXprimePixelModules = cms.PSet(
            Nbinx = cms.int32(50), xmin = cms.double(-0.005), xmax = cms.double(0.005)
        ),
   
        # DMR (distribution of median of residuals per module) of Y coordinate (Pixel)
        TH1DmrYprimePixelModules = cms.PSet(
            Nbinx = cms.int32(50), xmin = cms.double(-0.005), xmax = cms.double(0.005)
        )
    )

    ## DQM file saver
    from DQMServices.Core.DQM_cfg import *
    DqmSaverTkAl = cms.EDAnalyzer("DQMFileSaver",
            convention=cms.untracked.string("Offline"),
            workflow=cms.untracked.string("/Cosmics/TkAl09-AlignmentSpecification_R000100000_R000100050_ValSkim-v1/ALCARECO"), 
            dirName=cms.untracked.string("."),
            saveByRun=cms.untracked.int32(-1),
            saveAtJobEnd=cms.untracked.bool(True),                        
            forceRunNumber=cms.untracked.int32(100000)   # Current Convention: Take first processed run
    )

    seqTrackerOfflineValidation = cms.Sequence(
                        TrackerOfflineValidation*
                        TrackerOfflineValidationSummary*
                        DqmSaverTkAl
    )   

##Let all sequences run
process.p = cms.Path(process.seqTrackselRefit*seqTrackerOfflineValidation)
