#!/usr/bin/env python3

import os
import shutil
import argparse

parser = argparse.ArgumentParser()
parser.description = 'Submit jobs to slurm. Change input parameters for transport/digitization/reconstruction/qa in batch_job.py'
parser.add_argument('-r', '--remove', dest='removeData', default=False, action='store_true', help='Flag to remove all data in directory given by -d / --data')
parser.add_argument('-j', '--jobs', dest='nJobs', type=int, required=True, help='Number of jobs')
parser.add_argument('-c', '--config', dest='configPath', type=str, required=True, help='Path of cbmroot config file')
parser.add_argument('-d', '--data', dest='dataDir', type=str, required=True, help='Data directory to store tra/digi/reco etc. files')
parser.add_argument('-n', '--name', dest='jobName', type=str, default="RICH", help='Job name for slurm')
args = parser.parse_args()

print("--- Provided arguments ---")
print("Remove data: " + str(args.removeData))
print("Number of jobs: " + str(args.nJobs))
print("Config Path: " + args.configPath)
print("Data directory: " + args.dataDir)
print("Job name: " + args.jobName)

removeData = args.removeData
nJobs      = args.nJobs
configPath = args.configPath
dataDir    = (args.dataDir).rstrip("/")
jobName    = args.jobName

timeLimit = "08:00:00"
logFile   = dataDir + "/log/log_slurm-%A_%a.out"
errorFile = dataDir + "/error/error_slurm-%A_%a.out"

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

if os.path.exists(os.path.dirname(logFile)):
  shutil.rmtree(os.path.dirname(logFile))
os.makedirs(os.path.dirname(logFile))

if os.path.exists(os.path.dirname(errorFile)):
  shutil.rmtree(os.path.dirname(errorFile))
os.makedirs(os.path.dirname(errorFile))

commandStr = f'sbatch --job-name={jobName} --time={timeLimit} --output={logFile} --error={errorFile} --array=1-{nJobs} batch_job.py {dataDir} {configPath}'
# commandStr = f'sbatch -p debug --job-name={jobName} --time={timeLimit} --output={logFile} --error={errorFile} --array=1-{nJobs} batch_job.py {dataDir} {configPath}'
print("\n--- Executing command ---\n" + commandStr)
os.system(commandStr)
