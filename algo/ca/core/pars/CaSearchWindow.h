/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// \file   L1SearchWindow.h
/// \brief  Provides parameterisation for hits searching window in the CA tracking (header)
/// \date   08.11.2022
/// \author S.Zharko <s.zharko@gsi.de>

#pragma once  // include this header only once per compilation unit

#include <boost/serialization/array.hpp>
#include <boost/serialization/string.hpp>

#include <array>
#include <string>


/// TODO: SZh 8.11.2022: add selection of parameterisation

namespace cbm::algo::ca
{
  /// \class cbm::algo::ca::SearchWindow
  /// \brief Class L1SearchWindow defines a parameterisation of hits search window for CA tracking algorithm
  /// TODO: SZh 8.11.2022: add description
  class SearchWindow {
   public:
    /// \brief Constructor
    /// \param stationID  Global index of active station
    /// \param trackGrID  Track group ID
    SearchWindow(int stationID, int trackGrID);

    /// \brief Default constructor
    SearchWindow() = default;

    /// \brief Destructor
    ~SearchWindow() = default;

    /// \brief Copy constructor
    SearchWindow(const SearchWindow& other) = default;

    /// \brief Copy assignment operator
    SearchWindow& operator=(const SearchWindow& other) = default;

    /// \brief Move constructor
    SearchWindow(SearchWindow&& other) noexcept = default;

    /// \brief Move assignment operator
    SearchWindow& operator=(SearchWindow&& other) = default;

    /// \brief Parameterisation function for dx_max(x0)
    float DxMaxVsX0(float /*x*/) const { return fvParams[kDxMaxVsX0 /*+ 0*/]; }

    /// \brief Parameterisation function for dx_min(x0)
    float DxMinVsX0(float /*x*/) const { return fvParams[kDxMinVsX0 /*+ 0*/]; }

    /// \brief Parameterisation function for dx_max(y0)
    float DxMaxVsY0(float /*x*/) const { return fvParams[kDxMaxVsY0 /*+ 0*/]; }

    /// \brief Parameterisation function for dx_min(y0)
    float DxMinVsY0(float /*x*/) const { return fvParams[kDxMinVsY0 /*+ 0*/]; }

    /// \brief Parameterisation function for dy_max(x0)
    float DyMaxVsX0(float /*x*/) const { return fvParams[kDyMaxVsX0 /*+ 0*/]; }

    /// \brief Parameterisation function for dy_min(x0)
    float DyMinVsX0(float /*x*/) const { return fvParams[kDyMinVsX0 /*+ 0*/]; }

    /// \brief Parameterisation function for dy_max(y0)
    float DyMaxVsY0(float /*y*/) const { return fvParams[kDyMaxVsY0 /*+ 0*/]; }

    /// \brief Parameterisation function for dy_min(y0)
    float DyMinVsY0(float /*y*/) const { return fvParams[kDyMinVsY0 /*+ 0*/]; }


    /// \brief Gets station id
    int GetStationID() const { return fStationID; }

    /// \brief Gets track group id
    int GetTrackGroupID() const { return fTrackGroupID; }

    /// \brief Sets tag
    ///
    /// A tag can be used for insurance, if this search window corresponds to a desired track finder iteration
    void SetTag(const char* name) { fsTag = name; }

    // TODO: SZh 08.11.2022: Implement variadic  template function
    // TODO: SZh 08.11.2022: Try to reduce number of functions
    /// \brief Sets parameters for dx_max(x0)
    /// \param id  Parameter index
    /// \param val Parameter value
    void SetParamDxMaxVsX0(int id, float val);

    /// \brief Sets parameters for dx_min(x0)
    /// \param id  Parameter index
    /// \param val Parameter value
    void SetParamDxMinVsX0(int id, float val);

    /// \brief Sets parameters for dx_max(y0)
    /// \param id  Parameter index
    /// \param val Parameter value
    void SetParamDxMaxVsY0(int id, float val);

    /// \brief Sets parameters for dx_min(y0)
    /// \param id  Parameter index
    /// \param val Parameter value
    void SetParamDxMinVsY0(int id, float val);

    /// \brief Sets parameters for dy_max(x0)
    /// \param id  Parameter index
    /// \param val Parameter value
    void SetParamDyMaxVsX0(int id, float val);

    /// \brief Sets parameters for dy_min(x0)
    /// \param id  Parameter index
    /// \param val Parameter value
    void SetParamDyMinVsX0(int id, float val);

    /// \brief Sets parameters for dy_max(y0)
    /// \param id  Parameter index
    /// \param val Parameter value
    void SetParamDyMaxVsY0(int id, float val);

    /// \brief Sets parameters for dy_min(y0)
    /// \param id  Parameter index
    /// \param val Parameter value
    void SetParamDyMinVsY0(int id, float val);

    /// \brief String representation of the contents
    std::string ToString() const;

   private:
    static constexpr unsigned char kNpars = 1;  ///< Max number of parameters for one dependency
    static constexpr unsigned char kNdeps = 8;  ///< Number of the dependencies

    /// \brief Enumeration for dependencies stored
    enum EDependency
    {
      kDxMaxVsX0,
      kDxMinVsX0,
      kDxMaxVsY0,
      kDxMinVsY0,
      kDyMaxVsX0,
      kDyMinVsX0,
      kDyMaxVsY0,
      kDyMinVsY0
    };

    /// \brief Search window parameter array containing parameters from
    /// - dx_max vs x0 - indexes [0          .. kNpars - 1]
    /// - dx_min vs x0 - indexes [kNpars     .. (2 * kNpars - 1)]
    /// - dx_max vs y0 - indexes [2 * kNpars .. (3 * kNpars - 1)]
    /// - dx_min vs y0 - indexes [3 * kNpars .. (4 * kNpars - 1)]
    /// - dy_max vs y0 - indexes [4 * kNpars .. (5 * kNpars - 1)]
    /// - dy_min vs y0 - indexes [5 * kNpars .. (6 * kNpars - 1)]
    /// - dy_max vs y0 - indexes [6 * kNpars .. (7 * kNpars - 1)]
    /// - dy_min vs y0 - indexes [7 * kNpars .. (8 * kNpars - 1)]
    std::array<float, kNdeps* kNpars> fvParams = {0};

    int fStationID    = -1;  ///< Global index of active tracking station
    int fTrackGroupID = -1;  ///< Index of tracks group
    std::string fsTag = "";  ///< Tag, containing information on the tracks group (TODO: can be omitted?)

    /// \brief Serialization function
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int)
    {
      ar& fvParams;
      ar& fStationID;
      ar& fTrackGroupID;
      ar& fsTag;
    }
  };
}  // namespace cbm::algo::ca
