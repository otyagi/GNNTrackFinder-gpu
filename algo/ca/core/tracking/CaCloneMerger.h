/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer], Maksym Zyzak */

/// \file    CloneMerger.h
/// \authors S.Zharko <s.zharko@gsi.de> (interface), M.Zyzak (original algorithm)
/// \brief   A class wrapper over clones merger algorithm for the Ca track finder (declaration)
/// \since   22.07.2022

#pragma once  // include this header only once per compilation unit

#include "CaHit.h"  // For ca::HitIndex_t
#include "CaInputData.h"
#include "CaParameters.h"
#include "CaSimd.h"  // TEMPORARY FOR fvec, fscal
#include "CaTrack.h"
#include "CaVector.h"
#include "CaWindowData.h"

namespace cbm::algo::ca
{
  class Track;
  class Framework;

  /// Class implements a clones merger algorithm for the CA track finder
  ///
  class CloneMerger {
   public:
    /// Default constructor
    CloneMerger(const ca::Parameters<fvec>& pars, const fscal mass);

    /// Destructor
    ~CloneMerger();

    /// Copy constructor
    CloneMerger(const CloneMerger&) = default;

    /// Move constructor
    CloneMerger(CloneMerger&&) = default;

    /// Copy assignment operator
    CloneMerger& operator=(const CloneMerger&) = delete;

    /// Move assignment operator
    CloneMerger& operator=(CloneMerger&&) = delete;

    /// Registers

    /// Executes track clones merging algorithm and updates input containers
    /// \param  input       Reference to input data
    /// \param  wData       Reference to the external container of reconstructed tracks and hit indices
    //void Exec(Vector<Track>& extTracks, Vector<ca::HitIndex_t>&, const ca::InputData& input);
    void Exec(const ca::InputData& input, WindowData& wData);

   private:
    // ***************
    // ** Functions **
    // ***************

    ///
    static void InvertCholesky(fvec a[15]);

    /// Multiplication of two symmetric matryces 5x5
    /// \param  C  Left matrix:
    ///      C[0]  C[1]  C[3]  C[6]  C[10]
    ///      C[1]  C[2]  C[4]  C[7]  C[11]
    /// C =  C[3]  C[4]  C[5]  C[8]  C[12]
    ///      C[6]  C[7]  C[8]  C[9]  C[13]
    ///      C[10] C[11] C[12] C[13] C[14]
    /// \param  V  Right matrix:
    ///      V[0]  V[1]  V[3]  V[6]  V[10]
    ///      V[1]  V[2]  V[4]  V[7]  V[11]
    /// V =  V[3]  V[4]  V[5]  V[8]  V[12]
    ///      V[6]  V[7]  V[8]  V[9]  V[13]
    ///      V[10] V[11] V[12] V[13] V[14]
    /// \param  K  Output: K = C * V
    static void MultiplySS(fvec const C[15], fvec const V[15], fvec K[5][5]);

    ///
    static void MultiplyMS(fvec const C[5][5], fvec const V[15], fvec K[15]);

    ///
    static void MultiplySR(fvec const C[15], fvec const r_in[5], fvec r_out[5]);

    ///
    static void FilterTracks(fvec const r[5], fvec const C[15], fvec const m[5], fvec const V[15], fvec R[5],
                             fvec W[15], fvec* chi2);


    // ***************
    // ** Variables **
    // ***************

    /// First station of a track
    Vector<unsigned short> fTrackFirstStation{"CloneMerger::fTrackFirstStation"};

    /// Last station of a track
    Vector<unsigned short> fTrackLastStation{"CloneMerger::fTrackLastStation"};

    /// Index of the first hit of a track
    Vector<ca::HitIndex_t> fTrackFirstHit{"CloneMerger::fTrackFirstHit"};

    /// Index of the last hit of a track
    Vector<ca::HitIndex_t> fTrackLastHit{"CloneMerger::fTrackLastHit"};

    /// Index (TODO:??) of a track that can be merge with the given track
    Vector<unsigned short> fTrackNeighbour{"CloneMerger::fTrackNeighbour"};

    /// Chi2 value of the track merging procedure
    Vector<fscal> fTrackChi2{"CloneMerger::fTrackChi2"};

    /// Flag: is the given track already stored to the output
    Vector<char> fTrackIsStored{"CloneMerger::fTrackIsStored"};

    /// Flag: is the track a downstream neighbour of another track
    Vector<char> fTrackIsDownstreamNeighbour{"CloneMerger::fTrackIsDownstreamNeighbour"};

    Vector<Track> fTracksNew{"CaCloneMerger::fTracksNew"};  ///< vector of tracks after the merge

    Vector<ca::HitIndex_t> fRecoHitsNew{"CaCloneMerger::fRecoHitsNew"};  ///< vector of track hits after the merge

    const Parameters<fvec>& fParameters;            ///< Object of Framework parameters class
    fscal fDefaultMass{constants::phys::MuonMass};  ///< mass of the propagated particle [GeV/c2]
  };

}  // namespace cbm::algo::ca
