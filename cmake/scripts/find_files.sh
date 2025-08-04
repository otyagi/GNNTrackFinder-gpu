#!/bin/bash
# Copyright (C) 2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
# SPDX-License-Identifier: GPL-3.0-only
# First commited by Florian Uhlig

BASE_COMMIT=${1:-HEAD}

FILES=$(git diff --name-only $BASE_COMMIT | grep -E '.*\.(h|hpp|c|C|cpp|cxx|tpl)$' | grep -viE '.*LinkDef.h$')

echo $FILES
