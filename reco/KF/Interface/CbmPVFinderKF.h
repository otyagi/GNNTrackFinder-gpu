/* Copyright (C) 2006-2016 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Denis Bertini [committer], Volker Friese, Sergei Zharko */

/// \file   CbmPVFinderKF.h
/// \author S.Gorbunov

#ifndef CBMKFPVFINDERKF_H
#define CBMKFPVFINDERKF_H 1

#include "CbmPrimaryVertexFinder.h"

/// \class  CbmPVFinderKF
/// \brief  Implementation of the primary vertex finder using KF utility
///
class CbmPVFinderKF : public CbmPrimaryVertexFinder {
 public:
  /// \brief Track type for PV recnostruction
  enum ESourceTrackType
  {
    kStsTrack    = 0,
    kGlobalTrack = 1
  };

  /// \brief Default constructor
  CbmPVFinderKF(){};

  /// \brief Destructior
  ~CbmPVFinderKF(){};

  /// \brief Execution of PV finding.
  /// \param tracks   TClonesArray of CbmStsTracks
  /// \param vertex   Primary vertex (output)
  /// \param event    Pointer to event object
  virtual Int_t FindPrimaryVertex(TClonesArray* tracks, CbmVertex* vertex);


  /// \brief Execution of PV finding.
  /// \param event    Pointer to event object
  /// \param tracks   TClonesArray of CbmStsTracks
  virtual Int_t FindEventVertex(CbmEvent* event, TClonesArray* tracks);

  ClassDef(CbmPVFinderKF, 1);
};

#endif
