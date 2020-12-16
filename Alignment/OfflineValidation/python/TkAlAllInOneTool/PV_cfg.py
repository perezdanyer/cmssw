import FWCore.ParameterSet.Config as cms
import FWCore.PythonUtilities.LumiList as LumiList
from PhysicsTools.PatAlgos.patInputFiles_cff import filesRelValTTbarPileUpGENSIMRECO

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
if options.config == "":
    config = {"validation": {},
              "alignment": {}}
else:
    with open(options.config, "r") as configFile:
        config = json.load(configFile)

isDA = config["validation"].get("isda", True)
isMC = config["validation"].get("ismc", True)

runboundary = config["validation"].get("runboundary", 1)

##Read filenames from given TXT file and define input source
readFiles = []

if "dataset" in config["validation"]:
    with open(config["validation"]["dataset"], "r") as datafiles:
        for fileName in datafiles.readlines():
            readFiles.append(fileName.replace("\n", ""))

    process.source = cms.Source("PoolSource",
                                fileNames = cms.untracked.vstring(readFiles),
                                skipEvents = cms.untracked.uint32(0)
                            )
else:
    print ">>>>>>>>>> PV_cfg.py: msg%-i: config not specified! Loading default MC simulation -> filesRelValTTbarPileUpGENSIMRECO!"
    process.source = cms.Source("PoolSource",
                                fileNames = filesRelValTTbarPileUpGENSIMRECO,
                                skipEvents = cms.untracked.uint32(0)
                            )

##Get good lumi section and load data or handle MC
if "goodlumi" in config["validation"]:
    if os.path.isfile(config["validation"]["goodlumi"]):
        goodLumiSecs = cms.untracked.VLuminosityBlockRange(LumiList.LumiList(filename = config["validation"]["goodlumi"]).getCMSSWString().split(','))
        
    else:
        print("Does not exist: {}. Continue without good lumi section file.")
        goodLumiSecs = cms.untracked.VLuminosityBlockRange()

else:
    goodLumiSecs = cms.untracked.VLuminosityBlockRange()

if isMC:
     print ">>>>>>>>>> PV_cfg.py: msg%-i: This is simulation!"
     runboundary = 1
else:
     process.source.lumisToProcess = goodLumiSecs

isMultipleRuns=False
if(isinstance(runboundary, (list, tuple))):
     isMultipleRuns=True
     print "Multiple Runs are selected"       
if(isMultipleRuns):
     process.source.firstRun = cms.untracked.uint32(runboundary[0])
else:
     process.source.firstRun = cms.untracked.uint32(runboundary)

##default set to 1 for unit tests
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(config["validation"].get("maxevents", 1)))

##Bookeeping
process.options = cms.untracked.PSet(
   wantSummary = cms.untracked.bool(False),
   Rethrow = cms.untracked.vstring("ProductNotFound"),
   fileMode  =  cms.untracked.string('NOMERGE'),
)

process.load("FWCore.MessageLogger.MessageLogger_cfi")
process.MessageLogger = cms.Service("MessageLogger",
       destinations   = cms.untracked.vstring('cerr'),
       cerr       = cms.untracked.PSet(
                    threshold = cms.untracked.string('ERROR')
        )

)

##Basic modules
process.load("RecoVertex.BeamSpotProducer.BeamSpot_cff")
process.load("Configuration.Geometry.GeometryRecoDB_cff") #or process.load("Configuration.Geometry.GeometryDB_cff")?????
process.load('Configuration.StandardSequences.Services_cff')
process.load("Configuration.StandardSequences.MagneticField_cff")

####################################################################
# Produce the Transient Track Record in the event
####################################################################
process.load("TrackingTools.TransientTrack.TransientTrackBuilder_cfi")

