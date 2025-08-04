/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer], Maksym Zyzak */

/// \file    TrackExtender.h
/// \authors S.Zharko <s.zharko@gsi.de> (interface), M.Zyzak (original algorithm)
/// \brief   A class wrapper over clones merger algorithm for the CA track finder (declaration)
/// \since   22.07.2022

#pragma once  // include this header only once per compilation unit

#include "CaBranch.h"
#include "CaHit.h"
#include "CaParameters.h"
#include "CaSimd.h"
#include "CaVector.h"
#include "CaWindowData.h"
#include "KfTrackKalmanFilter.h"
#include "KfTrackParam.h"

namespace cbm::algo::ca
{

  class Track;
  class Framework;
  class InputData;

  /// Class implements a clones merger algorithm for the CA track finder
  ///
  class TrackExtender {

   public:
    /// Default constructor
    TrackExtender(const ca::Parameters<fvec>& pars, const fscal mass);

    /// Destructor
    ~TrackExtender();

    /// Copy constructor
    TrackExtender(const TrackExtender&) = default;

    /// Move constructor
    TrackExtender(TrackExtender&&) = default;

    /// Copy assignment operator
    TrackExtender& operator=(const TrackExtender&) = delete;

    /// Move assignment operator
    TrackExtender& operator=(TrackExtender&&) = delete;


    /// Find additional hits for existing track both downstream and upstream
    /// \return chi2
    fscal ExtendBranch(ca::Branch& t, WindowData& wData);

   private:
    ///-------------------------------
    /// Private methods

    /// Finds additional hits for already found track in the given direction
    /// \param t - track with hits
    /// \param T - track params
    /// \param dir - 0 - forward, 1 - backward
    /// \param qp0 - momentum value to linearize the extrapolation
    void FindMoreHits(ca::Branch& t, TrackParamV& T, const kf::FitDirection direction, const fvec qp0);

    /// Fits the branch. Does few passes over the hits.
    /// \param t - track branch with hits
    /// \param T - track parameters
    /// \param dir - false - forward, true - backward
    /// \param qp0 - momentum value to linearize the extrapolation
    /// \param initParams - should params be ititialized. 1 - yes.
    void FitBranch(const ca::Branch& t, TrackParamV& T, const kf::FitDirection direction, const fvec qp0,
                   const bool initParams = true);


    /// Fits the branch. Does only one pass over the hits.
    /// \param t - track branch with hits
    /// \param T - track parameters
    /// \param dir - false - forward, true - backward
    /// \param qp0 - momentum value to linearize the extrapolation
    /// \param initParams - should params be ititialized. 1 - yes.
    void FitBranchFast(const ca::Branch& t, TrackParamV& T, const kf::FitDirection direction, const fvec qp0,
                       const bool initParams = true);

   private:
    ///-------------------------------
    /// Data members

    const Parameters<fvec>& fParameters;  ///< Object of Framework parameters class
    const cbm::algo::kf::Setup<fvec>& fSetup;
    WindowData* frWData;
    fscal fDefaultMass{constants::phys::MuonMass};  ///< mass of the propagated particle [GeV/c2]
  };

}  // namespace cbm::algo::ca
