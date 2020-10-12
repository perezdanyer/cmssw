#! /bin/bash

function die { echo $1: status $2 ; exit $2; }

echo "TESTING Alignment/OfflineValidation..."
./${LOCAL_TEST_DIR}/Test/DMR/merge/Legacy/316569/DMRmerge ${LOCAL_TEST_DIR}/testDMRmerge.json
