import FWCore.ParameterSet.Config as cms
import FWCore.PythonUtilities.LumiList as LumiList

from FWCore.ParameterSet.VarParsing import VarParsing

import json
import os

##Define process
process = cms.Process("PrimaryVertexValidation")

##Argument parsing
options = VarParsing()
options.register("config", "", VarParsing.multiplicity.singleton, VarParsing.varType.string , "AllInOne config")

options.parseArguments()

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
    if os.path.isfile(config["validation"]["goodlumi"]):
        goodLumiSecs = cms.untracked.VLuminosityBlockRange(LumiList.LumiList(filename = config["validation"]["goodlumi"]).getCMSSWString().split(','))

    else:
        print("Does not exist: {}. Continue without good lumi section file.")
        goodLumiSecs = cms.untracked.VLuminosityBlockRange()

else:
    goodLumiSecs = cms.untracked.VLuminosityBlockRange()

##Define input source
process.source = cms.Source("PoolSource",
                            fileNames = cms.untracked.vstring(readFiles),
                            lumisToProcess = goodLumiSecs,
)

process.source.firstRun = cms.untracked.uint32(int(config["validation"].get("runboundary", 1)))
#TODO should runboundry be set to 1 if ismc?
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(config["validation"].get("maxevents", 2000000)))

##Bookeeping
process.options = cms.untracked.PSet(
   wantSummary = cms.untracked.bool(False),
   Rethrow = cms.untracked.vstring("ProductNotFound"),
   fileMode  =  cms.untracked.string('NOMERGE'),
)

process.load('FWCore.MessageService.MessageLogger_cfi')
process.MessageLogger.categories.append("PrimaryVertexValidation")  
process.MessageLogger.categories.append("FilterOutLowPt")
process.MessageLogger.destinations = cms.untracked.vstring("cout")
process.MessageLogger.cout = cms.untracked.PSet(
    threshold = cms.untracked.string("INFO"),
    default   = cms.untracked.PSet(limit = cms.untracked.int32(0)),                       
    FwkReport = cms.untracked.PSet(limit = cms.untracked.int32(-1),
                                   reportEvery = cms.untracked.int32(1000)
                                   ),                                                      
    PrimaryVertexValidation = cms.untracked.PSet( limit = cms.untracked.int32(-1)),
    FilterOutLowPt          = cms.untracked.PSet( limit = cms.untracked.int32(-1))
    )
process.MessageLogger.statistics.append('cout')

##Basic modules 
process.load("TrackingTools.TransientTrack.TransientTrackBuilder_cfi")
process.load('Configuration.StandardSequences.MagneticField_AutoFromDBCurrent_cff')
process.load("Configuration.Geometry.GeometryRecoDB_cff")
process.load("RecoVertex.BeamSpotProducer.BeamSpot_cff")

#Global tag
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, config["alignment"].get("globaltag", "106X_dataRun2_v27"))
#TODO is it dangerous to set a default global tag? 

##Load conditions if wished
if "conditions" in config["alignment"]:
    from CalibTracker.Configuration.Common.PoolDBESSource_cfi import poolDBESSource

    for condition in config["alignment"]["conditions"]:
        setattr(process, "conditionsIn{}".format(condition), poolDBESSource.clone(
             connect = cms.string(str(config["alignment"]["conditions"][condition]["connect"])),
             toGet = cms.VPSet(
                        cms.PSet(
                                 record = cms.string(str(condition)),
                                 tag = cms.string(str(config["alignment"]["conditions"][condition]["tag"]))
                        )
                     )
            )
        )

        setattr(process, "prefer_conditionsIn{}".format(condition), cms.ESPrefer("PoolDBESSource", "conditionsIn{}".format(condition)))

#should it include timetype and does it work for bows, kinks and apes


####################################################################
# Load and Configure event selection
####################################################################
process.primaryVertexFilter = cms.EDFilter("VertexSelector",
                                           src = cms.InputTag(config["validation"]["vertexcollection"]),
                                           cut = cms.string("!isFake && ndof > 4 && abs(z) <= 24 && position.Rho <= 2"),
                                           filter = cms.bool(True)
                                           )

process.noscraping = cms.EDFilter("FilterOutScraping",
                                  applyfilter = cms.untracked.bool(True),
                                  src =  cms.untracked.InputTag(config["validation"]["trackcollection"]), #config
                                  debugOn = cms.untracked.bool(False),
                                  numtrack = cms.untracked.uint32(10),
                                  thresh = cms.untracked.double(0.25)
                                  )


