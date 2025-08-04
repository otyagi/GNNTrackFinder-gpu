/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file    CbmPVFinderKFGlobal.h
/// \brief   Primary vertex finder from the global tracks (header)
/// \since   09.10.2023
/// \authors Anna Senger, Sergei Zharko

#ifndef CbmPVFinderKFGlobal_h
#define CbmPVFinderKFGlobal_h 1

#include "CbmPrimaryVertexFinder.h"

/// \class  CbmPVFinderKFGlobal
/// \brief  Implementation of the primary vertex finder using KF utility
///
class CbmPVFinderKFGlobal : public CbmPrimaryVertexFinder {
 public:
  /// \brief Default constructor
  CbmPVFinderKFGlobal() = default;

  /// \brief Destructior
  ~CbmPVFinderKFGlobal() = default;

  /// \brief Execution of PV finding.
  /// \param tracks   TClonesArray of CbmStsTracks
  /// \param vertex   Primary vertex (output)
  /// \param event    Pointer to event object
  virtual Int_t FindPrimaryVertex(TClonesArray* tracks, CbmVertex* vertex);

  /// \brief Execution of PV finding.
  /// \param event    Pointer to event object
  /// \param tracks   TClonesArray of CbmStsTracks
  virtual Int_t FindEventVertex(CbmEvent* event, TClonesArray* tracks);

  ClassDef(CbmPVFinderKFGlobal, 1);
};

#endif
