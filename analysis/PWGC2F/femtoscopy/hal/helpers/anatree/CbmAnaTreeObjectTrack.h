/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef CBMROOT_ANALYSIS_PWGC2F_FEMTOSCOPY_NICAFEMTO_FORMAT_ANATREE_CBMANATREEOBJECTTRACK_H_
#define CBMROOT_ANALYSIS_PWGC2F_FEMTOSCOPY_NICAFEMTO_FORMAT_ANATREE_CBMANATREEOBJECTTRACK_H_

#include <TObject.h>

#include <AnalysisTree/Detector.hpp>
#include <AnalysisTree/Hit.hpp>
#include <AnalysisTree/Particle.hpp>

class CbmAnaTreeObjectTrack : public TObject {
  AnalysisTree::Particle* fTreeParticle = {nullptr};
  AnalysisTree::Track* fTreeTrack       = {nullptr};
  AnalysisTree::Hit* fTreeHit           = {nullptr};

 public:
  CbmAnaTreeObjectTrack();
  AnalysisTree::Particle* GetTreeParticle() const { return fTreeParticle; };
  AnalysisTree::Track* GetTreeTrack() const { return fTreeTrack; };
  AnalysisTree::Hit* GetTreeHit() const { return fTreeHit; };
  void SetTreeParticle(AnalysisTree::Particle* p) { fTreeParticle = p; };
  void SetTreeTrack(AnalysisTree::Track* t) { fTreeTrack = t; };
  void SetTreeTof(AnalysisTree::Hit* h) { fTreeHit = h; };
  virtual ~CbmAnaTreeObjectTrack();
  CbmAnaTreeObjectTrack(const CbmAnaTreeObjectTrack& other);
  CbmAnaTreeObjectTrack& operator=(const CbmAnaTreeObjectTrack& other);
  ClassDef(CbmAnaTreeObjectTrack, 1)
};

#endif /* CBMROOT_ANALYSIS_PWGC2F_FEMTOSCOPY_NICAFEMTO_FORMAT_ANATREE_CBMANATREEOBJECTTRACK_H_ */
