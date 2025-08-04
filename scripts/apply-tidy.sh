#!/bin/bash
# Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
# SPDX-License-Identifier: GPL-3.0-only
# First commited by Florian Uhlig

# Extract files with an existing compilation unit from the compile command database
# which are in the passed directory or in case only one file is passed check if the
# file is in the compilation database
# For all found files run clang-tidy to fix the problems found. Exclude files from
# the external directory and root-cling generated dictionaries. 

source_directory=$1
build_directory=$2

CLANG_TIDY_BIN=${FAIRROOT_CLANG_TIDY_BIN:-clang-tidy}

FILES=$(grep '"file"' $build_directory/compile_commands.json | grep "$source_directory" | cut -d: -f2 | cut -d\" -f2)

for file in $FILES; do
  if [[ $file =~ "/external/" ]]; then
    continue
  fi
  if [[ $file =~ "G__" ]]; then
    continue
  fi
  echo "Run clang-tidy on $file"
  $CLANG_TIDY_BIN --fix -p $build_directory  $file
done;
