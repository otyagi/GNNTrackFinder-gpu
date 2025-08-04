#!/bin/bash

cbmreco_bin=$1
parameter_dir=$2
tsa_file=$3
nb_ts=${4:-1}
nb_threads=${5:-1}

reco_args=" \
    -p $parameter_dir \
    -i $tsa_file \
    -n $nb_ts \
    --omp $nb_threads \
    --steps Unpack DigiTrigger LocalReco Tracking \
"

script=$(readlink -f "$0")
function check_arg {
  if [ -z "$1" ]; then
    echo "Error: <$2> not specified."
    echo "Usage: $script <cbmreco_bin> <parameter_dir> <tsa_file> [<nb_ts>(=1) <nb_threads>(=1)]"
    exit 1
  fi
}

function ensure_gt_zero {
  local output="$1"
  local pattern="$2"

  # Extract placeholders from the pattern, e.g. %1, %2, ..., %9
  # [[ $? == 1 ]] is a workaround for the following issue:
  #   grep returns 1 if no match is found, but set -e causes the script to exit in this case
  placeholders=($(echo "$pattern" | grep -o '%[0-9]\+' || [[ $? == 1 ]]))
  if [[ $? -ne 0 ]]; then
    echo "Error: Failed to extract placeholders: $pattern"
    exit 1
  fi

  if [ -z "${placeholders[*]}" ]; then
    echo "Error: No placeholders found: $pattern"
    exit 1
  fi

  # Replace placeholders with a regex that matches the value
  sed_pattern=$pattern
  for placeholder in "${placeholders[@]}"; do
    sed_pattern="${sed_pattern//$placeholder/\\([0-9]\\+\\)}"
  done

  for placeholder in "${placeholders[@]}"; do
    # Use sed to extract the value for the current placeholder
    # echo "Searching for $placeholder..."
    id=$(echo "$placeholder" | grep -o '[0-9]\+')
    value=$(echo "$output" | sed -n "s/.*$sed_pattern.*/\\$id/p")
    if [ -n "$value" ] && [ "$value" -gt 0 ]; then
      continue
    else
      echo "$pattern: FAIL ($placeholder is not greater than zero: $value)"
      echo "Tried to match: $sed_pattern"
      echo "Output:"
      echo "$output"
      exit 1
    fi
  done

  echo "$pattern: OK"
}

check_arg "$cbmreco_bin" "cbmreco_bin"
check_arg "$parameter_dir" "parameter_dir"
check_arg "$tsa_file" "tsa_file"

# Run the reconstruction and capture the log
echo "Running '$cbmreco_bin $reco_args'"
log="$($cbmreco_bin $reco_args 2>&1)"
reco_ok=$?
if [ $reco_ok -ne 0 ]; then
  echo "$log"
  echo "=============================="
  echo "Error: Reconstruction failed. Exit early."
  exit 1
fi

# Check Digis
ensure_gt_zero "$log" "TS contains Digis: STS=%1 MUCH=%2 TOF=%3 BMON=%4 TRD=%5 TRD2D=%6 RICH=%7 PSD=0 FSD=0"

# Check Events
ensure_gt_zero "$log" "Triggers: %1, events %2"

# Check Hits
ensure_gt_zero "$log" "TS contains Hits: STS=%1 TOF=%2 TRD=%3"

# Check Tracks
ensure_gt_zero "$log" "TrackingChain: Timeslice contains %1 tracks, with %2 sts hits, %3 tof hits, %4 trd hits;"