####################################################################
# Load and Configure Common Track Selection and refitting sequence
####################################################################
import Alignment.CommonAlignment.tools.trackselectionRefitting as trackselRefit
process.seqTrackselRefit = trackselRefit.getSequence(process,
                                                     config["validation"].get("trackcollection", "generalTracks"),
                                                     isPVValidation=True,
                                                     TTRHBuilder=config["validation"].get("tthrbuilder", "WithAngleAndTemplate"),
                                                     usePixelQualityFlag=config["validation"].get("usePixelQualityFlag", True),
                                                     openMassWindow=False,
                                                     cosmicsDecoMode=True,
                                                     cosmicsZeroTesla=config["validation"].get("cosmicsZeroTesla", False),                                                     
                                                     momentumConstraint=None,
                                                     cosmicTrackSplitting=False,
                                                     use_d0cut=False,
                                                     )

#Global tag
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, config["alignment"].get("globaltag", "auto:phase1_2017_realistic"))

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


####################################################################
# Load and Configure event selection
####################################################################
process.primaryVertexFilter = cms.EDFilter("VertexSelector",
                                           src = cms.InputTag(config["validation"].get("vertexcollection", "offlinePrimaryVertices")),
                                           cut = cms.string("!isFake && ndof > 4 && abs(z) <= 24 && position.Rho <= 2"),
                                           filter = cms.bool(True)
                                           )

process.noscraping = cms.EDFilter("FilterOutScraping",
                                  applyfilter = cms.untracked.bool(True),
                                  src = cms.untracked.InputTag(config["validation"].get("trackcollection", "generalTracks")),
                                  debugOn = cms.untracked.bool(False),
                                  numtrack = cms.untracked.uint32(10),
                                  thresh = cms.untracked.double(0.25)
                                  )

process.load("Alignment.CommonAlignment.filterOutLowPt_cfi")
process.filterOutLowPt.src = cms.untracked.InputTag(config["validation"].get("trackcollection", "generalTracks"))
process.filterOutLowPt.ptmin = cms.untracked.double(config["validation"].get("ptCut", 3.))
process.filterOutLowPt.runControl = False
if(isMultipleRuns):
     process.filterOutLowPt.runControlNumber.extend((runboundary))
else:
     process.filterOutLowPt.runControlNumber = [runboundary]

if isMC:
     process.goodvertexSkim = cms.Sequence(process.noscraping + process.filterOutLowPt)
else:
     process.goodvertexSkim = cms.Sequence(process.primaryVertexFilter + process.noscraping + process.filterOutLowPt)


