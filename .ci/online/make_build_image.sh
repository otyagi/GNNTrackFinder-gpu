#!/bin/bash

set -e

user="$1"
password="$2"

script=$(readlink -f "$0")

function check_arg() {
    if [ -z "$1" ]; then
        echo "Error: No $2 specified."
        echo "Usage: $script <user> <password>"
        exit 1
    fi
}

check_arg "$user" "user"
check_arg "$password" "password"

# Set tag as optional third argument
if [ -z "$3" ]; then
    tag="$user-debug"
else
    tag="$3"
fi

# This script must be run from the CbmRoot top level directory.
# Check for .git, .clang-format, and .gitlab-ci.yml
if [ ! -d .git ] || [ ! -f .clang-format ] || [ ! -f .gitlab-ci.yml ]; then
    echo "Error: This script must be run from the CbmRoot top level directory."
    exit 1
fi

registry="hub.cbm.gsi.de/computing/cbmroot"
image="cbm_online"

build_args=" \
    --build-arg="USERNAME=$user" \
    --build-arg="PASSWORD=$password" \
    --build-arg="TAG=$tag" \
"
dockerfile="algo/containers/cbm_online/DockerfileBuild"
docker build --progress plain $build_args -f $dockerfile -t cbm_online_builder .
