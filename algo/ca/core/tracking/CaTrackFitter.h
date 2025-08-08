/* Copyright (C) 2023-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer], Maksym Zyzak */

/// \file    TrackFitter.h
/// \brief   A class wrapper over clones merger algorithm for the Ca track finder (declaration)
/// \since   19.10.2023

#pragma once  // include this header only once per compilation unit

#include "CaBranch.h"
#include "CaHit.h"
#include "CaInputData.h"
#include "CaParameters.h"
#include "CaSimd.h"
#include "CaVector.h"
#include "CaWindowData.h"
#include "KfTrackParam.h"


namespace cbm::algo::ca
{
  class Track;

  /// Class implements a track fit the CA track finder
  ///
  class TrackFitter {
   public:
    /// Default constructor
    TrackFitter(const ca::Parameters<fvec>& pars, const fscal mass, const ca::TrackingMode& mode);

    /// Destructor
    ~TrackFitter();

    /// Copy constructor
    TrackFitter(const TrackFitter&) = default;

    /// Move constructor
    TrackFitter(TrackFitter&&) = default;

    /// Copy assignment operator
    TrackFitter& operator=(const TrackFitter&) = delete;

    /// Move assignment operator
    TrackFitter& operator=(TrackFitter&&) = delete;

    /// Fit tracks, found by the CA tracker
    void FitCaTracks(const ca::InputData& input, WindowData& wData);

    /// Fit triplets found by GNN
    void FitGNNTriplets(const ca::InputData& input, WindowData& wData, Vector<Track>& tripletCandidates,
                                     Vector<HitIndex_t>& tripletHits, Vector<int>& selectedTripletIndexes,
                                     Vector<float>& selectedTripletScores,
                                     std::vector<std::vector<float>>& selectedTripletParams, const int GNNiteration);


   private:
    ///-------------------------------
    /// Data members

    const Parameters<fvec>& fParameters;            ///< Object of Framework parameters class
    const cbm::algo::kf::Setup<fvec>& fSetup;       ///< Setup instance
    fscal fDefaultMass{constants::phys::MuonMass};  ///< mass of the propagated particle [GeV/c2]
    ca::TrackingMode fTrackingMode;
  };

}  // namespace cbm::algo::ca
