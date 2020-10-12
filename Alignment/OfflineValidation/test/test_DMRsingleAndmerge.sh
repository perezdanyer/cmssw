#! /bin/bash

function die { echo $1: status $2 ; exit $2; }

cd $CMSSW_BASE/src/Alignment/OfflineValidation/test

echo "TESTING Alignment/OfflineValidation..."
test_yaml/DMR/single/Legacy/ULRun2/316569/cmsRun test_yaml/DMR/single/Legacy/ULRun2/316569/validation_cfg.py config=test_yaml/DMR/single/Legacy/ULRun2/316569/validation.json
test_yaml/DMR/single/Legacy/ReReco/316569/cmsRun test_yaml/DMR/single/Legacy/ReReco/316569/validation_cfg.py config=test_yaml/DMR/single/Legacy/ReReco/316569/validation.json

#cmsRun testDMR_ULRun2_cfg.py || die "Failure running testDMR_ULRun2_cfg.py" $?
#cmsRun testDMR_ReReco_cfg.py || die "Failure running testDMR_ReReco_cfg.py" $?

echo "TESTING DMR merge step"
test_yaml/DMR/merge/Legacy/316569/DMRmerge test_yaml/DMR/merge/Legacy/316569/validation.json || die "Failure running DMR merge step" $?
