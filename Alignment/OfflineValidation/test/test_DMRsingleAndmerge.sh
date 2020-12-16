#! /bin/bash

function die { echo $1: status $2 ; exit $2; }

echo "TESTING Alignment/DMR single configuration with json..."
cd $CMSSW_BASE/src/Alignment/OfflineValidation/test/test_yaml_new/DMR/single/testUnits/unitTest/317655/
./cmsRun validation_cfg.py config=validation.json || die "Failure running DMR single configuration with json" $?

echo "TESTING Alignment/DMR single configuration standalone..."
./cmsRun validation_cfg.py || die "Failure running DMR single configuration standalone" $?

echo "TESTING DMR merge step"
cd $CMSSW_BASE/src/Alignment/OfflineValidation/test/test_yaml_new/DMR/merge/testUnits/317655/
./DMRmerge validation.json || die "Failure running DMR merge step" $?
