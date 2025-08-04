#!/bin/bash
# Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
# SPDX-License-Identifier: GPL-3.0-only
# First commited by Florian Uhlig

infile=$1
outfile=$2
builddir=$3

CLANG_TIDY_BIN=${CLANG_TIDY_BIN:-clang-tidy}

# special case when a file was deleted
# don't run the test in such a case
if [ ! -e $infile ]; then
  exit 0
fi

extension=${infile##*.}

# Only check source or header files
if [ "$extension" == "h" -o "$extension" == "cxx" ]; then
  echo "Checking file $infile"
else
  exit 0
fi

# Don't do anythink for LinkDef files
if [[ $infile =~ "LinkDef" ]]; then
  exit 0
fi

# Check if the file is a target in the compilation database or a header file
# for which the corresponding source file is in the compilation database
if [[ "$extension" == "h" ]]; then
  checkfile=${infile%.*}.cxx
else
  checkfile=$infile
fi

file=$(grep '"file"' $builddir/compile_commands.json | grep "$checkfile" | cut -d: -f2 | cut -d\" -f2)

if [[ "$file" != "" ]]; then
  # create directory if it not yet exist
  dir="$(dirname $outfile)"
  mkdir -p $dir
  OUTPUT=$($CLANG_TIDY_BIN -p $builddir $file 2> $outfile)
else
  OUTPUT=""
fi

if [ "$OUTPUT" == "" ]; then
  exit 0
else
  echo "ERROR: clang-tidy check failed for file $infile. Suggested changes:" > $outfile
  echo  >> $outfile
  echo "$OUTPUT" >> $outfile
  echo "ERROR: clang-tidy check failed for file $infile. Suggested changes:"
  echo
  echo "$OUTPUT"
  exit 1
fi
