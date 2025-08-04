#!/bin/bash
# Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
# SPDX-License-Identifier: GPL-3.0-only
# First commited by Pierre-Alain Loizeau

# Script used to compute a "safe" end time for the ctest test stage of a Dart/Cdash chain in order
# to have enough time to upload the CDASH results before a job is killed on a SLURM cluster (e.g. Virgo @ GSI)
#
# Usage:
# - with a full/relative/absolute path to an output Dart.cfg file as parameter, where the export will be added
# - without any parameter, then setting an export
#
# => Typical use case would be calling it in your sbatch script after setting up the other parts of the
#    Dart.cfg gile and before the call to Dart.sh
#>> echo "export NCPU=10" >> ${DART_CFG}
#>> [...]
#>> ./scripts/find_slurm_ctest_stop_time.sh ${DART_CFG}
#>> [...]
#>> ./Dart.sh Weekly ./${DART_CFG} &> ${OUT_DIR}/Dart_weekly.log

# SLURM end time
# 1. From SLURM env variables if possible (virgo3)
if [[ -n ${SLURM_JOB_END_TIME} ]]; then
  echo "Env variable SLURM_JOB_END_TIME=${SLURM_JOB_END_TIME}"
  SLURM_JOB_END_TIME_DATE=`date -d "@${SLURM_JOB_END_TIME}"`
  echo "SLURM_JOB_END_TIME in human format: ${SLURM_JOB_END_TIME_DATE}"
  CTEST_END_TIME_LIMIT=`date -d "${SLURM_JOB_END_TIME_DATE} -15minutes" +"%H:%M:%S"`
else
  # 2. From what squeue return if not possible directly from environment (virgo2)
  # ==> Broken from bare-metal submit nodes end May 2024, maybe bug?
  END_TIME=`squeue -j ${SLURM_JOB_ID} -h --Format EndTime`
  CET_CEST=`date +"%Z"`
  echo "SLURM Job end time ${END_TIME} probably ${CET_CEST}"

  # Ctest end time = SLURM -5 minutes (should be enough for coverage and uploads)
  # => Not working as ctest expects the "time point in hours-minutes-seconds within current day"
  #                         + hardcode the day/year/month to the current day...
  # https://gitlab.kitware.com/cmake/cmake/-/blob/master/Help/manual/ctest.1.rst#L440
  # https://gitlab.kitware.com/cmake/cmake/-/blob/master/Source/cmCTest.cxx#L1991
  # https://gitlab.kitware.com/cmake/cmake/-/blob/master/Source/cmCTest.cxx#L3012
  # https://gitlab.kitware.com/cmake/cmake/-/blob/master/Source/CTest/cmCTestRunTest.cxx#L811
  # => Does not work for long running tests close to end of the day
  # CTEST_END_TIME_LIMIT=`date -d "${END_TIME}CET -15minutes" +"%Y-%m-%d %H:%M:%S %z"`
  # => But following should be ok for weeklies starting early in the morning
  # => Does not work at least with cmake 3.22 and 3.24, seesm that timezone is breaking decoding
  #    (tried all timezone options of date, with and without space between time and timezone)
  #CTEST_END_TIME_LIMIT=`date -d "${END_TIME}CET -15minutes" +"%H:%M:%S %z"`
  # => Working! but probably unsafe on day of Summer time swaps + if tests run close to midnight...
  # => Fixed for summer time but will probably still fail on the day of the swap itself... so twice a year
  CTEST_END_TIME_LIMIT=`date -d "${END_TIME}${CET_CEST} -15minutes" +"%H:%M:%S"`
fi

echo "Setting the job time limit for ctest to ${CTEST_END_TIME_LIMIT} to make sure CDASH data is uploaded"
if [[ $# -eq 1 ]]; then
  # Set the export in the chosen Dart.cfg file
  echo "export CTEST_END_TIME_LIMIT=\"${CTEST_END_TIME_LIMIT}\"" >> $1
else
  export CTEST_END_TIME_LIMIT=\"${CTEST_END_TIME_LIMIT}\"
fi
