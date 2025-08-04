# Geometry related helper scripts and macros

In this directory a collection of ROOT macros and shell scripts is available
which helps to create or check information which is stored in binary ROOT
files which contain a TGeoManager.

## Scan Geometry (scan_geometry.C)

Now moved to the `geometry/ci_scripts` folder.

Scan geometry travels through the root geometry hierarchy of a typical root geometry binary and outputs standard information regarding the node to the screen. There are several options, some of which are commented out in the file, which allows the easy comparison of root binaries. A GRAPHVIZ file named geo.gv is also output to the working directory.

### Examining Text

A typical use may be

```
root -l -q scan_geometry.C("pipe_v16b_1e.geo.root") | less
```

to look at output on screen. To save this output to a file, something like

```
root -l -q '~/cbmroot/macro/geometry/scan_geometry.C("tof_v20a_1h.geo.root")' 2>&1 1>tof_v20a_1h
```

Outputting two files may be compared with standard UNIX diff such as

```
diff -y -W 200 tof_v20a_1h tof_v20b_1h | less
```

### Examining Graphical Representation

Sometimes, especially if the output is very large, it can be more insightful, to get the overall picture to look at a graph tree of the volumes in a geometry instead. This can be done for a single detector or indeed the whole experimental setup, e.g. test.geo.root To make a pdf graph, a basic graph may be made with a command like

```
sed -n '2,$p' geo.gv | head -n -1 | sort | uniq -c | awk 'BEGIN{print "digraph {"}{printf "%s -> %s [label=\"%d\"]\n", $2, $4, $1}END{print "}"}' > geo2.gv
dot -Tpdf geo2.gv > geo.pdf
```

---

## check_media.C

The macro compares the media information at execution time with the media
information assigned to the nodes when building the detector system
geometry. The macro is executed automatically in the CbmRoot test suite for
all tested setups.
If needed it can be executed also manually as described in the following.
If the CbmRoot run time environment is set and a geometry file
for a CBM setup is available the macro is used  in the following way

root -l $VMCWORKDIR/macro/geometry/check_media.C'("setup_name")'

The setup_name is the name of the setup used during the simulation to create
the geometry file.

This macro to check the sanity of the full geometry is/was needed due to
the way we build the complete CBM geometry from independent geometries of the
several detector systems.
When building the complete geometry it happened that the media information
of some nodes was overridden by wrong media information. This problem results
in wrong physical results. The error was fixed but to catch the problem if
it reappears the macro is executed in the test suite for all tested setups.

---

## create_medialist.C

Macro to save the media information when a detector geometry is generated.
The assumption is that the media information is correct at the time when the
detector geometry is created. At the end of this process the macro is
executed and a binary file with the media information for this specific
detector geometry is created. The created root file has to be added to the
input repository in the directory geometry_check.
If needed the macro can also be executed manually with the following procedure.
The macro can be executed only if there is exactly on volume in the geometry file.
To achieve this some preparatory work is needed.

1. Setup the CbmRoot run time environment

   source $BUILDIR/config.sh (if using from build directory)\
   source $INSTALLDIR/bin/CbmRootConfig.sh (if using from installation directory)

2. Create a new test setup

   1. Copy one of the existing setups in geometry/setup
   2. Comment all lines `setup->SetModule(****);` using  single line comments `//` only except the line with the
      detector geometry you want to use

3. Modify the simulation macro

   Remove the target from macro/run/run_tra_file.C by commenting the line `run.SetTarget(****);`

