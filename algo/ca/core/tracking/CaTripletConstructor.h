/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer] */

/// \file CaTripletConstructor.h
/// \brief Triplet constructor for the CA tracker
/// \author Sergey Gorbunov

#pragma once  // include this header only once per compilation unit

#include "CaFramework.h"
#include "CaGridEntry.h"
#include "CaStation.h"
#include "CaTriplet.h"
#include "CaVector.h"
#include "CaWindowData.h"
#include "KfFieldRegion.h"
#include "KfSetup.h"
#include "KfSimd.h"
#include "KfTrackKalmanFilter.h"
#include "KfTrackParam.h"

namespace cbm::algo::ca
{

  /// Construction of triplets for the CA tracker
  ///
  class alignas(kf::VcMemAlign) TripletConstructor {

   public:
    ///                             ------  Constructors and destructor ------

    /// Constructor
    /// \param nThreads  Number of threads for multi-threaded mode
    TripletConstructor(const ca::Parameters<fvec>& pars, WindowData& wData, const fscal mass,
                       const ca::TrackingMode& mode);

    /// Copy constructor
    TripletConstructor(const TripletConstructor&) = delete;

    /// Move constructor
    TripletConstructor(TripletConstructor&&) = delete;

    /// Copy assignment operator
    TripletConstructor& operator=(const TripletConstructor&) = delete;

    /// Move assignment operator
    TripletConstructor& operator=(TripletConstructor&&) = delete;

    /// Destructor
    ~TripletConstructor() = default;

    ///                             ------  FUNCTIONAL PART ------
    void CreateTripletsForHit(Vector<ca::Triplet>& tripletsOut, int istal, int istam, int istar, ca::HitIndex_t ihl);

   private:
    typedef std::pair<Vector<TrackParamV>, Vector<ca::HitIndex_t>> Doublet_t;
    typedef std::tuple<Vector<TrackParamV>, Vector<ca::HitIndex_t>, Vector<ca::HitIndex_t>> Triplet_t;

    bool InitStations(int istal, int istam, int istar);

    /// Find the doublets. Reformat data in the portion of doublets.
    void FindDoublets(kf::TrackKalmanFilter<fvec>& fit);

    /// Add the middle hits to parameters estimation. Propagate to right station.
    /// Find the triplets (right hit). Reformat data in the portion of triplets.
    void FindTripletHits();

    /// Find triplets on station
    void FindTriplets();

    /// Select good triplets.
    void SelectTriplets(Vector<ca::Triplet>& tripletsOut);

    void CollectHits(Vector<ca::HitIndex_t>& collectedHits, kf::TrackKalmanFilter<fvec>& fit, const int iSta,
                     const double chi2Cut, const int iMC, const int maxNhits);

   private:
    const Parameters<fvec>& fParameters;       ///< Object of Framework parameters class
    const cbm::algo::kf::Setup<fvec>& fSetup;  ///< Reference to the setup
    WindowData& frWData;
    fscal fDefaultMass{constants::phys::MuonMass};  ///< mass of the propagated particle [GeV/c2]
    ca::TrackingMode fTrackingMode;

    bool fIsTargetField{false};  ///< is the magnetic field present at the target

    // Number of stations situated in field region (MVD + STS in CBM)
    int fNfieldStations{0};

    int fIstaL{-1};  ///< left station index
    int fIstaM{-1};  ///< middle station index
    int fIstaR{-1};  ///< right station index

    const ca::Station<fvec>* fStaL{nullptr};  ///< left station
    const ca::Station<fvec>* fStaM{nullptr};  ///< mid station
    const ca::Station<fvec>* fStaR{nullptr};  ///< right station

    const ca::Station<fvec>*
      fFld0Sta[2];  // two stations for approximating the field between the target and the left hit
    const ca::Station<fvec>*
      fFld1Sta[3];  // three stations for approximating the field between the left and the right hit

    ca::HitIndex_t fIhitL;  ///< index of the left hit in fAlgo->fWindowHits
    kf::FieldRegion<fvec> fFldL;

    // Persistent storage for triplet tracks and hits
    Triplet_t fTripletData;

    // Persistent storage for doublet tracks and hits
    Doublet_t fDoubletData;

   private:
    static constexpr bool fDebugDublets     = false;  // print debug info for dublets
    static constexpr bool fDebugTriplets    = false;  // print debug info for triplets
    static constexpr bool fDebugCollectHits = false;  // print debug info for CollectHits
  };

}  // namespace cbm::algo::ca
