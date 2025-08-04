/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

#include "CaSearchWindow.h"

#include <cassert>
#include <iomanip>
#include <sstream>

using cbm::algo::ca::SearchWindow;

// ---------------------------------------------------------------------------------------------------------------------
//
SearchWindow::SearchWindow(int stationID, int trackGrID) : fStationID(stationID), fTrackGroupID(trackGrID)
{
  assert(stationID > -1);
  assert(trackGrID > -1);

  // Case for constant windows (TEMPORARY: we should add selection of different windows):
  static_assert(kNpars == 1);
}

// TODO: SZh 08.11.2022: Probably, we should have the assertions in the InitManager and remove them from here, since
//                       this class is supposed to be used inside the algorithm core
// ---------------------------------------------------------------------------------------------------------------------
//
void SearchWindow::SetParamDxMaxVsX0(int id, float val)
{
  assert(id > -1 && id < kNpars);
  fvParams[kDxMaxVsX0 * kNpars + id] = val;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void SearchWindow::SetParamDxMinVsX0(int id, float val)
{
  assert(id > -1 && id < kNpars);
  fvParams[kDxMinVsX0 * kNpars + id] = val;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void SearchWindow::SetParamDxMaxVsY0(int id, float val)
{
  assert(id > -1 && id < kNpars);
  fvParams[kDxMaxVsY0 * kNpars + id] = val;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void SearchWindow::SetParamDxMinVsY0(int id, float val)
{
  assert(id > -1 && id < kNpars);
  fvParams[kDxMinVsY0 * kNpars + id] = val;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void SearchWindow::SetParamDyMaxVsX0(int id, float val)
{
  assert(id > -1 && id < kNpars);
  fvParams[kDyMaxVsX0 * kNpars + id] = val;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void SearchWindow::SetParamDyMinVsX0(int id, float val)
{
  assert(id > -1 && id < kNpars);
  fvParams[kDyMinVsX0 * kNpars + id] = val;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void SearchWindow::SetParamDyMaxVsY0(int id, float val)
{
  assert(id > -1 && id < kNpars);
  fvParams[kDyMaxVsY0 * kNpars + id] = val;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void SearchWindow::SetParamDyMinVsY0(int id, float val)
{
  assert(id > -1 && id < kNpars);
  fvParams[kDyMinVsY0 * kNpars + id] = val;
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string SearchWindow::ToString() const
{
  using std::setw;
  std::stringstream msg;
  msg << "----- CA hits search window: \n";
  msg << "\tstation ID:      " << fStationID << '\n';
  msg << "\ttracks group ID: " << fTrackGroupID << '\n';
  msg << "\tparameters:\n";
  msg << "\t\t" << setw(6) << "No." << ' ';
  msg << setw(12) << "dx_max(x0)" << ' ';
  msg << setw(12) << "dx_min(x0)" << ' ';
  msg << setw(12) << "dx_max(y0)" << ' ';
  msg << setw(12) << "dx_min(y0)" << ' ';
  msg << setw(12) << "dy_max(x0)" << ' ';
  msg << setw(12) << "dy_min(x0)" << ' ';
  msg << setw(12) << "dy_max(y0)" << ' ';
  msg << setw(12) << "dy_min(y0)" << '\n';
  for (int iPar = 0; iPar < kNpars; ++iPar) {
    msg << "\t\t" << setw(6) << iPar << ' ';
    for (int iDep = 0; iDep < kNdeps; ++iDep) {
      msg << setw(12) << fvParams[iDep * kNpars + iPar] << ' ';
    }
  }
  return msg.str();
}
