#!/bin/bash
# Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
# SPDX-License-Identifier: GPL-3.0-only
# First commited by Pierre-Alain Loizeau

# Script used to compute a "safe" end time for the ctest test stage of a Dart/Cdash chain in order
# to have enough time to upload the CDASH results before a job is killed in a Gitlab CI pipeline
# Uses the GITLAB CI automatically pre-defined variable ${CI_JOB_TIMEOUT}
#
# Usage:
# - with a full/relative/absolute path to an output Dart.cfg file as parameter, where the export will be added
# - without any parameter, then setting an export
#
# => Typical use case would be calling it in your sbatch script after setting up the other parts of the
#    Dart.cfg gile and before the call to Dart.sh
#>> echo "export NCPU=10" >> ${DART_CFG}
#>> [...]
#>> ./scripts/find_cijob_ctest_stop_time.sh ${DART_CFG}
#>> [...]
#>> ./Dart.sh Weekly ./${DART_CFG} &> ${OUT_DIR}/Dart_weekly.log

# Job end time
# 1. Make end time from current time + job timeout (not really precise due to job start time until here but should be
#    good enough)
echo "Env. variable CI_JOB_TIMEOUT = ${CI_JOB_TIMEOUT} s"
CI_JOB_END_TIME_DATE=$(date -d"now +${CI_JOB_TIMEOUT}seconds")

# 2. Remove 5 minutes at the end for safety
echo "Now in human format: $(date +\"%H:%M:%S\")"
echo "CI_END_TIME in human format: ${CI_JOB_END_TIME_DATE}"
CTEST_END_TIME_LIMIT=$(date -d "${CI_JOB_END_TIME_DATE} -5minutes" +"%H:%M:%S")

echo "Setting time limit for ctest to ${CTEST_END_TIME_LIMIT} (5min bef. job TO) to make sure CDASH data is uploaded"
if [[ $# -eq 1 ]]; then
  # Set the export in the chosen Dart.cfg file
  echo "export CTEST_END_TIME_LIMIT=\"${CTEST_END_TIME_LIMIT}\"" >> $1
else
  export CTEST_END_TIME_LIMIT=\"${CTEST_END_TIME_LIMIT}\"
fi
