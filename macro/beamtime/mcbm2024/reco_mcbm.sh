#!/bin/bash
# Copyright (C) 2024-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
# SPDX-License-Identifier: GPL-3.0-only
# Authors: Sergei Zharko [committer], Pierre-Alain Loizeau
#
# @file   reco_mcbm.sh
# @brief  Script to run the TSA file reconstruction in mCBM
# @since  21.05.2024
# @author Sergei Zharko <s.zharko@gsi.de>
#
# **********************************************************************************************************************
# ***                                      User Manual (v. 0.1.0, 22.01.2025)                                        ***
# **********************************************************************************************************************
#
# 1. Definitions
#   <setup>:
#     An official setup tag, e.g. "mcbm_beam_2024_05_08_nickel".
#   <label>:
#     A common part of name bases of the data-files, e.g. "3105_node8_05_0002"
#     NOTE: <label> is strictly determined at least for a particular setup.
#   <top_dir>:
#     A top "data" directory.
#
# 2. Introduction
#
#   This script aims to provide a generic data reconstruction scenario in mCBM, which includes:
#     - setup files generation:      [--setup]
#     - setup files generation:      [--setup-only]  (Special flag, to be called, if the other steps must not be selected)
#     - TSA file unpacking           [--unpack]
#     - reconstruction               [--reco]
#     - main QA                      [--qa]
#     - reconstruciton QA (Alex)     [--qa-module]
#     - KF Particle Finder (Lambda)  [--kfpf]
#
#   Required options:
#     --tsa <path>           Path to the input TSA file
#                             NOTE: must contain a run index as a first integer in its base name.
#     OR
#
#     --tsa-index-file  <file>  A path to a text file with a list of TSAs. The line of the file equals to the job number
#
#     OR
#
#     --run <runID>  Run identifier, BUT ONLY, IF ONLY THE SETUP IS NEEDED (--setup-only)
#
#   Auxiliary options:
#     -n [--nts] <N_TS>  Number of timeslices to procede
#     --setup, --unpack, --reco, --qa, --qa-module
#     --data-dir     <path>  Path to output directory
#     --param-online <path>  Path to the online parameters
#                             NOTE: should be ${VMCWORKDIR}/parameters/online, but local user may have copy for testing
#
#     --setup-dir    <path>  Path to the setup output
#     --unpack-dir   <path>  Path to the unpacking output
#     --reco-dir     <path>  Path to the reconstruction output
#     --qa-dir       <path>  Path to the QA output
#     --kfpf-dir     <path>  Path to the KFPF output
#     NOTE: if any of these parameters are not provided, the corresponding data will be stored to the <top_dir>. If the 
#           corresponding path is a relative path (!=realpath), the <top_dir> will be selected is the parent directory
#
#   Options for running on the batch farm:
#     -j [--job] <jobId>        Index of the job (default == 1)
#        
#     --disable-logs  Disables storing output in the temporary log files   
#
# 3. File names involved:
#   <geo>:       Input geometry file:  <top_dir>/<setup>.geo.root
#   <par>:       Input parameter file:  <top_dir>/<setup>.par.root --copy--> <top_dir>/<label>.par.root
#   <tra>:       Dummy transport file: <top_dir>/<setup>.tra.root
#   <digi>:      Digitization output: <top_dir>/<label>.digi.root --ln--> <top_dir>/<label>.raw.root
#   <reco>:      Reconstruction output: <top_dir>/<label>.reco.root --ln--> <top_dir>/<label>.rec.root
#   <qa>:        Main QA output: <top_dir>/<label>.qa.root
#   <qa-module>: Reconstruction module QA: <top_dir>/<label>.rqa.root
#   <kfp.ana>:   Output from the lambda analysis: <top_dir>/<label>.kfp.ana.root
#                [optional] QA file:  <top_dir>/<label>.qa.kfp.ana.root
#
# 4. Knwon missing features
#
#   - Script and its sub-process is not killed if CTEST is stopped with CTRL+C
#   - Error in reco of run 3105:
# CbmAlgoBuildRawEvents::filterBmon: Bmon digi with wrong address [GetSmType() != 5]. Skip.
#   - LOG to change from INFO to DEBUG in RECO CA step:
#[2024-07-08 10:11:10.301054] [0x00007f96766b4a40] [info]    ca: Hit out of range: iHit = 35, time = 126.389 ms, window = 1
#
#   - LOG to change from INFO to DEBUG in RECO CA step:
# =======  Ca Track finder: process event 17297 ...
# [2024-07-08 10:11:10.375592] [0x00007f96766b4a40] [info]    CA tracker process time slice 127.431 -- 127.431 [ms] with 47 hits
# [2024-07-08 10:11:10.375597] [0x00007f96766b4a40] [info]    Fraction of hits from monster events: 0
# [2024-07-08 10:11:10.375603] [0x00007f96766b4a40] [info]    Thread: 0 from 127.431 ms  to 127.431 ms (delta = 0.00012 ms)
# [2024-07-08 10:11:10.376800] [0x00007f96766b4a40] [info]    CA tracker: time slice finished. Reconstructed 4 tracks with 16 hits. Processed 47 hits in 1 time windows. Reco time 1.21725e-12 s
# Ca Track Finder finished, found 4 tracks
# CA Track Finder: 0.00121725 s/sub-ts
# Tracking performance... done
# End of CA
# CA: N hits used/tot = 46/49
#
#   - LOG to change from INFO to DEBUG in QA step: [root.exe][10:11:24][INFO] CA: N hits used/tot = 17/18
#
# *********************
# ** USER PARAMETERS **
# *********************

