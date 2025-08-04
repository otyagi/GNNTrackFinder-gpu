# Online Container

This document provides an overview over the containers currently used for CBM online operations, how to build them and test the reconstruction container.

## Build container

### Base images

Base images are hosted at [computing/images/base](https://git.cbm.gsi.de/computing/images/base). These are mirrored from places like docker-hub and google container registry to keep building images independent of external sources as well as possible. Included are OS images like Debian and Ubuntu, and tools like Kaniko and Crane.
Mirrored images are updated once per week.

### Login

Login via: `docker login hub.cbm.gsi.de`

### Build Images

Currently 3 images are build, each depending on the previous one(s):
1. `online/dev`: Contains all files to compile the CbmRoot online-code. This includes:
    - Subset of FairSoft packages (boost, root, fmt)
    - ROCm GPU compiler + hip development packages
    - Various system libraries (+ header)
2. `online/runtime`: Minimal container that only contains libraries required to run
  online reconstruction
3. `cbm_online`: Runnable container. Consists of `online_runtime` + cbm online binaries

The first two are hosted outside of CbmRoot in https://git.cbm.gsi.de/computing/images/online. And are rebuilt once per week, to keep system packages up-to-date.

Building new images locally should also be done via Kaniko. To ensure reproducibility with the CI:
```
mv secrets.json.in secrets.json
# Enter your gitlab username and password in secrets.json
docker build -t cbm-builder -f Dockerfile.build . # Create the Kaniko build container
docker run -it --rm -v $PWD:/workspace cbm-builder # Enter the build container
# Run kaniko with the respective Dockerfile to build the dev or runtime image.
```

**Your password is stored in plain text in the container. Make sure to delete it after you're done.**


Building the online code and creating a new image, happens inside a container via kaniko.
To build new cbm_online-images locally, you first have to create a new build-image:
```
cd <CbmRoot>
./algo/containers/cbm_online/make_build_image.sh <gitlab_user> <gitlab_password> <tag>
```
**This will store your password in plain text in the container. Make sure to delete after you're done.**

The image tag is optional. If no tag is provided, defaults to `<gitlab_user>-debug`.

Then run the new container, to build a new cbm_online-image:
```
docker run -it --rm cbm_online_builder
```

This will push a new container image `cbm_online:<tag>` to the CbmRoot registry.

Pull the new image first with:
```
docker pull hub.cbm.gsi.de/computing/cbmroot/cbm_online:<tag>
```

You can use `test_run.sh` or `test_run_gpu.sh` scripts to test the image locally.


## Test on virgo

### Login

Login via: `apptainer remote login -u <user> docker://hub.cbm.gsi.de`

`<user>` should be your gitlab-username. apptainer will ask for your password.

### Pull new images

`apptainer pull docker://hub.cbm.gsi.de/computing/cbmroot/cbm_online:<tag>`

Use `master` as tag to pull the current CbmRoot master. Use `mr<N>` to pull the container for
MR `N`.

### Run on debug partition

Run the container on the virgo debug partition with GPUs:

```
srun --constraint=mi50 -p debug apptainer exec cbm_online.sif cbmreco -p /lustre/cbm/online/params -i /lustre/cbm/online/data/2391_node8_0_0000.tsa -d hip0 -n1
```
