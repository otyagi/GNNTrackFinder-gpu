Conversion of CAD files to ROOT
==============================

We outline an approach to convert a native CAD geometries to ROOT(GEANT) geometries.

This document will focus on conversion of

[1] STP to STL
Available from any CAD software. Each part, or parts of the same material are written as seperate stl files.

[2] STL to GDML
The python parser available from [here][https://github.com/tihovav/cad-to-geant4-converter] is used:
```shell
for file in *.stl; do python /opt/cad-to-geant4-converter/stl_gdml.py noneeded $file; done
```
[3] GDML to ROOT
```shell
for file in `ls *.gdml`; do echo $file; root -q 'make_TGeoVolumeAssembly.C("'$file'")'; done;
```
[4] Parsing of the control document allows the introduction of materials, places the ROOT binaries in correct location.
```shell
awk -F'|' '{gsub("[ ]*$","",$1); gsub(" ","_",$1); gsub("^[ ]*","",$5); gsub("[ ]*$","",$5); gsub(" ","_",$5);  printf "add_binary(\\\n\"%s.root\",\\\ntop, gM->GetMedium(\"%s\"), inum++,\\\ncad_matrix(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\\\n);\n\n", $1, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15/10, $16/10, $17/10;}' CONTROL_CAD.txt > PARTS.C
```

[5] Stiching together all files into a single detector geometry.
```shell
root -q Create_Geometry.C
```






