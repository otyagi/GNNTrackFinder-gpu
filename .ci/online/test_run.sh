#! /bin/bash

set -e

IMAGE=hub.cbm.gsi.de/fweig/containers/cbm_online:online-container
DOCKER_FLAGS=" \
    --rm \
    -it \
    --mount type=bind,source=$HOME/cbm/params,target=/opt/cbm/params \
    --mount type=bind,source=/home/cbmdata/mcbm22/2391,target=/opt/cbm/data \
"

CMD="cbmreco"
CMD_FLAGS=" \
    -p /opt/cbm/params \
    -i /opt/cbm/data/2391_node8_1_0000.tsa \
    -n 1 \
    --omp 1 \
"

docker run $DOCKER_FLAGS $IMAGE $CMD $CMD_FLAGS
# docker run $DOCKER_FLAGS $IMAGE ls -l /opt/cbm/data
