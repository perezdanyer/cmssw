#! /bin/bash

function die { echo $1: status $2 ; exit $2; }

echo "TESTING Alignment/PV single configuration with json..."
cd $CMSSW_BASE/src/Alignment/OfflineValidation/test/test_yaml_new/PV/single/testUnits/unitTest/1/
./cmsRun validation_cfg.py config=validation.json || die "Failure running PV single configuration with json" $?

echo "TESTING Alignment/PV single configuration standalone..."
./cmsRun validation_cfg.py || die "Failure running PV single configuration standalone" $?

echo "TESTING PV merge step"
cd $CMSSW_BASE/src/Alignment/OfflineValidation/test/test_yaml_new/PV/merge/testUnits/1/
./PVmerge validation.json --verbose || die "Failure running PV merge step" $?
