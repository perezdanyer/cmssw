#! /bin/bash

function die { echo $1: status $2 ; exit $2; }

echo "TESTING Alignment/OfflineValidation ..."
cmsRun ${LOCAL_TEST_DIR}/test_all_cfg.py || die "Failure running test_OfflineValidation_cfg.py" $?
cmsRun ${LOCAL_TEST_DIR}/DiMuonVertexValidation_cfg.py maxEvents=10 || die "Failure running DiMuonVertexValidation_cfg.py" $?
cmsRun ${LOCAL_TEST_DIR}/inspectData_cfg.py unitTest=True || die "Failure running inspectData_cfg.py" $?

## copy into local sqlite file the ideal alignment
echo "COPYING locally Ideal Alignment ..."
conddb --yes --db pro copy TrackerAlignment_Upgrade2017_design_v4 --destdb myfile.db
conddb --yes --db pro copy TrackerAlignmentErrorsExtended_Upgrade2017_design_v0 --destdb myfile.db

echo " TESTING Primary Vertex Validation run-by-run submission ..."
submitPVValidationJobs.py -j UNIT_TEST -D /HLTPhysics/Run2016C-TkAlMinBias-07Dec2018-v1/ALCARECO -i ${LOCAL_TEST_DIR}/testPVValidation_Relvals_DATA.ini -r --unitTest || die "Failure running PV Validation run-by-run submission" $?

printf "\n\n"

echo " TESTING Split Vertex Validation submission ..."
submitPVResolutionJobs.py -j UNIT_TEST -D /JetHT/Run2018C-TkAlMinBias-12Nov2019_UL2018-v2/ALCARECO -i ${LOCAL_TEST_DIR}/PVResolutionExample.ini --unitTest || die "Failure running Split Vertex Validation submission" $?
