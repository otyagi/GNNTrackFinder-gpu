/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Igor Kulakov, Sergey Gorbunov, Andrey Lebedev, Sergei Zharko [committer] */

#pragma once  // include this header only once per compilation unit

// NOTE: No dependency from CaCore is allowed
//#include "CaSimd.h"
//#include "CaUtils.h"
#include "AlgoFairloggerCompat.h"
#include "KfDefs.h"
#include "KfUtils.h"

#include <boost/serialization/vector.hpp>

#include <iomanip>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

//TODO: rewrite
namespace cbm::algo::kf
{
  /// \class MaterialMap
  /// \brief A map of station thickness in units of radiation length (X0) to the specific point in XY plane
  class alignas(VcMemAlign) MaterialMap {
   public:
    /// \brief Default constructor
    MaterialMap() = default;

    /// \brief Constructor from parameters
    /// \param nBins  Number of rows or columns
    /// \param xyMax  Size of station in x and y dimensions [cm]
    /// \param zRef   Reference z-coordinate of the material layer [cm]
    /// \param zMin   Lower boundary z-coordinate for the material layer [cm]
    /// \param zMax   Upper boundary z-coordinate for the material layer [cm]
    MaterialMap(int nBins, float xyMax, float zRef, float zMin, float zMax);

    /// \brief Copy constructor
    MaterialMap(const MaterialMap& other) = default;

    /// \brief Copy assignment operator
    MaterialMap& operator=(const MaterialMap& other) = default;

    /// \brief Move constructor
    MaterialMap(MaterialMap&& other) noexcept;

    /// \brief Move assignment operator
    MaterialMap& operator=(MaterialMap&& other) noexcept;

    /// \brief Destructor
    ~MaterialMap() noexcept = default;

    /// \brief Adds material layer
    /// \param other  Other material layer
    /// \param zTarg  z-coordinate of the target
    void Add(const MaterialMap& other, float zTarg = defs::Undef<float>);

    /// \brief Gets bin index for (x,y). Returns -1 when outside of the map
    int GetBin(float x, float y) const;

    /// \brief Gets number of bins (rows or columns) of the material table
    int GetNbins() const { return fNbins; }

    /// \brief Gets radius of the material table [cm]
    float GetXYmax() const { return fXYmax; }

    /// \brief Gets reference Z of the material [cm]
    float GetZref() const { return fZref; }

    /// \brief Gets minimal Z of the collected material [cm]
    float GetZmin() const { return fZmin; }

    /// \brief Gets maximal Z of the collected material [cm]
    float GetZmax() const { return fZmax; }

    /// \brief Gets material thickness in units of radiational length X0
    /// \tparam  I  Type of the x and y (floating point)
    /// \param   x  X coordinate of the point [cm]
    /// \param   y  Y coordinate of the point [cm]
    template<typename I>
    I GetThicknessX0(const I& x, const I& y) const
    {
      if constexpr (std::is_same_v<I, fvec>) {
        fvec res;
        for (size_t i = 0; i < utils::simd::Size<I>(); ++i) {
          res[i] = GetThicknessX0(x[i], y[i]);
        }
        return res;
      }
      else {
        I xNew = (x < fXYmax && x >= -fXYmax) ? x : 0;
        I yNew = (y < fXYmax && y >= -fXYmax) ? y : 0;
        int i  = static_cast<int>((xNew + fXYmax) * fFactor);
        int j  = static_cast<int>((yNew + fXYmax) * fFactor);
        i      = (i < fNbins && i >= 0) ? i : fNbins / 2;
        j      = (j < fNbins && j >= 0) ? j : fNbins / 2;
        return fTable[i + j * fNbins];
      }
    }

    /// \brief Gets material thickness in units of radiational length X0
    /// \tparam  I      Type of material thickness
    /// \param   iGlob  Global bin number:  iGlob = iX + iY * fNbins
    template<typename I>
    I GetBinThicknessX0(int iGlob) const
    {
      if constexpr (std::is_same_v<I, fvec>) {
        fvec res;
        for (size_t i = 0; i < utils::simd::Size<I>(); ++i) {
          res[i] = GetBinThicknessX0<fscal>(iGlob);
        }
        return res;
      }
      else {
        return fTable[iGlob];
      }
    }

    /// \brief Gets material thickness in units of radiational length X0
    /// \tparam  I  Type of material thickness
    /// \param   iX Bin number along x axis
    /// \param   iY Bin number along y axis
    template<typename I>
    I GetBinThicknessX0(int iX, int iY) const
    {
      return GetBinThicknessX0<I>(iX + iY * fNbins);
    }

    /// \brief Gets material thickness table
    const std::vector<float>& GetTable() const { return fTable; }

    /// \brief Function to test the instance for NaN
    bool IsUndefined() const
    {
      return utils::IsUndefined(fNbins) || utils::IsUndefined(fXYmax * fFactor * fZref * fZmin * fZmax);
    }

    /// \brief Reduces number of bins by a given factor
    /// \param factor  Number of bins in a new bin
    void Rebin(int factor);

    /// \brief Sets value of material thickness in units of X0 for a given cell of the material table
    /// \param iBinX      Index of table column
    /// \param iBinY      Index of table row
    /// \param thickness  Thickness of the material in units of X0
    /// \note  Indices of rows and columns in the table runs from 0 to nBins-1 inclusively, where nBins is the number
    ///        both of rows and columns. One should be careful while reading and storing the table from ROOT-file,
    ///        because iBinX = 0 and iBinY = 0 in the TH1::SetBinContent method of usually defines the underflow bin.
    void SetRadThickBin(int iBinX, int iBinY, float thickness) { fTable[iBinX + fNbins * iBinY] = thickness; }

    /// \brief Swap method
    void Swap(MaterialMap& other) noexcept;

    /// \brief String representation of the object
    /// \param indentLevel  Indent level of the string output
    /// \param verbose      Verbosity level
    std::string ToString(int indentLevel = 0, int verbose = 1) const;

    /// \brief Comparison operator (material map ordering by fZref)
    friend bool operator<(const MaterialMap& lhs, const MaterialMap& rhs) { return lhs.fZref < rhs.fZref; }

   private:
    /// \brief Checks the object consistency
    /// \throw std::logic_error  If the object is in non-valid mode
    void CheckConsistency() const;


    int fNbins    = defs::Undef<int>;    ///< Number of rows (== N columns) in the material budget table
    float fXYmax  = defs::Undef<float>;  ///< Size of the station in x and y dimensions [cm]
    float fFactor = defs::Undef<float>;  ///< Util. var. for the conversion of point coordinates to row/column id
    float fZref   = defs::Undef<float>;  ///< Reference Z of the collected material [cm]
    float fZmin   = defs::Undef<float>;  ///< Minimal Z of the collected material [cm]
    float fZmax   = defs::Undef<float>;  ///< Minimal Z of the collected material [cm]
    std::vector<float> fTable{};         ///< Material budget table

    /// \brief Serialization function
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int)
    {
      ar& fNbins;
      ar& fXYmax;
      ar& fFactor;
      ar& fZref;
      ar& fZmin;
      ar& fZmax;
      ar& fTable;
    }
  };
}  // namespace cbm::algo::kf
