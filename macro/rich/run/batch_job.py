#!/usr/bin/env python3

import os
import sys
import shutil

dataDir = sys.argv[1]
configPath = sys.argv[2]
taskId = os.environ.get('SLURM_ARRAY_TASK_ID')
jobId = os.environ.get('SLURM_ARRAY_JOB_ID')

workDir = dataDir + "/workdir/" + jobId + "_" + taskId + "/"
if os.path.exists(workDir):
  shutil.rmtree(workDir)
os.makedirs(workDir)
os.chdir(workDir)

## Settings begin ##
randomSeed = 0
monitor    = 0

# Transport
nEvents    = 1000
urqmdFile  = "/lustre/cbm/prod/gen/urqmd/auau/10gev/centr/urqmd.auau.10gev.centr." + str(taskId).zfill(5) + ".root"
plutoFile  = ""
nElectrons = 5
nPositrons = 5
geoSetup   = "sis100_electron"
engine     = 0 # 0: geant3 1: geant4

# Digitization
eventRate = -1e7
tsLength  = -1

# Reconstruction
eventBuilder = "Ideal"
useMC        = 1

# Qa
resultDir = ""

## Settings end ##

# Files
fileName = "data"
filePathNoExt = dataDir + "/data/" + fileName + taskId
traFile  = filePathNoExt + ".tra.root"
parFile  = filePathNoExt + ".par.root"
geoFile  = filePathNoExt + ".geo.root"
digiFile = filePathNoExt + ".digi.root"
recoFile = filePathNoExt + ".reco.root"
qaFile   = filePathNoExt + ".qa.root"

traCmd  = f'root -l -b -q {"${VMCWORKDIR}"}/macro/rich/run/run_transport.C' \
          f'\(\\"{urqmdFile}\\",\\"{traFile}\\",\\"{parFile}\\",\\"{geoFile}\\",{nEvents},{nElectrons},{nPositrons},\\"{plutoFile}\\",\\"{geoSetup}\\",{engine},{randomSeed}\)'
digiCmd = f'root -l -b -q {"${VMCWORKDIR}"}/macro/rich/run/run_digi.C' \
          f'\(\\"{traFile}\\",\\"{parFile}\\",\\"{digiFile}\\",{nEvents},{eventRate},{tsLength},{randomSeed},{monitor}\)'
recoCmd = f'root -l -b -q {"${VMCWORKDIR}"}/macro/rich/run/run_reco.C' \
          f'\(\\"{traFile}\\",\\"{parFile}\\",\\"{digiFile}\\",\\"{recoFile}\\",{nEvents},\\"{geoSetup}\\",\\"{eventBuilder}\\",{useMC},{monitor}\)'
qaCmd   = f'root -l -b -q {"${VMCWORKDIR}"}/macro/rich/run/run_qa.C' \
          f'\(\\"{traFile}\\",\\"{parFile}\\",\\"{digiFile}\\",\\"{recoFile}\\",\\"{qaFile}\\",{nEvents},\\"{geoSetup}\\",\\"{resultDir}\\",{monitor}\)'

os.system(f'. {configPath} -a ; {traCmd}')
os.system(f'. {configPath} -a ; {digiCmd}')
os.system(f'. {configPath} -a ; {recoCmd}')
os.system(f'. {configPath} -a ; {qaCmd}')
