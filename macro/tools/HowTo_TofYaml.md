# Generation of TOF parameter YAML files for the algo version of the TOF HitFinder/Clusterizer

**!!! WORKING ONLY FOR mCBM RUNS !!!**

1. Pick a run (`<RUN ID>` in later parts), check in `core/base/utils/CbmMcbmUtils.cxx` to which setup it corresponds and check in `geometry/setup` that this would load the geometries you expect (if not modify te setup file)
1. Locate where you have your custom TOF calibration file (`.....tofClust.hst.root`)
1. Locate where you have your custom geometry alignment file (typ. `AlignmentMatrices_<setup_name>.root`) and make sure it fit you (eventually custom) setup
1. Create the geo root file in `macro/run` by running
   ```
   root -l -b -q 'create_mcbm_geo_setup.C(<RUN ID>)'
   ```
1. Create an unpacked file with at least 1 TS for this run (with `macro/run/run_unpack_tsa.C`) or locate an existing unpacked file and make a symlink for it called `<RUN ID>.digi.root` in `macro/run/data`
1. Go to `macro/tools`
1. Run the following command (replacing the `<....>` parts with the corresponding info)
   ```
   root -l -b -q 'tof_hitfinder_run.C(<RUN ID>, 1, "../run/data/", "../run/data/", -1, "", <FULL PATH TO TOF CALIB FILE>, <FULL PATH TO ALIGNMENT FILE>, true)'
   ```
1. You should now find in the folder you are two new YAML files called `TofCalibratePar.yaml` and `TofHitfinderPar.yaml`
   - If you want you can try to compare them with `diff` or `meld` to the ones in `parameters/online`
1. To test the new YAML param files
   1. Copy the files to `parameters/online`
   1. Run (using the dummy/example file made for the yaml generation) 
      ```
      root -l -b -q 'tof_hitfinder_run.C(<RUN ID>, 1, "./data/", "./data/", -1)'
      ```
