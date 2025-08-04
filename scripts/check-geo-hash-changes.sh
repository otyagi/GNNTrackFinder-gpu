#!/bin/bash
# Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
# SPDX-License-Identifier: GPL-3.0-only
# First commited by Pierre-Alain Loizeau

# This script will check if the Geometry repository installation script (externals) holds any modification relative to
# the "upstream" repository leading to modifications of the setup files.
# If it is the case, it will check if the hash was changed.
# If the hash was changed, it will check if setup files were modifed between the old and new hash values.
#
# The result of the check is echoed with a custom pattern to be extracted by CMAKE into the variable GEO_HASH_CHANGE
# The eventual list (; sep) of modified setups is similarly echoed for the variable CHANGED_SETUPS
# The custom pattern extracted in the `geometry/InstallGeometry.cmake` file is
# CMAKE_EXPORT <VAR_NAME>=<VAR_VAL or <VAR_VAL_LIST_1:VAR_VAL_LIST_2:...:>
#
# upstream is used here in the CBM workflow and CBM CI meaning
# => designating the target branch of an MR or the official master branch
# => Not your own fork in most use cases and the script is mostly meant to be auto-called
# => Be careful if calling by hand connect_upstream_repo as it updates the remote called upstream if different!!!
#    This is typically visible by a long list of "[new branch]" when this new remote if fetched!

if [[ $# -ne 2 ]]; then
  echo "Missing argument! please call this script with both the upstream URL and the upstream branch:"
  echo "./scripts/check-geo-hash-changes.sh <url> <branch>"
  echo ""
  echo "upstream is used here in the CBM workflow and CBM CI meaning"
  echo "=> designating the target branch of an MR or the official master branch"
  echo "=> Not your own fork in most use cases and the script is mostly meant to be auto-called"
  echo "=> Be careful if calling by hand connect_upstream_repo: it updates the remote called upstream if different!!!"
  echo ""
  echo "Examples: "
  echo "./scripts/check-geo-hash-changes.sh https://git.cbm.gsi.de/computing/cbmroot.git master # https without login"
  echo "./scripts/check-geo-hash-changes.sh git@git.cbm.gsi.de:computing/cbmroot.git master     # ssh with key login"
  echo "./scripts/check-geo-hash-changes.sh \$CI_MERGE_REQUEST_PROJECT_URL \$CI_MERGE_REQUEST_TARGET_BRANCH_NAME #MR CI"
  exit -1
fi

# Copied from connect_upstream_repo.sh to avoid replacing an existing upstream remote!
upstream_there=$(git remote -v | grep upstream)
if [ $? -eq 0 ]; then
  upstream_there=$(git remote -v | grep upstream | grep $1)
  if [ $? -ne 0 ]; then
    echo "Remote link upstream already exist and points to a different repo, stopping here!! (not changing local conf.)"
    echo "Existing: "
    git remote -v | grep upstream
    echo "Parameter: "
    echo "upstream	$1"
    exit -1
  fi
fi

scripts/connect_upstream_repo.sh $1 &> build/geo_check_connect_upstream_repo.log  # Store logs in case of failure
git fetch upstream  &> build/geo_check_fetch_upstream_repo.log  # Store logs in case of failure

BASE_COMMIT=upstream/$2
CMAKE_FILE=external/InstallGeometry.cmake

GEO_SETUPS_CHANGE=`git diff --name-only $BASE_COMMIT | grep ${CMAKE_FILE} | wc -l`
if [[ $GEO_SETUPS_CHANGE -eq 0 ]]; then
  echo "No changes found to Geometry repository install script"
else
  echo "Changes found to Geometry repository install script, checking if hash was bumped"
  OLD_HASH=`git diff ${BASE_COMMIT} ${CMAKE_FILE} | grep '\-set' | grep -Po 'GEOMETRY_VERSION \s*\K\S*' | tr -d '()'`
  NEW_HASH=`git diff ${BASE_COMMIT} ${CMAKE_FILE} | grep '\+set' | grep -Po 'GEOMETRY_VERSION \s*\K\S*' | tr -d '()'`

  if [[ -n ${OLD_HASH} && -n ${NEW_HASH} ]]; then
    echo "Hash bumped, checking if some setup files were modified"
    echo "Old hash: ${OLD_HASH}"
    echo "New hash: ${NEW_HASH}"

    PWD_ORIG=`pwd`
    cd geometry
    git fetch origin
    # Use : as temporary separator to avoid troubles with CMAKE strings/lists processing built-in functions
    CHANGED_SETUPS=`git diff --name-only ${OLD_HASH} ${NEW_HASH} | grep setup | tr '\n' ':'`
    cd ${PWD_ORIG}
    if [[ -n ${CHANGED_SETUPS} ]]; then
      echo "Some setups files were found to have been modified"
      echo "Changed setups:"
      echo "CMAKE_EXPORT CHANGED_SETUPS=$CHANGED_SETUPS"  # Only way to pass values from script to parent cmake
    else
      echo "No setup found to have been modified"
      GEO_SETUPS_CHANGE=0
    fi
  else
    echo "Hashes were not changed, modifications are in other part of cmake file"
    GEO_SETUPS_CHANGE=0
  fi
fi
echo "CMAKE_EXPORT GEO_SETUPS_CHANGE=$GEO_SETUPS_CHANGE"  # Only way to pass values from script to parent cmake