process.nolowpt = cms.EDFilter("FilterOutLowPt", #TODO does it matter that this is capital?
                                applyfilter = cms.untracked.bool(True),
                                src =  cms.untracked.InputTag(config["validation"]["trackcollection"]),
                                debugOn = cms.untracked.bool(False),
                                numtrack = cms.untracked.uint32(0),
                                thresh = cms.untracked.int32(1),
                                ptmin  = cms.untracked.double(config["validation"].get("ptmin", 3)),
                                runControl = cms.untracked.bool(config["validation"].get("runControl", True)),
                                runControlNumber = cms.untracked.vuint32(int(config["validation"].get("runboundary", 1)))
                                )

if config["validation"]["ismc"]:
     process.goodvertexSkim = cms.Sequence(process.noscraping) #TODO should low pt be filtered out
else:
     process.goodvertexSkim = cms.Sequence(process.primaryVertexFilter + process.noscraping + process.nolowpt)


if(config["validation"]["theRefitter"] == cms.untracked.string("common")):

     print(">>>>>>>>>> testPVValidation_cfg.py: msg%-i: using the common track selection and refit sequence!")          
     ####################################################################
     # Load and Configure Common Track Selection and refitting sequence
     ####################################################################
     import Alignment.CommonAlignment.tools.trackselectionRefitting as trackselRefit
     process.seqTrackselRefit = trackselRefit.getSequence(process,
                                                          config["validation"]["trackcollection"],
                                                          isPVValidation=True,
                                                          TTRHBuilder=config["validation"].get("tthrbuilder", "WithAngleAndTemplate"),
                                                          usePixelQualityFlag=True,
                                                          openMassWindow=False,
                                                          cosmicsDecoMode=True,
                                                          cosmicsZeroTesla=False, #TODO is it concerning that this is set to false per default
                                                          momentumConstraint=None,
                                                          cosmicTrackSplitting=False,
                                                          use_d0cut=False,
                                                          )
     
elif (config["validation"]["theRefitter"] == cms.untracked.string("standard")):

     print(">>>>>>>>>> testPVValidation_cfg.py: msg%-i: using the standard single refit sequence!")         

     ####################################################################
     # Load and Configure TrackRefitter
     ####################################################################
     process.load("RecoTracker.TrackProducer.TrackRefitters_cff")
     import RecoTracker.TrackProducer.TrackRefitters_cff
     process.FinalTrackRefitter = RecoTracker.TrackProducer.TrackRefitter_cfi.TrackRefitter.clone()
     process.FinalTrackRefitter.src = config["validation"]["trackcollection"]
     process.FinalTrackRefitter.TrajectoryInEvent = True
     process.FinalTrackRefitter.NavigationSchool = ''
     process.FinalTrackRefitter.TTRHBuilder=config["validation"].get("tthrbuilder", "WithAngleAndTemplate")
     #TODO check if these options in the standard sequence should still be enabled from the config

     ####################################################################
     # Sequence
     ####################################################################
     process.seqTrackselRefit = cms.Sequence(process.offlineBeamSpot*
                                             # in case NavigatioSchool is set !='' 
                                             #process.MeasurementTrackerEvent*
                                             process.FinalTrackRefitter)     

####################################################################
# Output file
####################################################################
process.TFileService = cms.Service("TFileService",
            fileName = cms.string("{}/PV.root".format(config["output"])),
            closeFileFast = cms.untracked.bool(True),
    )


