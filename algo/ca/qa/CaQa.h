/* Copyright (C) 2023-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CaQa.h
/// \date   20.11.2023
/// \brief  A QA module for CA tracking (header)
/// \author S.Zharko <s.zharko@gsi.de>

#pragma once

#include "CaEnumArray.h"
#include "CaHit.h"  // for HitIndex_t
#include "CaTimesliceHeader.h"
#include "CaVector.h"
#include "qa/QaTaskHeader.h"

namespace cbm::algo
{
  namespace qa
  {
    class H1D;
    class H2D;
    class Manager;
  }  // namespace qa
  namespace ca
  {
    template<typename DataT>
    class Parameters;
    class InputData;
    class Track;
  }  // namespace ca
}  // namespace cbm::algo

namespace cbm::algo::ca
{
  /// \class cbm::algo::ca::qa::Qa
  /// \brief Qa class for the CA tracking QA (header)
  ///
  class Qa : public qa::TaskHeader {
    /// \brief Hit set entries
    enum class EHitSet
    {
      Input,  ///< Input hits
      Used,   ///< Hits used in tracks
      END
    };

    /// \brief Definition of enum array over EHitSet entries
    template<typename T>
    using HitSetArray_t = EnumArray<EHitSet, T>;

    /// \brief Array of EHitSet entries for iteration
    static constexpr HitSetArray_t<EHitSet> kHitSets = {EHitSet::Input, EHitSet::Used};

   public:
    /// \brief Constructor
    /// \param pManager  Pointer to the QA manager
    /// \param name      Name of the QA (directory)
    Qa(const std::unique_ptr<qa::Manager>& pManager, std::string_view name) : qa::TaskHeader(pManager, name) {}

    /// \brief Constructor from the configuration object
    /// \param config  QA configuration object
    Qa() = default;

    /// \brief Copy constructor
    Qa(const Qa&) = delete;

    /// \brief Move constructor
    Qa(Qa&&) = delete;

    /// \brief Destructor
    ~Qa() = default;

    /// \brief Copy assignment operator
    Qa& operator=(const Qa&) = delete;

    /// \brief Move assignment operator
    Qa& operator=(Qa&&) = delete;

    /// \brief QA execution function
    void Exec();

    /// \brief Initializes the QA
    void Init();

    /// \brief Check initialization
    /// \return true   All variables are initialized
    /// \return false  Some of are not initialized
    bool CheckInit() const;

    /// \brief Registers tracking input data object
    /// \note  Call per TS
    void RegisterInputData(const InputData* pInputData) { fpInputData = pInputData; }

    /// \brief Registers track vector
    /// \note  Call per TS
    void RegisterTracks(const Vector<Track>* pvTracks) { fpvTracks = pvTracks; }

    /// \brief Registers reco hits indices vector
    /// \note  Call per TS
    void RegisterRecoHitIndices(const Vector<HitIndex_t>* pvRecoHits) { fpvRecoHits = pvRecoHits; }

    /// \brief Registers tracking parameters object
    /// \note  Call per run
    void RegisterParameters(const Parameters<fvec>* pParameters) { fpParameters = pParameters; }

   private:
    /// \brief Fills hit distributions
    /// \param  hitSet  Hit set enum entry
    /// \param  hit     Reference to hit
    void FillHitDistributionsForHitSet(EHitSet hitSet, const ca::Hit& hit);

    // parameters
    static constexpr double kXYZMargin  = 0.05;   ///< Margin for occupancy distributions in XY plane
    static constexpr int knHitSets      = 2;      ///< Number of hit sets: input/used
    static constexpr int knTrkParPoints = 2;      ///< Number of track points to build par distributions
    static constexpr int knStaMax       = 16;     ///< Max number of stations (histogram binning)
    static constexpr bool kDebug        = false;  ///< Additional histograms
    static constexpr int kOriginB       = 400;    ///< Track X(Y) at origin: n bins
    static constexpr double kOriginL    = -1.;    ///< Track X(Y) at origin: lower bound [cm]
    static constexpr double kOriginU    = +1.;    ///< Track X(Y) at origin: upper bound [cm]

    double fMinHitTime = std::numeric_limits<double>::max();
    double fMaxHitTime = std::numeric_limits<double>::lowest();

    const Parameters<fvec>* fpParameters      = nullptr;  ///< Pointer to tracking parameters
    const InputData* fpInputData              = nullptr;  ///< Pointer to input data
    const Vector<Track>* fpvTracks            = nullptr;  ///< Pointer to tracks vector
    const Vector<HitIndex_t>* fpvRecoHits     = nullptr;  ///< Pointer to reco hit indices

    // Hit distributions
    using OccupHistContainer_t = std::vector<HitSetArray_t<qa::H2D*>>;
    OccupHistContainer_t fvphHitOccupXY;  ///< hist: Hit occupancy in different stations in XY plane
    OccupHistContainer_t fvphHitOccupZX;  ///< hist: Hit occupancy in different stations in ZX plane
    OccupHistContainer_t fvphHitOccupZY;  ///< hist: Hit occupancy in different stations in ZY plane

    std::vector<qa::Prof2D*> fvphHitUsageXY;  ///< prof: Hit usage in different stations in XY plane

    HitSetArray_t<qa::H1D*> fvphHitFrontKeyIndex = {nullptr, nullptr};  ///< Indices of front hit keys
    HitSetArray_t<qa::H1D*> fvphHitBackKeyIndex  = {nullptr, nullptr};  ///< Indices of back hit keys

    std::vector<HitSetArray_t<qa::H1D*>> fvphHitTime;  ///< Time distribution of hits

    // Track distributions
    std::array<qa::H1D*, knTrkParPoints> fvphTrkTheta    = {{0}};  ///< hist: theta at first/last hit
    std::array<qa::H1D*, knTrkParPoints> fvphTrkPhi      = {{0}};  ///< hist: phi at first/last hit
    std::array<qa::H1D*, knTrkParPoints> fvphTrkChi2Ndf  = {{0}};  ///< hist: chi2/NDF at first/last hit
    std::array<qa::H2D*, knTrkParPoints> fvphTrkPhiTheta = {{0}};  ///< hist: theta vs. phi at first/last hit

    qa::H2D* fphTrkOriginXY  = nullptr;  ///< hist: origin of tracks in x-y plane [cm]
    qa::H2D* fphTrkFstLstSta = nullptr;  ///< hist: fst vs lst station index
    qa::H1D* fphTrkNofHits   = nullptr;  ///< hist: number of hits in track
  };
}  // namespace cbm::algo::ca
