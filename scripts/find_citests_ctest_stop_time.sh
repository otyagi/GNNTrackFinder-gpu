#!/bin/bash
# Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
# SPDX-License-Identifier: GPL-3.0-only
# First commited by Pierre-Alain Loizeau

# Script used to compute a "total timeout" end time for the ctest test stage of a Dart/Cdash chain
#
# Usage:
# - without any parameter, result sent to stdout
# - with a single parameter, compare the two dates (after conversion to seconds) and return the earliest one
#   => !!! No check of the format of the parameter before usage, meant to be "HH:MM:SS" !!!
#
# => Typical use case would be calling it in the CDASH/ctest cmake between the build and test stages

# Job end time
# 1. Make end time from current time + job timeout (not really precise due to job start time until here but should be
#    good enough)


CI_TESTS_TOTAL_END_TIME_DATE=$(date -d "now +${CI_TEST_STAGE_TOTAL_TIME_LIMIT}seconds" +"%H:%M:%S")

if [[ $# -eq 1 ]]; then
  if [[ $(date --date="${1}" +%s) -lt $(date --date="${CI_TESTS_TOTAL_END_TIME_DATE}" +%s) ]]; then
    CI_TESTS_TOTAL_END_TIME_DATE=$1
  fi
fi
echo ${CI_TESTS_TOTAL_END_TIME_DATE}

