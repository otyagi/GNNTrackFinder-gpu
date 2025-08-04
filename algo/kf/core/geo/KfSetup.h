/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   KfSetup.h
/// @brief  Setup representation for the Kalman-filter framework (header)
/// @since  28.03.2024
/// @author Sergei Zharko <s.zharko@gsi.de>

#pragma once  // include this header only once per compilation unit

#include "KfDefs.h"
#include "KfField.h"
#include "KfMaterialMap.h"
#include "KfModuleIndexMap.h"
#include "KfTarget.h"
#include "KfVector.h"

#include <boost/serialization/access.hpp>

#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace cbm::algo::kf
{
  /// \class  Setup
  /// \brief  KF-framework representation of the detector setup
  /// \tparam T  Underlying floating-point data type
  template<typename T>
  class alignas(VcMemAlign) Setup {
    friend class boost::serialization::access;  // Boost serializer methods
    friend class SetupBuilder;
    template<typename>
    friend class Setup;

   public:
    /// \brief Constructor
    /// \param fldMode  Field mode
    Setup(EFieldMode fldMode)
      : fvMaterialLayers({})
      , fField(Field<T>(fldMode, EFieldType::Normal))
      , fTarget(Target<T>())
    {
    }

    /// \brief Destructor
    ~Setup() = default;

    /// \brief  Copy constructor
    /// \tparam I  Underlying floating-point data type of the source
    template<typename I>
    Setup(const Setup<I>& other)
      : fModuleIndexMap(other.fModuleIndexMap)
      , fvMaterialLayers(other.fvMaterialLayers)
      , fField(other.fField)
      , fTarget(other.fTarget)
    {
    }

    /// \brief Move constructor
    //Setup(Setup&&) noexcept;

    /// \brief  Copy assignment operator
    Setup& operator=(const Setup& other) = default;

    /// \brief Move assignment operator
    //Setup& operator=(Setup&&) noexcept;

    /// \brief Disables geo layer
    /// \tparam  EDetID  Index of the detector subsystem
    /// \param iDet  DetectorID
    /// \param iLoc  Local index of the module
    template<class EDetID>
    void DisableLayer(EDetID iDet, int iLoc);

    /// \brief Makes an instance of the field depending on the template parameter
    /// \throw  std::logic_error  If the particular field member is undefined (nullopt)
    const Field<T>& GetField() const { return fField; }

    /// \brief Gets material layer
    /// \param iLayer  Index of layer
    const MaterialMap& GetMaterial(int iLayer) const { return fvMaterialLayers[iLayer]; }

    /// \brief Gets material layer from external indices
    /// \tparam  EDetID  Index of the detector subsystem
    /// \param iDet  DetectorID
    /// \param iLoc  Local index of the module
    template<class EDetID>
    const MaterialMap& GetMaterial(EDetID iDet, int iLoc) const
    {
      return fvMaterialLayers[fModuleIndexMap.LocalToGlobal(iDet, iLoc)];
    }

    /// \brief Gets module index map
    const ModuleIndexMap& GetIndexMap() const { return fModuleIndexMap; }

    /// \brief Gets number of geometry layers
    int GetNofLayers() const { return static_cast<int>(fvMaterialLayers.size()); }

    /// \brief Gets target
    const Target<T>& GetTarget() const { return fTarget; }

    /// \brief String representation of the class contents
    /// \param verbosity    A verbose level for output
    /// \param indentLevel  Indent level of the string output
    std::string ToString(int verbosity = 1, int indentLevel = 0) const;

   private:
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& fModuleIndexMap;
      ar& fvMaterialLayers;
      ar& fTarget;
      ar& fField;
    }

    ModuleIndexMap fModuleIndexMap{};             ///< Index map (internal->external)
    std::vector<MaterialMap> fvMaterialLayers{};  ///< Container of the material maps
    Field<T> fField;                              ///< Interpolated field (NOTE: maybe make optional)
    Target<T> fTarget;                            ///< Target layer
  };


  // -------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  template<class EDetID>
  void Setup<T>::DisableLayer(EDetID iDet, int iLoc)
  {
    int iLayer{fModuleIndexMap.LocalToGlobal(iDet, iLoc)};
    if (iLayer == -1) {
      return;
    }

    // Remove material layer and add it to the next one
    if (iLayer < static_cast<int>(fvMaterialLayers.size() - 1)) {
      fvMaterialLayers[iLayer + 1].Add(fvMaterialLayers[iLayer], utils::simd::Cast<T, float>(fTarget.GetZ()));
    }
    fvMaterialLayers.erase(fvMaterialLayers.begin() + iLayer);

    // Remove field slice
    fField.RemoveSlice(iLayer);

    // Disable layer in the index map
    fModuleIndexMap.Disable(iDet, iLoc);
    //std::cout << "LAYERS: \n" << fModuleIndexMap.ToString() << '\n';
  }


}  // namespace cbm::algo::kf
