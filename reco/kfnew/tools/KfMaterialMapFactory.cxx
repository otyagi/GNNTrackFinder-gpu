/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfMaterialMapFactory.cxx
/// \brief  Utility to generate material budget map from the TGeoNavigator representation of the Setup (implementation)
/// \author Sergey Gorbunov <se.gorbunov@gsi.de>
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \date   29.08.2024

#include "KfMaterialMapFactory.h"

#include "KfMaterialMap.h"
#include "TGeoManager.h"
#include "TGeoMedium.h"
#include "TGeoNavigator.h"
#include "TGeoNode.h"
#include "TGeoPhysicalNode.h"
#include "TGeoVoxelFinder.h"

#include <Logger.h>

#include <sstream>
#include <thread>

using cbm::algo::kf::MaterialMap;
using kf::tools::MaterialMapFactory;

// ---------------------------------------------------------------------------------------------------------------------
//
MaterialMapFactory::MaterialMapFactory(int verbose) : fVerbose(verbose)
{
  // constructor

  // save the current number of threads to restore it when the work is done
  fNthreadsOld = gGeoManager->GetMaxThreads();

  // set defaut n threads to 1

  fNthreads = 1;

  std::stringstream msg;
  msg << "\n------------------------------------------------------------------------------------";
  msg << "\n                     Material budget map creator";
  msg << '\n';
  msg << "\n  !! To run it with the full speed, set: \"export OMP_NUM_THREADS=<n CPUs>\" before running !!";
  msg << '\n';
  // if possible, read the allowed n threads from the environment variable
  {
    const char* env = std::getenv("OMP_NUM_THREADS");
    if (env) {
      fNthreads = TString(env).Atoi();
      msg << "\n  found environment variable OMP_NUM_THREADS = \"" << env << "\", read as integer: " << fNthreads;
    }
  }

  // ensure that at least one thread is set

  if (fNthreads < 1) {
    fNthreads = 1;
  }

  msg << "\n  Maps will be created using " << fNthreads << " CPU threads";
  msg << '\n';
  msg << "\n  It might crash because of a ROOT bug. In this case do one of the following: ";
  msg << '\n';
  msg << "\n    - fix illegal overlaps in the geometry via gGeoManager->CheckOverlaps()";
  msg << "\n    - run with CbmL1::SetSafeMaterialInitialization() option. ";
  msg << "\n      It fixes the crash, but slows down the initialization significantly.";
  msg << "\n    - manually disable voxel finders for problematic volumes in CaToolsMaterialMapFactory.cxx";
  msg << '\n';
  msg << "\n------------------------------------------------------------------------------------";
  LOG_IF(info, fVerbose > 0) << msg.str();

  InitThreads();

  //
  // It looks like TGeoVoxelFinder contains a bug and therefore sometimes produces a crash.
  //
  // Currently, the crash only happens on some of TGeoVolumeAssembly volumes
  // and only when an (inproper?) alignment matrices are applied.
  //
  // Here we try to catch this case and disable those finders.
  //
  // Disabling all the voxel finders leads to a very long initialization, try to avoid it!
  //
  // TODO: make a bug report to the ROOT team; remove the following code once the bug is fixed.
  //

  if (fDoSafeInitialization) {  // disable all voxel finders
    TObjArray* volumes = gGeoManager->GetListOfVolumes();
    if (volumes) {
      for (int iv = 0; iv < volumes->GetEntriesFast(); iv++) {
        TGeoVolume* vol = dynamic_cast<TGeoVolume*>(volumes->At(iv));
        if (!vol) {
          continue;
        }
        TGeoVoxelFinder* finder = vol->GetVoxels();
        if (finder) {
          finder->SetInvalid();
        }
      }
    }
  }
  else {  // disable some voxel finders in some cases

    // check if any alignment was applied

    bool isAlignmentApplied = false;
    {
      TObjArray* nodes = gGeoManager->GetListOfPhysicalNodes();
      if (nodes) {
        for (int in = 0; in < nodes->GetEntriesFast(); in++) {
          TGeoPhysicalNode* node = dynamic_cast<TGeoPhysicalNode*>(nodes->At(in));
          if (!node) continue;
          if (node->IsAligned()) {
            isAlignmentApplied = true;
            break;
          }
        }
      }
    }

    // disable potentially problematic voxel finders

    if (isAlignmentApplied) {
      TObjArray* volumes = gGeoManager->GetListOfVolumes();
      if (volumes) {
        for (int iv = 0; iv < volumes->GetEntriesFast(); iv++) {
          TGeoVolumeAssembly* vol = dynamic_cast<TGeoVolumeAssembly*>(volumes->At(iv));
          if (!vol) {
            continue;
          }
          TGeoVoxelFinder* finder = vol->GetVoxels();
          if (finder) {
            finder->SetInvalid();
          }
        }
      }
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
MaterialMapFactory::~MaterialMapFactory()
{
  // destructor

  CleanUpThreads();

  // delete the navigators, created in this class

  for (auto i = fNavigators.begin(); i != fNavigators.end(); i++) {
    gGeoManager->RemoveNavigator(*i);
  }

  // once TGeoManager is swithched in multithreaded mode, there is no way to make it non-mltithreaded again
  // therefore we should set SetMaxThreads( >=0 )

  if (fNthreadsOld > 0) {
    gGeoManager->SetMaxThreads(fNthreadsOld);
  }
  else {
    gGeoManager->SetMaxThreads(1);
  }

  // ensure that the default navigator exists

  GetCurrentNavigator(-1);
}

// ---------------------------------------------------------------------------------------------------------------------
//
TGeoNavigator* MaterialMapFactory::GetCurrentNavigator(int iThread)
{
  // Get navigator for current thread, create it if it doesnt exist.
  // Produce an error when anything goes wrong

  TGeoNavigator* navi = gGeoManager->GetCurrentNavigator();
  if (!navi) {

    navi = gGeoManager->AddNavigator();

    if (iThread >= 0) {
      fNavigators.push_back(navi);
    }

    if (navi != gGeoManager->GetCurrentNavigator()) {
      LOG(fatal) << "ca::tools::MaterialMapFactory: unexpected behavior of TGeoManager::AddNavigator() !!";
    }
  }

  if (!navi) {
    LOG(fatal) << "ca::tools::MaterialMapFactory: can not find / create TGeoNavigator for thread " << iThread;
  }

  return navi;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MaterialMapFactory::CleanUpThreads()
{
  // clean up thread data in TGeoManager

  gGeoManager->ClearThreadsMap();

  // create a default navigator for multithreaded mode
  // otherwise gGeoManager->GetCurrentNavigator() returns nullptr that might produces crashes in other code

  GetCurrentNavigator(-1);
}

void MaterialMapFactory::InitThreads()
{
  // (re)initialise the number of threads in TGeoManager

  // reserve enough threads in TGeoManager
  if (fNthreads > gGeoManager->GetMaxThreads()) {
    gGeoManager->SetMaxThreads(fNthreads);
    // in case the number of threads is truncated by TGeoManager (must not happen)
    fNthreads = gGeoManager->GetMaxThreads();
    if (fNthreads < 1) {
      fNthreads = 1;
    }
  }
  CleanUpThreads();
}

// ---------------------------------------------------------------------------------------------------------------------
//
MaterialMap MaterialMapFactory::GenerateMaterialMap(double zRef, double zMin, double zMax, double xyMax, int nBinsDim)
{
  if (fDoRadialProjection) {
    if (fTargetZ + 0.05 >= zRef) {
      LOG(warn)
        << "kf::tools::MaterialMapFactory: the material reference z-coordinate must be at least 0.1 cm downstream "
           "the target. Shifting the reference z-coordinate to the lower limit: target Z = "
        << fTargetZ << ", material reference z = " << zRef;
    }
    zRef = std::max(zRef, fTargetZ + 0.05);
    zMin = std::max(fTargetZ, zMin);
    zMax = std::max(zMax, zRef + 0.05);
  }

  if (!(zMin <= zRef && zRef <= zMax)) {
    LOG(fatal) << "kf::tools::MaterialMapFactory: the material parameters are inconsistent. It must be: zMin " << zMin
               << " <= zRef " << zRef << " <= zMax " << zMax;
  }

  MaterialMap matBudget(nBinsDim, xyMax, zRef, zMin, zMax);
  double binSizeX = 2. * xyMax / nBinsDim;
  double binSizeY = 2. * xyMax / nBinsDim;

  // call it every time. for the case the number of threads was reset in some other code
  InitThreads();

  long nThreadCrosses[fNthreads];
  long nThreadRays[fNthreads];
  long nThreadRaysExpected = nBinsDim * nBinsDim * fNraysBinPerDim * fNraysBinPerDim / fNthreads;
  for (int i = 0; i < fNthreads; i++) {
    nThreadCrosses[i] = 0;
    nThreadRays[i]    = 0;
  }

  LOG_IF(info, fVerbose > -1) << "kf::tools::MaterialMapFactory: Generate material map using " << fNthreads
                              << " threads..";

  auto naviThread = [&](int iThread) {
    // create a navigator for this thread
    TGeoNavigator* navi = GetCurrentNavigator(iThread);

    for (int iBinX = iThread; iBinX < nBinsDim; iBinX += fNthreads) {
      for (int iBinY = 0; iBinY < nBinsDim; ++iBinY) {
        double radThick = 0.;
        int nRays       = 0;
        double d        = 1. / (fNraysBinPerDim);
        for (int iStepX = 0; iStepX < fNraysBinPerDim; iStepX++) {
          for (int iStepY = 0; iStepY < fNraysBinPerDim; iStepY++) {

            // ray position at zRef
            double rayX = -xyMax + (iBinX + d * (iStepX + 0.5)) * binSizeX;
            double rayY = -xyMax + (iBinY + d * (iStepY + 0.5)) * binSizeY;

            // ray slopes
            double tx = 0.;
            double ty = 0.;
            double t  = 1.;
            if (fDoRadialProjection) {
              tx = rayX / (zRef - fTargetZ);
              ty = rayY / (zRef - fTargetZ);
              t  = sqrt(1. + tx * tx + ty * ty);
            }

            // ray position at zMin
            double z = zMin;
            double x = rayX + tx * (z - zRef);
            double y = rayY + ty * (z - zRef);
            LOG_IF(info, fVerbose > 2) << "ray at " << x << " " << y << " " << z << " zMin " << zMin << " zMax "
                                       << zMax;

            TGeoNode* node = navi->InitTrack(x, y, z, tx / t, ty / t, 1. / t);

            bool doContinue = 1;
            for (int iStep = 0; doContinue; iStep++) {

              if (!node) {
                // may happen when tracing outside of the CBM volume -> produce a warning
                LOG(warning) << "kf::tools::MaterialMapFactory: TGeoNavigator can not find the geo node";
                break;
              }

              TGeoMedium* medium = node->GetMedium();
              if (!medium) {
                LOG(fatal) << "kf::tools::MaterialMapFactory: TGeoNavigator can not find the geo medium";
              }

              TGeoMaterial* material = medium->GetMaterial();
              if (!material) {
                LOG(fatal) << "kf::tools::MaterialMapFactory: TGeoNavigator can not find the geo material";
              }

              double radLen = material->GetRadLen();

              if (radLen < kMinRadLength) {  // 0.5612 is rad. length of Lead at normal density
                LOG(fatal) << "kf::tools::MaterialMapFactory: failed assertion! "
                           << "     TGeoNavigator returns a suspicious material with an unrealistic "
                           << "radiation length of " << radLen << " cm. "
                           << "     The allowed minimum is 0.56 cm (Lead has 0.5612 cm). Check your material! "
                              "Modify this assertion if necessary."
                           << "     TGeoNode \"" << node->GetName() << "\", TGeoMedium \"" << medium->GetName()
                           << "\", TGeoMaterial \"" << material->GetName() << "\"";
              }

              // move navi to the next material border
              node = navi->FindNextBoundaryAndStep();  //5., kTRUE);
              nThreadCrosses[iThread]++;

              LOG_IF(info, fVerbose > 2) << "   RL " << radLen << " next pt " << navi->GetCurrentPoint()[0] << " "
                                         << navi->GetCurrentPoint()[1] << " " << navi->GetCurrentPoint()[2];

              double zNew = navi->GetCurrentPoint()[2];
              if (zNew >= zMax) {
                zNew       = zMax;
                doContinue = 0;
              }

              //if (zNew < z) {
              //LOG(info) << " z curr " << z << " z new " << zNew << " dz " << zNew - z ; }
              double dz = zNew - z;
              if (dz < 0.) {
                if (iStep == 0 && dz > -1.e-8) {
                  // when the ray starts exactly at the volume border, first dz might become negative,
                  // probably due to some roundoff errors. So we let it be a bit negative for the first intersection.
                  dz = 0.;
                }
                else {
                  LOG(fatal)
                    << "kf::tools::MaterialMapFactory: TGeoNavigator propagates the ray upstream. Something is wrong."
                    << " z old " << z << " z new " << zNew << ", dz " << dz;
                }
              }
              radThick += dz / radLen;
              z = zNew;
            }

            nRays++;
            nThreadRays[iThread]++;
            if (fVerbose > 2 && nThreadRays[iThread] % 1000000 == 0) {
              LOG(info) << "kf::tools::MaterialMapFactory:  report from thread " << iThread << ":   material map is "
                        << 100. * nThreadRays[iThread] / nThreadRaysExpected << " \% done";
            }
          }
        }
        radThick /= nRays;
        LOG_IF(info, fVerbose > 2) << "   radThick " << radThick;
        //doPrint = (radThick > 0.01);
        matBudget.SetRadThickBin(iBinX, iBinY, radThick);
      }  // iBinX
    }    // iBinY
  };

  std::vector<std::thread> threads(fNthreads);

  // run n threads
  for (int i = 0; i < fNthreads; i++) {
    threads[i] = std::thread(naviThread, i);
  }

  // wait for the threads to finish
  for (auto& th : threads) {
    th.join();
  }

  CleanUpThreads();
  long nRays    = 0;
  long nCrosses = 0;
  for (int i = 0; i < fNthreads; i++) {
    nRays += nThreadRays[i];
    nCrosses += nThreadCrosses[i];
  }

  LOG_IF(info, fVerbose > 0) << "kf::tools::MaterialMapFactory:     shooted " << nRays << " rays. Each ray crossed "
                             << nCrosses * 1. / nRays << " material boundaries in average";

  return matBudget;
}
