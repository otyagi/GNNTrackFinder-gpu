/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese[committer] */

#include "CbmSourceTs.h"

#include <TimesliceAutoSource.hpp>

#include <FairSource.h>
#include <Logger.h>


using fles::Timeslice;
using std::string;
using std::vector;


// -----   Constructor   ------------------------------------------------------
CbmSourceTs::CbmSourceTs(const char* fileName) { AddInputFile(fileName); }
// ----------------------------------------------------------------------------

// -----   Constructor   ------------------------------------------------------
CbmSourceTs::CbmSourceTs(vector<string> fileNames) : fFileNames(fileNames) {}
// ----------------------------------------------------------------------------


// -----   Add an input file   ------------------------------------------------
size_t CbmSourceTs::AddInputFile(const char* fileName)
{
  string sFile(fileName);
  if (sFile.size()) fFileNames.push_back(sFile);
  return fFileNames.size();
}
// ----------------------------------------------------------------------------


// -----   Close   -----------------------------------------------------------
void CbmSourceTs::Close() {}
// ----------------------------------------------------------------------------


// -----   Initialisation   ---------------------------------------------------
Bool_t CbmSourceTs::Init()
{
  LOG(info) << "SourceTs: Creating TimesliceSource with " << fFileNames.size() << " input files.";
  fFlesSource = new fles::TimesliceAutoSource(fFileNames);
  return kTRUE;
}
// ----------------------------------------------------------------------------


// -----   Read one time slice from archive   ---------------------------------
Int_t CbmSourceTs::ReadEvent(UInt_t tsIndex)
{
  // Timeslices can only be read sequentially.
  // It appears that the first call to this method from FairRunOnline is in the
  // init stage. In order not to always lose the first timeslice, a call to
  // TimesliceSource::get is avoided in the first call.
  if (fNumCalls == 0) {
    LOG(info) << "SourceTs: Init call to ReadEvent";
    fNumCalls++;
  }
  else if (tsIndex > 0 && tsIndex < fNumCalls - 1) {
    LOG(error) << "SourceTs: Out-of-sequence reading of timeslices not supported.";
    return 1;
  }
  else {
    do {
      fFlesTs = nullptr;
      fFlesTs = fFlesSource->get();
      if (!fFlesTs) {
        LOG(info) << "SourceTs: End of archive reached; stopping run.";
        return 1;
      }
      if (tsIndex == fNumCalls - 1) {
        LOG(info) << "SourceTs: Reading time slice " << GetNumTs() << " (index " << fFlesTs->index()
                  << ") at t = " << fFlesTs->start_time() << " ns";
      }
      else {
        LOG(info) << "SourceTs: Skipping time slice " << GetNumTs() << " (index " << fFlesTs->index()
                  << ") at t = " << fFlesTs->start_time() << " ns";
        LOG(info) << "(TS is still fetched from disk, this may take some time)";
      }
      fNumCalls++;
    } while (tsIndex >= fNumCalls - 1);
  }
  return 0;
}
// ----------------------------------------------------------------------------


ClassImp(CbmSourceTs)