# Data directory
DATA_TOP_DIR='./data'
DISABLE_LOGS=0

# Subsystem flags
RECO_MVD=0
RECO_STS=1
RECO_TRD=1
RECO_TRD2d=1
RECO_RICH=1
RECO_MUCH=0
RECO_TOF=1
RECO_FSD=0
RECO_PSD=0

# Algorithm flags
RECO_TOFtr=0
RECO_ALI=1
RECO_EvB=1
RECO_CA=1
RECO_PV=1
RECO_QA=0
LAMBDA_MIXED_EVENT=0


# ----------------------------------------------------------------------------------------------------------------------
# ----------------------------------------------------------------------------------------------------------------------


# *************************
# ** Parameter selection **
# *************************

# ----- Execution steps
I_WANT_ONLY_SETUP=0
DO_CREATE_SETUP=0
DO_UNPACK=0
DO_RECO=0
DO_QA=0         # Main QA Macro
DO_QA_MODULE=0  # QA Macro by Alexandru
DO_LAMBDA=0     # Lambda analysis using KFParticleFinder
STEPS_TO_PRINT=""

ONLINE_PAR=${VMCWORKDIR}/parameters/online


# ----- Subdirectories
DIR_SETUP=""
DIR_UNPACK=""
DIR_RECO=""
DIR_QA=""
DIR_KFPF=""

