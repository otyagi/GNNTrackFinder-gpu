/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmKFV0FinderQa.h
/// \brief  A simple QA for the V0 finder class (runs together with the finder task)
/// \since  15.01.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

// FIXME: Later move this class into a separate QA-task

#pragma once

#include "CbmQaIO.h"

#include <TString.h>

#include <array>
#include <string>
#include <vector>

class TH1D;
class TH2D;

namespace cbm::kfp
{
  /// \class V0FinderQa
  /// \brief A simple QA for the V0 finder
  class V0FinderQa : public CbmQaIO {
   public:
    /// \brief  Constructor from parameters
    /// \param  bUseMc  If MC truth should be used
    V0FinderQa(bool bUseMc) : CbmQaIO("v0Finder"), fbUseMc(bUseMc) {}

    /// \brief Destructor
    ~V0FinderQa() = default;

    V0FinderQa(const V0FinderQa&) = delete;
    V0FinderQa(V0FinderQa&&)      = delete;
    V0FinderQa& operator=(const V0FinderQa&) = delete;
    V0FinderQa& operator=(V0FinderQa&&) = delete;

    /// \brief Initializes histograms
    void InitHistograms();

    /// \brief Writes histograms into file
    void WriteHistograms(const TString&);

    //* Histograms (public for easier access)
    TH1D* fph_tof_lst_hit_time{nullptr};             ///< Time of tof hits, used for track momentum estimation
    TH1D* fph_beta_all{nullptr};                     ///< Beta of tracks
    TH1D* fph_dca{nullptr};                          ///< Track DCA to origin
    TH2D* fph_dca2D{nullptr};                        ///< Track DCA to origin (2D)
    TH1D* fph_dca_projectionX{nullptr};              ///< Track DCA to origin (x-component)
    TH1D* fph_dca_projectionY{nullptr};              ///< Track DCA to origin (y-component)
    TH2D* fph_track_rapidity_vs_pt_all{nullptr};     ///< Phase space of all accepted tracks
    TH2D* fph_track_rapidity_vs_pt_pion{nullptr};    ///< Phase space of pion candidates
    TH2D* fph_track_rapidity_vs_pt_proton{nullptr};  ///< Phase space of proton candidates
    TH1D* fph_lambda_cand_mass{nullptr};             ///< Mass of lambda candidates

   private:
    bool fbUseMc{false};

    ClassDef(V0FinderQa, 0);
  };
}  // namespace cbm::kfp
