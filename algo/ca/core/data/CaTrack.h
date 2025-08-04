/* Copyright (C) 2010-2023 Frankfurt Institute for Advanced Studies, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Ivan Kisel,  Sergey Gorbunov, Maksym Zyzak, Igor Kulakov [committer], Sergei Zharko */

/// \file   CaTrack.h
/// \brief  header file for the ca::Track class
/// \since  02.06.2022
/// \author S.Zharko <s.zharko@gsi.de>

#ifndef CA_CORE_CaTrack_h
#define CA_CORE_CaTrack_h 1

#include "CaDefs.h"
#include "CaSimd.h"
#include "KfTrackParam.h"

#include <boost/serialization/access.hpp>

namespace cbm::algo::ca
{

  /// \class cbm::algo::ca::Track
  /// \brief Class representing an output track in the CA tracking algorithm
  ///
  /// Track parameters vector: {x, y, tx, ty, q/p, z, t, vi}
  /// Covariation matrix: C[20] (C55) corresponds to the time variance
  ///
  class Track {
   public:
    using TrackParam_t = cbm::algo::kf::TrackParamS;

    friend class boost::serialization::access;

    Track() = default;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& fNofHits;
      ar& fParFirst;
      ar& fParLast;
      ar& fParPV;
    }

   public:
    int fNofHits{kfdefs::Undef<int>};  ///< Number of hits in track

    TrackParam_t fParFirst;  ///< Track parameters on the first station
    TrackParam_t fParLast;   ///< Track parameters on the last station
    TrackParam_t fParPV;     ///< Track parameters in the primary vertex
  };

}  // namespace cbm::algo::ca

#endif  // CA_CORE_CaTrack_h
