#!/usr/bin/env python3

import os
import shutil
import argparse

parser = argparse.ArgumentParser()
parser.description = 'Run jobs locally in parallel. Change input parameters for transport/digitization/reconstruction/qa in run_one.py'
parser.add_argument('-r', '--remove', dest='removeData', default=False, action='store_true', help='Flag to remove all data in directory given by -d / --data')
parser.add_argument('-j', '--jobs', dest='nJobs', type=int, default=4, help='Number of jobs')
parser.add_argument('-c', '--config', dest='configPath', type=str, required=True, help='Path of cbmroot config file')
parser.add_argument('-d', '--data', dest='dataDir', type=str, required=True, help='Data directory to store tra/digi/reco etc. files')
args = parser.parse_args()

print("--- Provided arguments ---")
print("Remove data: " + str(args.removeData))
print("Number of jobs: " + str(args.nJobs))
print("Config Path: " + args.configPath)
print("Data directory: " + args.dataDir)

removeData = args.removeData
nJobs      = args.nJobs
configPath = args.configPath
dataDir    = (args.dataDir).rstrip("/")

logDir = dataDir + "/logs"

if removeData:
  answer = input(f'\nAll data in {dataDir} will be removed. Continue? (y/n): ')
  if answer.lower() in ['y','yes']:
    print("Removing ...")
    if os.path.exists(dataDir):
      shutil.rmtree(dataDir)
    print("Done removing.")
  else:
    print("Aborting ...")
    exit(0)

os.makedirs(dataDir, exist_ok=True)

if os.path.exists(logDir):
  shutil.rmtree(logDir)
os.makedirs(logDir)

print("\n--- Executing commands ---")
for taskId in range(nJobs):
  commandStr = (f'python3 run_one.py {args.dataDir} {args.configPath} {taskId} > {dataDir}/logs/output{taskId}.txt 2>&1 &')
  print(commandStr)
  os.system(commandStr)
