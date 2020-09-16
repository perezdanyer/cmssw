#!/bin/zsh

cd $CMSSW_BASE/src/Alignment/OfflineValidation/test

echo "Printing help"
validateAlignments.py -h

echo "Running over YAML"
validateAlignments.py -v -f -d testDMR_ULRun2_and_ReReco.yaml

echo "Running over JSON"
validateAlignments.py -v -d -f testDMR_ULRun2_and_ReReco.json