# ----- Run information
RUN=-1
RUN_IF_ONLY_SETUP_NEEDED=0
TSA=
N_TS=-1
TSA_INDEX=0
JOB_ID=1
while [[ $# -gt 0 ]]; do
  case ${1} in
    --setup )
      DO_CREATE_SETUP=1
      STEPS_TO_PRINT="${STEPS_TO_PRINT} --setup"
      ;;
    --unpack )
      DO_UNPACK=1
      STEPS_TO_PRINT="${STEPS_TO_PRINT} --unpack"
      ;;
    --reco )
      DO_RECO=1
      STEPS_TO_PRINT="${STEPS_TO_PRINT} --reco"
      ;;
    --qa )
      DO_QA=1
      STEPS_TO_PRINT="${STEPS_TO_PRINT} --qa"
      ;;
    --qa-module )
      DO_QA_MODULE=1
      STEPS_TO_PRINT="${STEPS_TO_PRINT} --qa-module"
      ;;
    --kfpf )
      DO_LAMBDA=1
      STEPS_TO_PRINT="${STEPS_TO_PRINT} --kfpf"
      ;;
    --tsa )
      TSA=${2}
      ;;
    --tsa-index-file )
      TSA_INDEX=${2}
      ;;
    -j | --job )
      JOB_ID=${2}
      ;;
    --param-online )
      ONLINE_PAR=${2}
      ;;
    -n | --nts )
      N_TS=${2}
      ;;
    --data-dir )
      DATA_TOP_DIR=${2}
      ;;
    --setup-dir )
      DIR_SETUP=${2}
      ;;
    --unpack-dir )
      DIR_UNPACK=${2}
      ;;
    --reco-dir )
      DIR_RECO=${2}
      ;;
    --qa-dir )
      DIR_QA=${2}
      ;;
    --kfpf-dir )
      DIR_KFPF=${2}
      ;;
    --setup-only )
      I_WANT_ONLY_SETUP=1
      ;;
    --run )
      RUN_IF_ONLY_SETUP_NEEDED=${2}
      ;;
    --disable-logs )
      DISABLE_LOGS=1
      ;;
  esac
  shift
done


# ------ Re-define the paths to step directories
DATA_TOP_DIR=$(realpath -m ${DATA_TOP_DIR})

if [[ -z ${DIR_SETUP} ]]; then 
  DIR_SETUP=${DATA_TOP_DIR}
else
  DIR_SETUP=$(realpath -m ${DIR_SETUP})
fi

if [[ -z ${DIR_UNPACK} ]]; then 
  DIR_UNPACK=${DATA_TOP_DIR}
else
  DIR_UNPACK=$(realpath -m ${DIR_UNPACK})
fi


if [[ -z ${DIR_RECO} ]]; then 
  DIR_RECO=${DATA_TOP_DIR}
else
  DIR_RECO=$(realpath -m ${DIR_RECO})
fi

if [[ -z ${DIR_QA} ]]; then 
  DIR_QA=${DATA_TOP_DIR}
else
  DIR_QA=$(realpath -m ${DIR_QA})
fi

if [[ -z ${DIR_KFPF} ]]; then 
  DIR_KFPF=${DATA_TOP_DIR}
else
  DIR_KFPF=$(realpath -m ${DIR_KFPF})
fi


# ----- Check the environment
#
if [[ -z "${VMCWORKDIR}" ]]; then
  printf "E- CBM environment is not defined (VMCWORKDIR is not found). Please, configure your CbmRoot\n"
  exit 1
fi

# ----- Redefine the TSA input, if the TSA index and job were provided
#
# TODO: Add check on the integer for JOB_ID
if [[ ${TSA_INDEX} != 0 && ! -z ${TSA_INDEX} ]]; then
  if [[ ${JOB_ID} -le 0 ]]; then 
    exit 100;  # illegal job number
  fi
  TSA=$(sed -n "${JOB_ID}p" "${TSA_INDEX}")
fi

# ----- Select the reconstruction binary
ONLINE_BINARY="${VMCWORKDIR}/../../bin/cbmreco"
if [[ ! -x "${ONLINE_BINARY}" ]]; then
  ONLINE_BINARY=$(which cbmreco)
  if [[ ! -x "${ONLINE_BINARY}" ]]; then
    # Test for execution within the build folder by CTEST (CI/CMAKE/CDASH)
    ONLINE_BINARY="${PWD}/../../../bin/cbmreco"
    if [[ ! -x "${ONLINE_BINARY}" ]]; then
      printf "E- Online binary was not found. Please:\n"
      printf "   - install and configure your CbmRoot\n"
      printf "   - or provide the bin directory in the PATH environmental variable\n"
      printf "   - or run within the CI/CTEST environment\n"
      exit 2
    fi
  fi
fi