4. Execute the simulation macro to create a geometry file

   root -l $VMCWORKIRDIR/macro/run/run_tra_file.C\(\"$VMCWORKDIR/input/urqmd.auau.10gev.centr.root\",
                           1,\"data/${setup}_test\", \"${setup}\"\)

   The variable setup has to be name of the setup created in 2.

5. Execute the macro create_medialist.C

   root -l $VMCWORKDIR/macro/geometry/create_medialist.C\(\"data/${setup}_test.geo.root\"\)

---

## print_medialist.C

The macro can be used to print the information which is stored in the *geometrycheck.root
files. The output is a list of all stored nodes with the corresponding media
name. To use the macro do

root -l $VMCWORKDIR/macro/geometry/print_medialist.C\(\"$VMCWORKDIR/input/geometry_check/sts_v19a_geometrycheck.root\"\)

---

## create_positionlist.C

Macro to store the global positions of the center of each nodes of a detector system in a
file. The usage is equal to the first four steps described for
create_medialist.C Only the last step is slightly different and creates the
output file example.txt

   root -l $VMCWORKDIR/macro/geometry/create_positionlist.C\(\"data/${setup}_test.geo.root\"\)

---

## costum_setup.sh

Intended for inspection of geometry setups. At the user risk, it may also work for simulation.
Generates a costum-setup file for the CBM experiemnt. User is prompted which subsystem and its configuration to include.
The scripts only works for default geometries and is not intended to switch between geometry versions.
For detectors which are placed on railsystems the user will be prompted as to specify its distance to the target.
Specifying a variable like PSD_INCLUDE=1 will skip question relating to including the PSD, allowing a user to predefine a configuration skip.

Suggested command to create costum setup file:

   sh costum-setup.sh

Creates the run_transport_costum.C and run_reco_costum.C to use the setup_sis100_auto instead of setup_sis100_electron as default.

Questions may be skipped by specifying to include a subsystem MAG_INCLUDE=1 or exclude MAG_INCLUDE=0. To a limited extent some subsystem
detectors may be specified such as MAG_TAG="v20a".

---

## switch_defaults.sh

This script switches between official and trial versions of the CBMROOT geometries. This
is intended for use by a expert user, who will remember to switch back to the official
geometry release (currently APR21) once the specific use case has ended. Current options
include This script switches between official and trial versions of the CBMROOT geometries. This
is intended for use by a knowledgeable user, who will remember to switch back to the official
geometry release (currently APR21) once the specific use case has ended. Current options
include

* APR20 - (previous 2020 default geometries. Run old defaults with the new CBMROOT software.)
* APR21 - (current 2021 default geometries. This is the official release geometries.
* TEST  - (Geometries shift such that the center of the magnet is the origin of the CBM exp.)


## check_radlen.sh

Now moved to the `geometry/ci_scripts` folder.

Checks for a common issue whereby the radiaiton length is miscalculated by root.

The script calculates the radiation length from the stated atomic number, mass and density of the material
and compares this to the stated radlen in the material. If the difference is more than a tolerance (currently
5%) then failure is declared. It is up to the use to assess information. Dummy and vacuum materials may be safely
ignored.

Sugggested command

sh check_radlen.sh much_v20b_mcbm.geo.root

This will check the materials rad length within a certain tolerance (5%). Recommended checking
the transported geometry file in simulations to check whether target definition and all geometries
are correct.

sh check_radlen.sh test.geo.root


---

## check_radlen_bulk.sh

Allows to run the `check_radlen` script on sets of geometries present in the subfolders of the local `geometry` folder.

**Usage of the script:**
- `./macro/geometry/check_radlen_bulk.sh`  or `./macro/geometry/check_radlen_bulk.sh git` \
  => check new geometries introduced by a hash change relative to the current upstream tip
- `./macro/geometry/check_radlen_bulk.sh all` \
  => check all geometries in the local geometry folder (including mCBM ones)
- `./macro/geometry/check_radlen_bulk.sh main` \
  => check all geometries without mcbm in their name in the local geometry folder
- `./macro/geometry/check_radlen_bulk.sh <tag>` \
  => check all geometries with a given tag (detector, version, ...) in their name in the local geometry folder \
  Example for mCBM: `./macro/geometry/check_radlen_bulk.sh mcbm` \
  Example for sts: `./macro/geometry/check_radlen_bulk.sh sts` \
  Example for a version: `./macro/geometry/check_radlen_bulk.sh v22a` \
  Special tags which cannot be used in this way: `git`, `all`, `main`

---

## examine_materials.C

Root macro which compares the materials in the simulated TGeoManager to the materials as extracted from the geometry
binaries for a specified setup. Compares only the names.

Sugggested commands

root -q examine_materials.C

root -q 'examine_materials.C("sis100_muon_jpsi_DEC21")'

 for file in `ls ../../geometry/setup/setup_mcbm_beam_2022_*`; do tag=`echo $file | awk -F'/' '{print $5}' | sed 's|setup_|"|'  | sed 's_.C$_"_'`; echo $tag; root -q './examine_material.C('$tag')'; done > MCBM_2022_EXAMINE

---

## check_overlaps.C

Allow to check overlaps in existing full geometry file (setup, e.g. output of `examine_materials.C`) and detect
expected/unexpected ones.

The lists of expected (known and ignored) overlaps are defined in 4 vectors at the begining of the macro for:
- overlaps between a BMon detector defined in the TOF geometry and the target/pipe vacuum in mCBM (always expected)
- overlaps in mCBM (should be expanded/reduced depending on simulation/analysis progresses)
- overlaps between a BMon detector defined in the TOF geometry and the target/pipe vacuum in CBM (always expected)
- overlaps in CBM (should be expanded/reduced depending on simulation/analysis progresses)

**Usage of the script (see also examples in `CMakeLists.txt`:**
1. With a standard geometry (as defined in setup file, no alignment):
   ```
   root -l -b -q 'check_overlaps.C("<full filename with path>.geo.root")'
   ```
1. With an aligned geometry (some shifts relative to setup file):
   ```
   root -l -b -q 'check_overlaps.C("<full filename with path>.geo.root", "<full filename with path for alignment matrices>.root")'
   ```
=======
---

## total_mass.C

Measures the total mass of the a geometry and setup, via a random dart method. Basic command may be run by, e.g., 

```
root -q total_mass.C'
```

This will output the total mass of each material in the setup.


