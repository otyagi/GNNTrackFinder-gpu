/* Copyright (C) 2007-2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer], Igor Kulakov, Sergei Zharko */

#pragma once  // include this header only once per compilation unit

#include "CaDefs.h"
#include "CaSimd.h"
#include "KfFieldRegion.h"

#include <string>

namespace cbm::algo::ca
{
  /// Structure Station
  /// Contains a set of geometry parameters for a particular station
  ///
  template<typename DataT>
  class Station {
   public:
    // TODO: SZh 12.05.2022: Rewrite type into L1DetectorID, change detector indexing scheme
    // TODO: SZh 12.05.2022: Provide getters to stations

    int type     = constants::Undef<int>;  //  TODO: replace with L1DetectorID
    int timeInfo = constants::Undef<int>;  ///< flag: if time information can be used
    int fieldStatus =
      constants::Undef<int>;               ///< flag: 1 - station is INSIDE the field, 0 - station is OUTSIDE the field
    int geoLayerID = constants::Undef<int>;    ///< Index of layer in geometrical setup
    DataT fZ   = constants::Undef<DataT>;  ///< z position of station     [cm]
    DataT Xmax = constants::Undef<DataT>;  ///< min radius of the station [cm]
    DataT Ymax = constants::Undef<DataT>;  ///< max radius of the station [cm]

    kf::FieldSlice<DataT> fieldSlice{};  ///< Magnetic field near the station

    Station() = default;

    /// \brief Copy constructor with type conversion
    template<typename DataIn>
    Station(const Station<DataIn>& other)
      : type(other.GetType())
      , timeInfo(other.GetTimeStatus())
      , fieldStatus(other.GetFieldStatus())
      , geoLayerID(other.GetGeoLayerID())
      , fZ(other.template GetZ<DataT>())
      , Xmax(other.template GetXmax<DataT>())
      , Ymax(other.template GetYmax<DataT>())
      , fieldSlice(other.fieldSlice)
    {
    }

    // Serialization block
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int)
    {
      ar& type;
      ar& timeInfo;
      ar& fieldStatus;
      ar& geoLayerID;

      ar& fZ;
      ar& Xmax;
      ar& Ymax;

      ar& fieldSlice;
    }

    /// \brief Verifies class invariant consistency
    /// \note  Object is considered undefined in the creation time, so this function should be called after the object
    ///        initialization
    void CheckConsistency() const;

    /// \brief Gets type of the station
    int GetType() const { return type; }

    /// \brief Gets time-measurement flag
    int GetTimeStatus() const { return timeInfo; }

    /// \brief Gets field status flag
    int GetFieldStatus() const { return fieldStatus; }

    /// \brief Gets index of the layer in the geometry setup (which can include inactive stations as well)
    int GetGeoLayerID() const { return geoLayerID; }

    /// \brief Gets z-position of the station
    template<typename DataOut = DataT>
    DataOut GetZ() const
    {
      return kfutils::simd::Cast<DataT, DataOut>(fZ);
    }

    /// \brief Gets limit of the station size in x-axis direction
    template<typename DataOut = DataT>
    DataOut GetXmax() const
    {
      return kfutils::simd::Cast<DataT, DataOut>(Xmax);
    }

    /// \brief Gets limit of the station size in x-axis direction
    template<typename DataOut = DataT>
    DataOut GetXmin() const
    {
      return kfutils::simd::Cast<DataT, DataOut>(-Xmax);
    }

    /// \brief Gets limit of the station size in y-axis direction
    template<typename DataOut = DataT>
    DataOut GetYmax() const
    {
      return kfutils::simd::Cast<DataT, DataOut>(Ymax);
    }

    /// \brief Gets limit of the station size in y-axis direction
    template<typename DataOut = DataT>
    DataOut GetYmin() const
    {
      return kfutils::simd::Cast<DataT, DataOut>(-Ymax);
    }

    /// \brief String representation of class contents
    /// \param verbosityLevel  Verbosity level of the output
    /// \param indentLevel     Number of indent characters in the output
    std::string ToString(int verbosityLevel = 0, int indentLevel = 0, bool isHeader = false) const;

  } _fvecalignment;

}  // namespace cbm::algo::ca
