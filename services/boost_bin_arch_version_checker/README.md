Standalone binary printing out the BOOST_ARCHIVE_VERSION of a boost binary archive and the minimal version of Fairsoft needed to read it.

# Caveats

1. The binary has to fully load/open the file to access the version number
1. `boost` throws an `unsupported version` exception of type `boost::archive::archive_exception` when encountering an archive created with a more recent and non-backward compatible version

=> !!! Use the newest version of `Fairsoft` or a most up to date version of `boost` to have largest compatibility !!!
=> User will probably want to use another version of `boost` than the one making troubles, therefore the independent compilation

# Compilation

With `Fairsoft nov22p1`
```
g++ check_archive_version.cpp -I/cvmfs/fairsoft.gsi.de/debian10/fairsoft/nov22p1/include/boost/archive/ -L/cvmfs/fairsoft.gsi.de/debian10/fairsoft/nov22p1/lib -Wl,-rpath,/cvmfs/fairsoft.gsi.de/debian10/fairsoft/nov22p1/lib -lboost_serialization -o check_archive_version
```

# Usage 

```
./check_archive_version <PATH TO BOOST BINARY ARCHIVE>
```

# Example
```
> ./check_archive_version /lustre/cbm/online/output/mCBM_24/2914_00_00.rra 
trying to check BOOST_ARCHIVE_VERSION of /lustre/cbm/online/output/mCBM_24/2914_00_00.rra
get_library_version() for /lustre/cbm/online/output/mCBM_24/2914_00_00.rra
 => 19
Minimal Fairsoft version: apr22 and its patch versions
```

# Pre-compiled version on lustre 

- !!! Subject to changes without warning !!!
- Should work as long as operating under `Debian10` with `cvmfs` available (e.g. `vae23`or `GSI Linux on lxg/lxi`)
- Tested on `vae23` and one lxg machine

```
/lustre/cbm/users/ploizeau/tools/boost/check_archive_version
```
