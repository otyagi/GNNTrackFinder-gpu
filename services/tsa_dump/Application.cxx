/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#include "Application.h"

#include "CbmFormatMsHeaderPrintout.h"
#include "CbmFormatTsPrintout.h"

#include <StorableTimeslice.hpp>
#include <TimesliceAutoSource.hpp>

#include <Logger.h>

#include <iomanip>
#include <ios>
#include <iostream>
#include <sstream>

void Application::Run()
{
  LOG(info) << "Calling string constructor with ";
  LOG(info) << fOpts.sFullFilename;
  fles::TimesliceAutoSource* fTsSource = new fles::TimesliceAutoSource(fOpts.sFullFilename);

  LOG(info) << "Selected SysID: " << std::hex << fOpts.selSysId << std::dec;

  LOG(info) << FormatMsHeaderHelp();

  std::unique_ptr<fles::Timeslice> ts;
  ts             = fTsSource->get();
  uint64_t uTsNb = 0;
  while (ts) {
    // Use << operator defined in <cbmroot_src>core/base/utils/flestools/CbmFormatTsPrintout.h
    std::stringstream ss;
    ss << FormatTsPrintout(*(ts.get()), fOpts.selSysId, fOpts.nbMsPerComp);
    LOG(info) << "=====================================\n" << ss.str();
    uTsNb++;
    if (fOpts.uNbTimeslices == uTsNb) {
      break;
    }
    ts = fTsSource->get();
  }
  if (fOpts.uNbTimeslices == uTsNb) {
    LOG(info) << "Requested number of TS reached; stopping there. Dumped " << uTsNb << " TS";
  }
  else {
    LOG(info) << "End of archive reached; stopping there. Dumped " << uTsNb << " TS";
  }
}
