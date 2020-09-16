#! /bin/bash

function die { echo $1: status $2 ; exit $2; }

echo "TESTING Alignment/OfflineValidation..."
cmsRun ${LOCAL_TEST_DIR}/testDMR_ULRun2_cfg.py || die "Failure running testDMR_ULRun2_cfg.py" $?
cmsRun ${LOCAL_TEST_DIR}/testDMR_ReReco_cfg.py || die "Failure running testDMR_ReReco_cfg.py" $?

echo "TESTING DMR merge step"
${LOCAL_TEST_DIR}/Test/DMR/merge/Legacy/316569/DMRmerge ${LOCAL_TEST_DIR}/testDMRmerge.json