#########continue from here!!!!!!!!!!!!!!
####################################################################
# Deterministic annealing clustering
####################################################################
if config["validation"]["isda"]:
     print ">>>>>>>>>> testPVValidation_cfg.py: msg%-i: Running DA Algorithm!"
     process.PVValidation = cms.EDAnalyzer("PrimaryVertexValidation",
                                           TrackCollectionTag = cms.InputTag("FinalTrackRefitter"),
                                           VertexCollectionTag = cms.InputTag(config["validation"]["vertexcollection"]), #config
                                           Debug = cms.bool(False),
                                           storeNtuple = cms.bool(False),
                                           useTracksFromRecoVtx = cms.bool(False),
                                           isLightNtuple = cms.bool(True),
                                           askFirstLayerHit = cms.bool(False),
                                           forceBeamSpot = cms.untracked.bool(config["validation"].get("forceBeamSpot", False)), #config
                                           probePt  = cms.untracked.double(config["validation"].get("ptCut", 3)), #config
                                           minPt   = cms.untracked.double(1.), #config
                                           maxPt   = cms.untracked.double(30.), #config
                                           #probeEta = cms.untracked.double(.oO[etaCut]Oo.),
                                           #doBPix   = cms.untracked.bool(.oO[doBPix]Oo.),
                                           #doFPix   = cms.untracked.bool(.oO[doFPix]Oo.),
                                           #numberOfBins = cms.untracked.int32(.oO[numberOfBins]Oo.),
                                           runControl = cms.untracked.bool(config["validation"].get("runControl", True)),
                                           runControlNumber = cms.untracked.vuint32(config["validation"].get("runboundary", 1)), #config
                                           
                                           TkFilterParameters = cms.PSet(algorithm=cms.string('filter'),                           
                                                                         maxNormalizedChi2 = cms.double(5.0),                        # chi2ndof < 5                  
                                                                         minPixelLayersWithHits = cms.int32(2),                      # PX hits > 2                       
                                                                         minSiliconLayersWithHits = cms.int32(5),                    # TK hits > 5  
                                                                         maxD0Significance = cms.double(5.0),                        # fake cut (requiring 1 PXB hit)     
                                                                         minPt = cms.double(0.0),                                    # better for softish events                        
                                                                         maxEta = cms.double(5.0),                                   # as per recommendation in PR #18330
                                                                         trackQuality = cms.string("any")
                                                                         ),

                                           ## MM 04.05.2017 (use settings as in: https://github.com/cms-sw/cmssw/pull/18330) #does this still apply!!!!!!!!!!!!!!!!!!!!!!
                                           TkClusParameters=cms.PSet(algorithm=cms.string('DA_vect'),
                                                                     TkDAClusParameters = cms.PSet(coolingFactor = cms.double(0.6),  # moderate annealing speed
                                                                                                   Tmin = cms.double(2.0),           # end of vertex splitting
                                                                                                   Tpurge = cms.double(2.0),         # cleaning
                                                                                                   Tstop = cms.double(0.5),          # end of annealing
                                                                                                   vertexSize = cms.double(0.006),   # added in quadrature to track-z resolutions
                                                                                                   d0CutOff = cms.double(3.),        # downweight high IP tracks
                                                                                                   dzCutOff = cms.double(3.),        # outlier rejection after freeze-out (T<Tmin)
                                                                                                   zmerge = cms.double(1e-2),        # merge intermediat clusters separated by less than zmerge
                                                                                                   uniquetrkweight = cms.double(0.8) # require at least two tracks with this weight at T=Tpurge
                                                                                                   )
                                                                     )
                                           )

####################################################################
# GAP clustering
####################################################################
else:
     print ">>>>>>>>>> testPVValidation_cfg.py: msg%-i: Running GAP Algorithm!"
     process.PVValidation = cms.EDAnalyzer("PrimaryVertexValidation",
                                           TrackCollectionTag = cms.InputTag("FinalTrackRefitter"),
                                           VertexCollectionTag = cms.InputTag(config["validation"]["vertexcollection"]), #config
                                           Debug = cms.bool(False),
                                           isLightNtuple = cms.bool(True),
                                           storeNtuple = cms.bool(False),
                                           useTracksFromRecoVtx = cms.bool(False),
                                           askFirstLayerHit = cms.bool(False),
                                           forceBeamSpot = cms.untracked.bool(config["validation"]["forceBeamSpot"]), #config
                                           probePt = cms.untracked.double(config["validation"].get("ptCut", 3)), #config
                                           minPt   = cms.untracked.double(1.), #config
                                           maxPt   = cms.untracked.double(30.), #config
                                           #probeEta = cms.untracked.double(.oO[etaCut]Oo.),
                                           #doBPix   = cms.untracked.bool(.oO[doBPix]Oo.),
                                           #doFPix   = cms.untracked.bool(.oO[doFPix]Oo.),
                                           #numberOfBins = cms.untracked.int32(.oO[numberOfBins]Oo.),
                                           runControl = cms.untracked.bool(config["validation"].get(runControl, True)), #config
                                           runControlNumber = cms.untracked.vuint32(int(config["validation"].get("runboundary", 1))), #config

                                           TkFilterParameters = cms.PSet(algorithm=cms.string('filter'),
                                                                         maxNormalizedChi2 = cms.double(5.0),                        # chi2ndof < 20
                                                                         minPixelLayersWithHits=cms.int32(2),                        # PX hits > 2
                                                                         minSiliconLayersWithHits = cms.int32(5),                    # TK hits > 5
                                                                         maxD0Significance = cms.double(5.0),                        # fake cut (requiring 1 PXB hit)
                                                                         minPt = cms.double(0.0),                                    # better for softish events
                                                                         maxEta = cms.double(5.0),                                   # as per recommendation in PR #18330 #does it still apply!!!!!
                                                                         trackQuality = cms.string("any")
                                                                         ),

                                           TkClusParameters = cms.PSet(algorithm   = cms.string('gap'),
                                                                       TkGapClusParameters = cms.PSet(zSeparation = cms.double(0.2)  # 0.2 cm max separation betw. clusters
                                                                                                      )
                                                                       )
                                           )


####################################################################
# Path
####################################################################
process.p = cms.Path(process.goodvertexSkim*process.seqTrackselRefit*process.PVValidation)