# The run_info must be in the same directory as the cbmreco
RUN_INFO=${ONLINE_BINARY%/*}/run_info

if [[ ! -x ${RUN_INFO} ]]; then 
  printf "E- the run_info binary was not found. It must be in the same bin directory, as the cbmreco binary. "
  printf "At this point such an error might not happen, but never the less it did, so please provide the "
  printf "path to the run_info binary in PATH manually.\n"
  exit 2
fi

printf "I- Online binary path: %s, with parameters: %s\n" ${ONLINE_BINARY} ${ONLINE_PAR}


# ----- Check the TSA input and retrieve the run index
if [[ ${I_WANT_ONLY_SETUP} -ne 1 ]]; then
  if [[ -z ${TSA} ]]; then
    printf "E- TSA input file is not defined\n"
    exit 3
  fi
  TSA_INFO=($(basename $(echo ${TSA}) | grep -oE '[0-9]*'))

  RUN=${TSA_INFO[0]}  # Implying, that TSA basename contains run index as the first integer
fi


if [[ ${I_WANT_ONLY_SETUP} -eq 1 ]]; then 
  DO_CREATE_SETUP=1
  DO_UNPACK=0
  DO_RECO=0
  DO_QA=0
  DO_QA_MODULE=0
  DO_LAMBDA=0
  STEPS_TO_PRINT=" --setup"
  if [[ ${RUN} -lt 0 ]]; then 
    RUN=${RUN_IF_ONLY_SETUP_NEEDED}
  fi
fi

if [[ ${RUN} -lt 0 ]]; then
  printf "E- Run number is undefined, please try again with option -r <RUN> or --run <RUN>\n"
fi

if [[ DO_UNPACK -eq 1 ]]; then
  mkdir -p ${DIR_UNPACK}
fi
if [[ DO_RECO -eq 1 ]]; then 
  mkdir -p ${DIR_RECO}
fi
if [[ DO_QA -eq 1 && DO_QA_MODULE -eq 1 ]]; then
  mkdir -p ${DIR_QA}
fi 
if [[ DO_LAMBDA -eq 1 ]]; then
  mkdir -p ${DIR_KFPF}
fi

# ----- Filenames definition
TSA_INP=${TSA}
MACRO_SETUP="${VMCWORKDIR}/macro/run/create_mcbm_geo_setup.C"
MACRO_RECO="${VMCWORKDIR}/macro/beamtime/mcbm2022/mcbm_event_reco_L1.C"
MACRO_QA="${VMCWORKDIR}/macro/mcbm/mcbm_qa.C"
MACRO_QA_MODULE="${VMCWORKDIR}/macro/qa/run_recoQa.C"
MACRO_KFPF="${VMCWORKDIR}/macro/mcbm/mcbm_hadron_kfp_ana.C"
SETUP_NAME=$(${RUN_INFO} --run ${RUN} --geotag)

# ----- Setting selections vs. run number 
#  
if [[ ${RUN} -ge 2350 && ${RUN} -le 2610 ]]; then
  MACRO_RECO="${VMCWORKDIR}/macro/beamtime/mcbm2022/mcbm_event_reco_L1.C"
elif [[ ${RUN} -ge 2724 ]]; then
  MACRO_RECO="${VMCWORKDIR}/macro/beamtime/mcbm2024/mcbm_event_reco_L1.C"
else
  printf "E- Run %5d is undefined. Exiting" "${RUN}"
  exit 4
fi

# -----
# Data file names
FILE_LABEL="test"
if [[ ${I_WANT_ONLY_SETUP} -ne 1 ]]; then
  FILE_LABEL=$(printf $(basename ${TSA_INP}) | cut -f 1 -d '.')
fi
DIGI_ONLINE_FILE="${DIR_UNPACK}/${FILE_LABEL}.digi.out"
DIGI_OFFLINE_FILE="${DIR_UNPACK}/${FILE_LABEL}.digi.root"
SETUP_PAR_FILE="${DIR_SETUP}/${SETUP_NAME}.par.root"
SETUP_GEO_FILE="${DIR_SETUP}/${SETUP_NAME}.geo.root"
RECO_PAR_FILE="${DIR_RECO}/${FILE_LABEL}.par.root"
RECO_FILE="${DIR_RECO}/${FILE_LABEL}.rec.root"
KF_SETUP_FILE="${DIR_RECO}/${FILE_LABEL}.rec.kf.setup"
KF_MATERIAL="${DIR_RECO}/${SETUP_NAME}.mat.kf.bin"
CA_PAR_FILE="${DIR_RECO}/${FILE_LABEL}.rec.ca.par"


# -----
# Log Files
SETUP_LOG="run_mcbm_setup.log"
UNPACK_LOG="run_mcbm_unpack.log"
RECO_LOG="run_mcbm_reco.log"
RECO_QA_LOG="run_mcbm_reco_qa.log"
QA_LOG="run_mcbm_qa.log"
KFPF_LOG="run_mcbm_kfpf.log"


printf "\n"
printf "\n"
printf "\t********************************************************************************************\n"
printf "\t***                                                                                      ***\n"
printf "\t***    --- The Compressed Baryonic Matter Experiment: Data Reconstruction Routine ---    ***\n"
printf "\t***                                                                                      ***\n"
printf "\t********************************************************************************************\n\n\n"

printf "\tSetup:                  ${DIR_SETUP}\n"
printf "\tUnpacking output:       ${DIR_UNPACK}\n"
printf "\tReconstruction output:  ${DIR_RECO}\n"
printf "\tQA output:              ${DIR_QA}\n"
printf "\tKFPF output:            ${DIR_KFPF}\n\n\n"
printf "\n"
printf "\tTSA:           ${TSA}\n"
printf "\tJOB:           ${JOB_ID}\n"
printf "\tROUTINE STEPS: ${STEPS_TO_PRINT}\n"
printf "\n"
printf "\tGEOMETRY FILE: ${SETUP_GEO_FILE}\n"



#  *********************
#  ** Steps execution **
#  *********************

# ----- Create setup files
if [[ ${DO_CREATE_SETUP} -eq 1 ]]; then
  root -b -l -q ${MACRO_SETUP}"(${RUN}, \"${DIR_SETUP}\")" &> ${SETUP_LOG}
  cat ${SETUP_LOG}

  if [[ (1 -ne $(grep -c " Test passed" "${SETUP_LOG}")) || (1 -ne $(grep -c " All ok " "${SETUP_LOG}")) ]]; then
    printf "\nSetup file creation for %s failed, stopping there\n" "${TSA_INP}"
    rm ${SETUP_LOG}
    exit 5
  fi
  rm ${SETUP_LOG}
fi

if [[ ${I_WANT_ONLY_SETUP} -eq 1 ]]; then
  exit 0
fi

# ----- Run unpacker
if [[ ${DO_UNPACK} -eq 1 ]]; then
  # TODO: Define unpack options for different setups
  ${ONLINE_BINARY} --steps Unpack -i ${TSA_INP} -r ${RUN} -p ${ONLINE_PAR} -O DigiTimeslice -o ${DIGI_ONLINE_FILE} -s STS BMON TOF TRD RICH -n ${N_TS}

  root -l -b -q ${VMCWORKDIR}/macro/run/run_inspect_digi_timeslice.C"(\"${DIGI_ONLINE_FILE}\", \"${DIGI_OFFLINE_FILE}\")" &> ${UNPACK_LOG}
  cat ${UNPACK_LOG}
  rm ${DIGI_ONLINE_FILE}

  if [[ 1 -ne $(grep -c "Macro finished successfully." "${UNPACK_LOG}") ]]; then
    printf "\nUnpacked data file conversion of %s failed, stopping there\n" "${TSA_INP}"
    rm ${UNPACK_LOG}
    exit 6
  fi
  rm ${UNPACK_LOG}
fi

# ----- Run reconstruction
if [[ ${DO_RECO} -eq 1 ]]; then
  UNP_FILE_ID=-1
  RECO_DIGI_INPUT="${DIR_RECO}/${FILE_LABEL}.digi.root";
  RECO_GEO_INPUT="${DIR_RECO}/${SETUP_NAME}.geo.root"
  if [[ ${RECO_DIGI_INPUT} != ${DIGI_OFFLINE_FILE} ]]; then 
    pushd .
    cd ${DIR_RECO}
    ln -s -f ${DIGI_OFFLINE_FILE} $(basename ${RECO_DIGI_INPUT})
    popd
  fi 
  if [[ ${SETUP_GEO_FILE} != ${RECO_GEO_INPUT} ]]; then 
    pushd .
    cd ${DIR_RECO}
    ln -s -f ${SETUP_GEO_FILE} $(basename ${RECO_GEO_INPUT})
    popd
  fi
  
  cp ${SETUP_PAR_FILE} ${RECO_PAR_FILE}
  
  PARS="${RUN},${N_TS},\"${DIR_RECO}\",\"${DIR_RECO}\",${UNP_FILE_ID},${RECO_MVD},${RECO_STS},${RECO_TRD}"
  PARS="${PARS},${RECO_TRD2d},${RECO_RICH},${RECO_MUCH},${RECO_TOF},${RECO_TOFtr},${RECO_PSD},${RECO_ALI},${RECO_EvB}"
  PARS="${PARS},${RECO_CA},${RECO_QA},${RECO_FSD},\"${RECO_DIGI_INPUT}\""
  if [[ ${MACRO_RECO} == "*.mcbm2024*" ]]; then
    PARS="${PARS},${RECO_PV}"
  fi

  if [[ ${DISABLE_LOGS} -eq 1 ]]; then
    root -b -l -q ${MACRO_RECO}"(${PARS})"
  else 
    root -b -l -q ${MACRO_RECO}"(${PARS})" &> ${RECO_LOG}
    cat ${RECO_LOG}

    if [[ (1 -ne $(grep -c " Test passed" "${RECO_LOG}")) || (1 -ne $(grep -c " All ok " "${RECO_LOG}")) ]]; then
      printf "\nReconstruction of %s failed, stopping there\n" "${TSA_INP}"
      rm ${RECO_LOG}
      exit 7
    fi
    rm ${RECO_LOG}
  fi
  # ln -s -f "${FILE_LABEL}.digi.root" "${DATA_TOP_DIR}/${FILE_LABEL}.raw.root"  # TMP for QA
  # Commented out as the output of mcbm_event_reco_L1.C is already [...].rec.root
  #  ln -s -f "${FILE_LABEL}.reco.root" "${DATA_TOP_DIR}/${FILE_LABEL}.rec.root"  # TMP for QA
fi

# ----- Run QA of reco modules
if [[ ${DO_QA_MODULE} -eq 1 ]]; then
  QA_GEO="${DIR_QA}/${SETUP_NAME}.geo.root"
  QA_REC="${DIR_QA}/${FILE_LABEL}.rec.root"
  QA_PAR="${DIR_QA}/${FILE_LABEL}.par.root"
  if [[ ${QA_REC} != ${RECO_FILE} ]]; then 
    pushd .
    cd ${DIR_QA}
    ln -s -f $(realpath ${RECO_FILE}) $(basename ${QA_REC})
    popd
  fi
  if [[ ${QA_GEO} != ${SETUP_GEO_FILE} ]]; then 
    pushd .
    cd ${DIR_QA}
    ln -s -f $(realpath ${SETUP_GEO_FILE}) $(basename ${QA_GEO})
    popd
  fi
  if [[ ${QA_REC} != ${RECO_FILE} ]]; then 
    pushd .
    cd ${DIR_QA}
    ln -s -f $(realpath ${RECO_PAR_FILE}) $(basename ${QA_PAR})
    popd
  fi
  ln -s ${SETUP_GEO_FILE} $(basename ${QA_GEO})

  PARS="-1,\"${QA_REC}\",\"${SETUP_NAME}\",kFALSE,${RECO_ALI}"
  if [[ ${DISABLE_LOGS} -eq 1 ]]; then
    root -b -l -q ${MACRO_QA_MODULE}"(${PARS})"
  else
    root -b -l -q ${MACRO_QA_MODULE}"(${PARS})" &> ${RECO_QA_LOG}
    cat ${RECO_QA_LOG}

    if [[ (1 -ne $(grep -c " Test passed" "${RECO_QA_LOG}")) || (1 -ne $(grep -c " All ok " "${RECO_QA_LOG}")) ]]; then
      printf "\nReco Modules QA for %s failed, stopping there\n" "${TSA_INP}"
      rm ${RECO_QA_LOG}
      exit 8
    fi
    rm ${RECO_QA_LOG}
  fi
fi

# ----- Run QA
if [[ ${DO_QA} -eq 1 ]]; then
  USE_MC="kFALSE"
  CONFIG=""
  BENCHMARK=""
  #PARS="1,\"${DATA_TOP_DIR}/${FILE_LABEL}\",\"${SETUP_NAME}\",${USE_MC},\"${CONFIG}\",\"${BENCHMARK}\""
  QA_RAW=${DIGI_OFFLINE_FILE}
  QA_REC=${RECO_FILE}
  QA_PAR=${RECO_PAR_FILE}
  QA_GEO=${SETUP_GEO_FILE}
  QA_OUT="${DIR_QA}/${FILE_LABEL}.qa.root"

  PARS="0,\"\",\"${QA_RAW}\",\"${QA_REC}\",\"${QA_PAR}\",\"${QA_GEO}\",\"${QA_OUT}\",\"${SETUP_NAME}\""
  PARS="${PARS},${USE_MC},\"${CONFIG}\",\"${BENCHMARK}\",${RECO_ALI}"
  
  if [[ ${DISABLE_LOGS} -eq 1 ]]; then
    root -b -l -q ${MACRO_QA}"(${PARS})"
  else
    root -b -l -q ${MACRO_QA}"(${PARS})" &> ${QA_LOG}
    cat ${QA_LOG}

    if [[ (1 -eq $(grep -c " QA checks failed" "${QA_LOG}")) || (1 -ne $(grep -c " Test passed" "${QA_LOG}"))
          || (1 -ne $(grep -c " All ok " "${QA_LOG}")) ]]; then
      printf "\nFull QA for %s failed, stopping there\n" "${TSA_INP}"
      rm ${QA_LOG}
      exit 9
    fi
    rm ${QA_LOG}
  fi
fi

# ----- Run Lambda reconstuction using the KFParticleFinder
if [[ ${DO_LAMBDA} -eq 1 ]]; then
  LAMBDA_FST_TS=0
  LAMBDA_LST_TS=0  # Run all the data produced by reconstruction
  LAMBDA_IN_REC=${RECO_FILE}
  LAMBDA_IN_TRA="\"\",\"\",\"\""  # Empty for real data
  LAMBDA_OUT="${DIR_KFPF}/${FILE_LABEL}.kfp.ana.root"
  LAMBDA_IN_GEO=${SETUP_GEO_FILE}
  LAMBDA_IN_PAR=${RECO_PAR_FILE}
  LAMBDA_USE_MC="false"
  PARS="${RUN},${LAMBDA_FST_TS},${LAMBDA_LST_TS},\"${LAMBDA_IN_REC}\",${LAMBDA_IN_TRA},\"${LAMBDA_OUT}\",\"${LAMBDA_IN_GEO}\""
  PARS="${PARS},\"${LAMBDA_IN_PAR}\",${RECO_ALI},${LAMBDA_MIXED_EVENT},${LAMBDA_USE_MC}"

  root -b -l -q ${MACRO_KFPF}"(${PARS})"
fi


printf "Reconstruction of %s succeeded\n" "${TSA_INP}"
