/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef CBMROOT_ANALYSIS_PWGC2F_FEMTOSCOPY_NICAFEMTO_HELPERS_CBMQACOREMANAGER_H_
#define CBMROOT_ANALYSIS_PWGC2F_FEMTOSCOPY_NICAFEMTO_HELPERS_CBMQACOREMANAGER_H_

#include <Hal/QAManager.h>
#include <Hal/QAManagerBasic.h>

namespace Hal
{
  class Event;
  class TrackAna;
  class TwoTrackAna;
}  // namespace Hal
class CbmQACoreManager : public Hal::Fair::QAManagerBasic {
 public:
  CbmQACoreManager();
  virtual FairRunAna* GetRunAna(TString outFile, TString simFile, TString recoFile, TString parFile = "");
  virtual Hal::Event* GetFormat(Hal::QAManager::eFormatType type,
                                Hal::QAManager::eAnaType ana = Hal::QAManager::eAnaType::kDefault);
  virtual void SetRecoTrackCut(Hal::TrackAna* ana, Hal::QAManager::ePidCut cut, Hal::QAManager::eParticleType primary,
                               TString flag = "");
  virtual void SetEventCut(Hal::TrackAna* /*ana*/, Int_t /*col*/, TString /*flag = ""*/){};
  virtual void SetPairCut(Hal::TwoTrackAna* /*ana*/, Hal::QAManager::ePidCut /*pid1*/,
                          Hal::QAManager::ePidCut /*pid2*/){};
  virtual ~CbmQACoreManager();
  ClassDef(CbmQACoreManager, 1)
};


#endif /* CBMROOT_ANALYSIS_PWGC2F_FEMTOSCOPY_NICAFEMTO_HELPERS_CBMQACOREMANAGER_H_ */
