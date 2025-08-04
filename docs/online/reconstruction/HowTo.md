# Online Reconstruction

The online reconstruction is a single executable called `cbmreco`.

Compile it via `make cbmreco` (or just `make`).

Then run it with:
```
build/bin/cbmreco -p parameters/online -i <path/to/tsa>
```
Where `-i` should point to a .tsa file and `-p` points to the folder of parameters.
Within CbmRoot these parameter files are located at `parameters/online` by default.
Example TSA files can be found on the `lustre` filesystem at GSI, please contact members of the mCBM or Online team to know the most recent locations (subject to changes over time).
Full run TSA files can be found on the `lustre` filesystem at GSI in subfolders of `/lustre/cbm/prod/beamtime/2022/0*/mcbm`, please contact members of the mCBM team to know the conditions of the various runs.

This will run the full reconstruction and print some basic results in the log.

## Selecting reconstruction steps

The reconstruction can be filtered by steps and active detectors.

Use `-s/--systems` to select specific detectors and `--steps` to limit the steps run in the reconstruction chain. E.g.:
```
build/bin/cbmreco -p parameters/online -i <path/to/tsa> -s STS --steps Unpack LocalReco
```
will only run unpacking + local reconstruction (hitfinding) for the STS.

A full list of all available steps can be found in `algo/base/Definitions.h` in the enum `Step`. (Multiple values are space seperated.)

## File storage

File storage is controlled via two flags:
- `-o` / `--output`: Name of the output file
- `-O` / `--output-types`: Types of data stored in file

E.g.:
```
build/bin/cbmreco -p parameters/online -i <path/to/tsa> -O DigiEvent Hit -o test.out
```
would store all digi events and reconstructed hits in a file `test.out`.

A full list of all available output types can be found in `algo/base/Definitions.h` in the enum `RecoData`.

## Monitoring

See [Monitoring](../monitoring/HowTo.md).

## Profiling

Running with `-t` will print the runtime of all steps and kernels that run during the reconstruction that were recorded via `xpu::scoped_timer` of `xpu::push_timer` / `xpu::pop_timer`.

## GPU compilation

Enable GPU compilation for AMD via
```
cmake -DXPU_ENABLE_HIP=ON build
```
Then recompile. You should see a library called `libAlgo_Hip.so` in the build log.

Use `-d <device>` to select a GPU device at runtime. E.g. use `hip0` to use the first AMD GPU on the machine.

### Listing devices

A list of all available devices can be printed with `xpuinfo`:
```
build/bin/xpuinfo
```

### CUDA and SYCL

CUDA and SYCL compilation can be enabled with the respective `XPU_ENABLE_CUDA` and `XPU_ENABLE_SYCL` cmake options.

**Warning: CUDA and SYCL compilation isn't currently tested and not guarenteed to work.**
