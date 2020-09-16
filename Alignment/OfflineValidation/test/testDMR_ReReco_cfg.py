import FWCore.ParameterSet.Config as cms
import FWCore.PythonUtilities.LumiList as LumiList

import json
import os

#example python config with ReReco where all is taken from the GT

##Set validation mode
valiMode = "StandAlone"

##Define process
process = cms.Process("Demo")

##Get good lumi section

goodLumiSecs = cms.untracked.VLuminosityBlockRange()

##Define input source

process.source = cms.Source("PoolSource",
                            fileNames = cms.untracked.vstring(
                                 '/store/data/Run2018A/HLTPhysics/ALCARECO/TkAlMinBias-06Jun2018-v1/40000/42CE0886-AC92-E811-A502-1866DAEA79C8.root',
                                 '/store/data/Run2018A/HLTPhysics/ALCARECO/TkAlMinBias-06Jun2018-v1/40000/9619D443-B192-E811-B466-1418774105B6.root',
                                 '/store/data/Run2018A/HLTPhysics/ALCARECO/TkAlMinBias-06Jun2018-v1/40000/CEF62DB7-B592-E811-8969-B083FED426E4.root',
                                 '/store/data/Run2018A/HLTPhysics/ALCARECO/TkAlMinBias-06Jun2018-v1/40000/B2637D14-B892-E811-A8E5-C81F66B7910C.root',
                                 '/store/data/Run2018A/HLTPhysics/ALCARECO/TkAlMinBias-06Jun2018-v1/40000/243EE1AC-3793-E811-8EEA-B083FED045EC.root',
                                 '/store/data/Run2018A/HLTPhysics/ALCARECO/TkAlMinBias-06Jun2018-v1/40000/96A8BD93-9293-E811-88E2-801844DEEC30.root',
                                 '/store/data/Run2018A/HLTPhysics/ALCARECO/TkAlMinBias-06Jun2018-v1/40000/6E137D1A-9B93-E811-9605-001EC94A8EF1.root'),
                                 duplicateCheckMode = cms.untracked.string('checkAllFilesOpened')
)

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(10) )

process.source.lumisToProcess = goodLumiSecs

##Bookeeping
process.options = cms.untracked.PSet(
   wantSummary = cms.untracked.bool(False),
   Rethrow = cms.untracked.vstring("ProductNotFound"),
   fileMode  =  cms.untracked.string('NOMERGE'),
)

process.load("FWCore.MessageLogger.MessageLogger_cfi")
process.MessageLogger.destinations = ['cout', 'cerr']
process.MessageLogger.cerr.FwkReport.reportEvery = 1
process.MessageLogger.statistics.append('cout')

##Basic modules
process.load("RecoVertex.BeamSpotProducer.BeamSpot_cff")
process.load("Configuration.Geometry.GeometryDB_cff")
process.load('Configuration.StandardSequences.Services_cff')
process.load("Configuration.StandardSequences.MagneticField_cff")

##Track fitting
import Alignment.CommonAlignment.tools.trackselectionRefitting as trackselRefit
process.seqTrackselRefit = trackselRefit.getSequence(process, 
                                                     "ALCARECOTkAlMinBias",
                                                     isPVValidation = False, 
                                                     TTRHBuilder = "WithAngleAndTemplate",
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
process.GlobalTag = GlobalTag(process.GlobalTag, "106X_dataRun2_v10")

##Filter good events
process.oneGoodVertexFilter = cms.EDFilter("VertexSelector",
                                           src = cms.InputTag("offlinePrimaryVertices"),
                                           cut = cms.string("!isFake && ndof > 4 && abs(z) <= 15 && position.Rho <= 2"),
                                           filter = cms.bool(True),
)
process.FilterGoodEvents=cms.Sequence(process.oneGoodVertexFilter)

process.noScraping= cms.EDFilter("FilterOutScraping",
                                 src=cms.InputTag("FinalTrackRefitter"),
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
    moduleLevelHistsTransient = cms.bool(False if valiMode == "DQM" else True), 
    moduleLevelProfiles       = cms.bool(False if valiMode == "DQM" else True),
    localCoorProfilesOn       = cms.bool(False),
    stripYResiduals           = cms.bool(False),
    useFwhm                   = cms.bool(True),
    useFit                    = cms.bool(False),  # Unused in DQM mode, where it has to be specified in TrackerOfflineValidationSummary
    useCombinedTrajectory     = cms.bool(False),
    useOverflowForRMS         = cms.bool(False),
    maxTracks                 = cms.uint64(0),
    chargeCut                 = cms.int32(0),
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
        fileName = cms.string("$(CMSSW_BASE)/src/Alignment/OfflineValidation/test/Test/DMR/single/Legacy/ReReco/316569/output/DMR.root"),
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
