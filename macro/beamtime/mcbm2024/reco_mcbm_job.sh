#!/bin/bash
# Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
# SPDX-License-Identifier: GPL-3.0-only
# Authors: Sergei Zharko [committer]
#
# @file   reco_mcbm_job.sh
# @brief  Script to run a particular job on VIRGO
# @since  23.01.2025
# @author Sergei Zharko <s.zharko@gsi.de>
#
# The script defines environment to reconstruct time slices vs. a particular job using VIRGO.
# To run the script one has to export a variable MCBM_DATA_DIR -- a path to the real data directory.
#
#
# OPTIONS:
#
# 1. Required:
# (A) SETUP GENERATION:
#    --setup:      Runs in setup generation mode
#    --run <num>   Sets run ID (<num> must be an integer with 4 digits)
# 
# (B) DATA RECONSTRUCTION:
#    --tsa-index <path>   Path to the text file with absolute paths to input TSA files 
#                         Each line of the file corresponds to the job index.
#    --job | -j  <num>    Job index (from 1 to number of files, defined under the option --tsa-index)
#    --run <num>   Sets run ID (<num> must be an integer with 4 digits)
#    
# 
# 2. Optional (in mode (B)):
#    --unpack, --reco, --qa, --qa-module, --kfpf, -n  -- See the reco_mcbm.sh script for details
#

RECO_OPTIONS=""
RUN=""
JOB=""
DO_GENERATE_SETUP=0
TSA_INDEX=
# ------ Option selection ----------------------------------------------------------------------------------------------
while [[ $# -gt 0 ]]; do
  case ${1} in
    --setup )
      DO_GENERATE_SETUP=1
      ;;
    --unpack )
      RECO_OPTIONS="${RECO_OPTIONS} --unpack"
      ;;
    --reco )
      RECO_OPTIONS="${RECO_OPTIONS} --reco"
      ;;
    --qa )
      RECO_OPTIONS="${RECO_OPTIONS} --qa"
      ;;
    --qa-module )
      RECO_OPTIONS="${RECO_OPTIONS} --qa-module"
      ;;
    --kfpf )
      RECO_OPTIONS="${RECO_OPTIONS} --kfpf"
      ;;
    --run )
      RUN=${2}
      ;;
    -n )
      RECO_OPTIONS="${RECO_OPTIONS} -n ${2}"
      ;;
    -j | --job )
      JOB=${2}
      ;;
    --tsa-index )
      TSA_INDEX=${2}
      ;;
  esac
  shift
done

# ------ Variable check ------------------------------------------------------------------------------------------------
if [[ -z "${VMCWORKDIR}" ]]; then
  printf "E- CBM environment is not defined (VMCWORKDIR is not found). Please, configure your CbmRoot\n"
  exit 1
fi

if [[ -z ${RUN} ]]; then
  printf "E- The run identifier is not specified. Please, specify it using the option \"--run <run_id>\"\n"
  exit 2
fi

if [[ -z ${JOB} && ${DO_GENERATE_SETUP} -ne 1 ]]; then 
  printf "E- The job number is not specified. Please set the job number using the option \"-j <job>\"\n"
  exit 3
fi 

if [[ -z ${MCBM_DATA_DIR} ]]; then
  printf "E- The data directory is not identified. Please provide the path to the directory, using the following bash "
  printf "command\n\t export MCBM_DATA_DIR=<path to data directory>\n"
  exit 4
fi

# ------ Script execution ----------------------------------------------------------------------------------------------
mkdir -p ${MCBM_DATA_DIR}

DIR_UNPACK=${MCBM_DATA_DIR}/unpacked_data/${RUN}
DIR_RECO=${MCBM_DATA_DIR}/reconstructed_data/${RUN}
DIR_SETUP=${MCBM_DATA_DIR}/setups
DIR_QA=${MCBM_DATA_DIR}/qa/${RUN}
DIR_KFPF=${MCBM_DATA_DIR}/kfpf/${RUN}

# NOTE: either the setup is generated, or the data are produced.
if [[ ${DO_GENERATE_SETUP} -eq 1 ]]; then
  RECO_OPTIONS=" --setup-only --run ${RUN}"
else 
  if [[ -f ${TSA_INDEX} ]]; then 
    TSA_INDEX=$(realpath ${TSA_INDEX})
    RECO_OPTIONS="${RECO_OPTIONS} --tsa-index-file ${TSA_INDEX}"
  else 
    printf "E- TSA index file ${TSA_INDEX} was not found\n"
    exit 5
  fi
fi

RECO_OPTIONS="${RECO_OPTIONS} --setup-dir ${DIR_SETUP} --unpack-dir ${DIR_UNPACK} --reco-dir ${DIR_RECO} --qa-dir ${DIR_QA} --kfpf-dir ${DIR_KFPF}"

RECO_SCRIPT=${VMCWORKDIR}/macro/beamtime/mcbm2024/reco_mcbm.sh

if [[ ! -x ${RECO_SCRIPT} ]]; then
  printf "E- The reco script (${RECO_SCRIPT}) either does not exist, or is not executable. Please make it executable "
  printf "with the \"chmod +x\" command\n"
  exit 6
fi

${RECO_SCRIPT} ${RECO_OPTIONS}