####################################################################
# Deterministic annealing clustering
####################################################################
if isDA:
     print ">>>>>>>>>> PV_cfg.py: msg%-i: Running DA Algorithm!"
     process.PVValidation = cms.EDAnalyzer("PrimaryVertexValidation",
                                           TrackCollectionTag = cms.InputTag("FinalTrackRefitter"),
                                           VertexCollectionTag = cms.InputTag(config["validation"].get("vertexcollection", "offlinePrimaryVertices")),
                                           Debug = cms.bool(False),
                                           storeNtuple = cms.bool(False),
                                           useTracksFromRecoVtx = cms.bool(False),
                                           isLightNtuple = cms.bool(True),
                                           askFirstLayerHit = cms.bool(False),
                                           forceBeamSpot = cms.untracked.bool(config["validation"].get("forceBeamSpot", False)),
                                           probePt  = cms.untracked.double(config["validation"].get("ptCut", 3)),
                                           probeEta  = cms.untracked.double(config["validation"].get("etaCut", 2.5)),
                                           minPt  = cms.untracked.double(config["validation"].get("minPt", 1.)),
                                           maxPt  = cms.untracked.double(config["validation"].get("maxPt", 30.)),
                                           doBPix   = cms.untracked.bool(config["validation"].get("doBPix", True)),
                                           doFPix   = cms.untracked.bool(config["validation"].get("doFPix", True)),
                                           numberOfBins = cms.untracked.int32(config["validation"].get("numberOfBins", 48)),
                                           runControl = cms.untracked.bool(config["validation"].get("runControl", False)),
                                           runControlNumber = cms.untracked.vuint32(runboundary),
                                           
                                           TkFilterParameters = cms.PSet(algorithm=cms.string('filter'),                           
                                                                         maxNormalizedChi2 = cms.double(5.0),                        # chi2ndof < 5                  
                                                                         minPixelLayersWithHits = cms.int32(2),                      # PX hits > 2                       
                                                                         minSiliconLayersWithHits = cms.int32(5),                    # TK hits > 5  
                                                                         maxD0Significance = cms.double(5.0),                        # fake cut (requiring 1 PXB hit)     
                                                                         minPt = cms.double(0.0),                                    # better for softish events                        
                                                                         maxEta = cms.double(5.0),                                   # as per recommendation in PR #18330
                                                                         trackQuality = cms.string("any")
                                                                         ),

                                           ## MM 04.05.2017 (use settings as in: https://github.com/cms-sw/cmssw/pull/18330)
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
     print ">>>>>>>>>> PV_cfg.py: msg%-i: Running GAP Algorithm!"
     process.PVValidation = cms.EDAnalyzer("PrimaryVertexValidation",
                                           TrackCollectionTag = cms.InputTag("FinalTrackRefitter"),
                                           VertexCollectionTag = cms.InputTag(config["validation"].get("vertexcollection", "offlinePrimaryVertices")),
                                           Debug = cms.bool(False),
                                           isLightNtuple = cms.bool(True),
                                           storeNtuple = cms.bool(False),
                                           useTracksFromRecoVtx = cms.bool(False),
                                           askFirstLayerHit = cms.bool(False),
                                           forceBeamSpot = cms.untracked.bool(config["validation"].get("forceBeamSpot", False)),
                                           probePt = cms.untracked.double(config["validation"].get("ptCut", 3)),
                                           probeEta  = cms.untracked.double(config["validation"].get("etaCut", 2.5)),
                                           minPt  = cms.untracked.double(config["validation"].get("minPt", 1.)),
                                           maxPt  = cms.untracked.double(config["validation"].get("maxPt", 30.)),
                                           doBPix   = cms.untracked.bool(config["validation"].get("doBPix", True)),
                                           doFPix   = cms.untracked.bool(config["validation"].get("doFPix", True)),
                                           numberOfBins = cms.untracked.int32(config["validation"].get("numberOfBins", 48)),
                                           runControl = cms.untracked.bool(config["validation"].get(runControl, False)),
                                           runControlNumber = cms.untracked.vuint32(int(runboundary)),

                                           TkFilterParameters = cms.PSet(algorithm=cms.string('filter'),
                                                                         maxNormalizedChi2 = cms.double(5.0),                        # chi2ndof < 20
                                                                         minPixelLayersWithHits=cms.int32(2),                        # PX hits > 2
                                                                         minSiliconLayersWithHits = cms.int32(5),                    # TK hits > 5
                                                                         maxD0Significance = cms.double(5.0),                        # fake cut (requiring 1 PXB hit)
                                                                         minPt = cms.double(0.0),                                    # better for softish events
                                                                         maxEta = cms.double(5.0),                                   # as per recommendation in PR #18330
                                                                         trackQuality = cms.string("any")
                                                                         ),

                                           TkClusParameters = cms.PSet(algorithm   = cms.string('gap'),
                                                                       TkGapClusParameters = cms.PSet(zSeparation = cms.double(0.2)  # 0.2 cm max separation betw. clusters
                                                                                                      )
                                                                       )
                                           )


####################################################################
# Output file
####################################################################
process.TFileService = cms.Service("TFileService",
            fileName = cms.string("{}/PV.root".format(config.get("output", os.getcwd()))),
            closeFileFast = cms.untracked.bool(True),
    )

####################################################################
# Path
####################################################################
process.p = cms.Path(process.goodvertexSkim*process.seqTrackselRefit*process.PVValidation)

print("Done")
