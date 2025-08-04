/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   KfSetupBuilder.h
/// @brief  A base KF-Setup initialization class (header)
/// @since  28.03.2024
/// @author Sergei Zharko <s.zharko@gsi.de>

#pragma once  // include this header only once per compilation unit

#include "KfIMaterialMapFactory.h"
#include "KfSetup.h"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace cbm::algo::kf
{
  /// \struct GeoLayer
  /// \brief  A helper structure to store geometrical information of the layers
  template<class EDetID>
  struct GeoLayer {
    GeoLayer(EDetID detID, int iLoc, double zRef, double zMin, double zMax, double xMax, double yMax)
      : fZref(zRef)
      , fZmin(zMin)
      , fZmax(zMax)
      , fXmax(xMax)
      , fYmax(yMax)
      , fDetID(detID)
      , fLocID(iLoc)
    {
    }

    double fZref;   ///< Reference z-coordinate [cm]
    double fZmin;   ///< Lower z-coordinate boundary [cm]
    double fZmax;   ///< Upper z-coordinate boundary [cm]
    double fXmax;   ///< Half size of the layer in the x-direction [cm]
    double fYmax;   ///< Half size of the layer in the y-direction [cm]
    EDetID fDetID;  ///< Index of detector subsystem
    int fLocID;     ///< Local index of the detector module
    bool operator<(const GeoLayer& rhs) const { return fZref < rhs.fZref; }
  };

  /// \class  SetupBuilder
  /// \brief  Creates a valid initialized Setup instance
  ///
  /// Initialization scenarios:
  ///   (a)  Manual:
  ///         - adding geometry layers one by one
  ///         - setting target
  ///         - setting field function (function + field type)
  ///   (b)  From setup:
  ///         - adding source setup (material layers, field slices and target)
  ///         - adding field function (if field -- interpolated)
  ///
  class SetupBuilder {
   public:
    /// \brief Default constructor
    SetupBuilder() = default;

    /// \brief Copy constructor
    SetupBuilder(const SetupBuilder&) = delete;

    /// \brief Move constructor
    SetupBuilder(SetupBuilder&&) = delete;

    /// \brief Destructor
    virtual ~SetupBuilder() = default;

    /// \brief Copy assignment operator
    SetupBuilder& operator=(const SetupBuilder&) = delete;

    /// \brief Move assignment operator
    SetupBuilder& operator=(SetupBuilder&&) = delete;

    /// \brief Adds material layer
    /// \tparam EDetID   detector ID index type
    /// \param  geoLayer A geometrical layer
    template<class EDetID>
    void AddLayer(const GeoLayer<EDetID>& geoLayer);

    /// \brief Creates a setup instance
    /// \param fldMode  Field mode of the setup
    template<typename T>
    Setup<T> MakeSetup(EFieldMode fldMode);

    /// \brief Resets the instance
    void Reset();

    /// \brief Sets magnetic field function
    /// \param fieldFn    Magnetic field function
    /// \param fieldType  Magnetic field type
    void SetFieldFunction(const FieldFn_t& fieldFn, EFieldType fieldType)
    {
      fbReady = false;
      fFieldFactory.SetFieldFunction(fieldFn, fieldType);
      fFieldFactory.SetStep(kTargetFieldInitStep);
      fbIfFieldFunctionSet = true;
    }

    /// \brief  Initializes the setup builder from existing setup
    /// \tparam T  Underlying data type of the setup
    /// \param  inSetup
    template<typename T>
    void SetFromSetup(const Setup<T>& inSetup);

    /// \brief Sets material map creator
    /// \param pMaterialFactory  Pointer to the actual material map creator instance
    void SetMaterialMapFactory(const std::shared_ptr<IMaterialMapFactory>& pMaterialFactory)
    {
      fbReady              = false;
      fpMaterialMapFactory = pMaterialFactory;
    }

    /// \brief Sets the material budget cache file name
    /// \param filename  Material budget cache file name
    /// \param refHash   Reference hash of the geometry
    ///
    /// If provided, the instance will try to read the material budget maps from the file.
    /// If the file does not exist,or the geometry hash was changed since the last time (reference hash differs from
    /// the one read from the file), a warning will be produced, the material budget maps will be recreated on the fly
    /// and they will be stored again to the file (meaning a new cache file will be generated over the existing one).
    void SetMaterialCacheFile(const std::string& filename, size_t refHash)
    {
      fsMaterialCacheFile = filename;
      fGeoHash            = refHash;
    }

    /// \brief Sets target initialization properties
    /// \param x  Target x-coordinate [cm]
    /// \param y  Target y-coordinate [cm]
    /// \param z  Target z-coordinate [cm]
    /// \param dz Target half-thickness [cm]
    /// \param r  Target transverse size (XYmax)
    void SetTargetProperty(double x, double y, double z, double dz, double r);

    // ********************
    // ** Static methods **
    // ********************

    /// \brief  Stores a serialized setup to a file
    /// \param  setup     A reference to the Setup (NOTE: a double-Setup must be passed explicitly)
    /// \param  fileName  Output file name
    ///
    /// Only a Setup with T = double can be serialized, so if a particular setup instance has another floating-point
    /// type, it will be converted to the double-version. In the serialization only the interpolated magnetic field
    /// variant is stored, the original field version will be set to nullopt during the loading from the file.
    static void Store(const Setup<double>& setup, const std::string& fileName);

    /// \brief  Loads a serialized setup from a file
    /// \tparam T         Underlying floating-point type for the output setup object
    /// \param  fileName  Input file name
    /// \throw  std::runtime_error  If the fileName cannot be opened
    template<typename T>
    static Setup<T> Load(const std::string& fileName);

   private:
    /// \brief  Initializes, validates and caches the parameters
    /// \throw  std::runtime_error If pre-initialization was incomplete
    /// \note   Does not touch the field function and field type/mode
    void Init();

    /// \brief Prints initialization status message
    std::string InitStatusMsg() const;

    /// \brief Reads material from file
    /// \return  true   Material budget was read from file
    /// \return  false  Material budget could not read (file does not exist or has incorrect properties)
    bool LoadMaterial();

    /// \brief Stores material to file
    void StoreMaterial() const;

    // TODO: Define target material more precisely
    static constexpr double kTargetCenterOffset{0.05};                 // Offset from target center [cm]
    static constexpr double kTargetMaterialOffset{2.};                 // Offset of the target material [in dz]
    static constexpr double kTargetMaterialTransverseSizeMargin{1.3};  // Margin of target transverse size
    static constexpr double kTargetFieldInitStep{2.5};  // Step between nodal points to determine field near target [cm]
    static constexpr int kTargetMaterialMapNofBins{20};

    std::set<GeoLayer<int>> fGeoLayers{};                                ///< Set of geo layers
    std::string fsMaterialCacheFile{""};                                 ///< A cache file for the material
    std::vector<MaterialMap> fvMaterial{};                               ///< Material map container
    std::shared_ptr<IMaterialMapFactory> fpMaterialMapFactory{nullptr};  ///< Material map creator
    ModuleIndexMapFactory fModuleIndexFactory;                           ///< Module index factory
    FieldFactory fFieldFactory;                                          ///< Instance of field factory
    Target<double> fTarget;                                              ///< Target properties
    size_t fGeoHash{0};                                                  ///< A hash of the geometry
    int fMatMapNofBins{100};                                             ///< Number of bins in material maps

    bool fbIfTargetSet{false};         ///< Target initialized
    bool fbIfGeoLayersInit{false};     ///< Geo layers initialized
    bool fbIfFieldFunctionSet{false};  ///< Field function initialized
    bool fbIfSetFromSetup{false};
    bool fbReady{false};  ///< Instance is ready for setup generation
  };


  // ********************************
  // ** Template method definition **
  // ********************************


  // ---------------------------------------------------------------------------------------------------------------------
  //
  template<class EDetID>
  void SetupBuilder::AddLayer(const GeoLayer<EDetID>& geoLayer)
  {
    fbReady    = false;
    auto layer = fGeoLayers.emplace(static_cast<int>(geoLayer.fDetID), geoLayer.fLocID, geoLayer.fZref, geoLayer.fZmin,
                                    geoLayer.fZmax, geoLayer.fXmax, geoLayer.fYmax);
    if (!layer.second) {
      std::stringstream msg;
      msg << "SetupBuilder::AddLayer: attempt of adding a duplicating geometry layer: ";
      msg << "DetID = " << static_cast<int>(geoLayer.fDetID) << ", localID = " << geoLayer.fLocID
          << "fZref = " << geoLayer.fZref << ", fZmin = " << geoLayer.fZmin << ", fZmax = " << geoLayer.fZmax
          << ", fXmax = " << geoLayer.fXmax << ", fYmax = " << geoLayer.fYmax
          << ".\nThe next layers were already added:";
      for (const auto& el : fGeoLayers) {
        msg << "\n\t- DetID = " << static_cast<int>(el.fDetID) << ", localID = " << el.fLocID << "fZref = " << el.fZref
            << ", fZmin = " << el.fZmin << ", fZmax = " << el.fZmax << ", fXmax = " << el.fXmax
            << ", fYmax = " << el.fYmax;
      }
      throw std::logic_error(msg.str());
    }
  }

  // ---------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  void SetupBuilder::SetFromSetup(const Setup<T>& inSetup)
  {
    LOG(info) << inSetup.ToString(2);
    fbReady = false;
    // Reset geo layer information
    fGeoLayers.clear();
    fvMaterial.clear();
    fModuleIndexFactory.Reset();
    fFieldFactory.ResetSliceReferences();

    // Init target properties
    fTarget = Target<double>(inSetup.fTarget);

    // Init geometry layers
    for (int iLayer{0}; iLayer < inSetup.GetNofMaterialLayers(); ++iLayer) {
      const auto& material{inSetup.GetMaterial(iLayer)};
      fvMaterial.push_back(material);
      fFieldFactory.AddSliceReference(material.GetXYmax(), material.GetXYmax(), material.GetZref());
      const auto& [iDetExt, iLoc] = inSetup.GetIndexMap().template GlobalToLocal<int>(iLayer);
      fModuleIndexFactory.AddComponent(iDetExt, iLoc, material.GetZref());
    }

    fbIfGeoLayersInit = true;
    fbIfTargetSet     = true;
    fbIfSetFromSetup  = true;
  }

  // ---------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  Setup<T> SetupBuilder::MakeSetup(EFieldMode fldMode)
  {
    Setup<T> setup(fldMode);
    if (!fbReady) {
      this->Init();
    }
    setup.fTarget = this->fTarget;
    for (const auto& material : this->fvMaterial) {
      setup.fvMaterialLayers.push_back(material);
    }
    setup.fField          = fFieldFactory.MakeField<T>();
    setup.fModuleIndexMap = fModuleIndexFactory.MakeIndexMap();
    return setup;
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  Setup<T> SetupBuilder::Load(const std::string& fileName)
  {
    Setup<double> setup(EFieldMode::Intrpl);
    std::ifstream ifs(fileName, std::ios::binary);
    if (!ifs) {
      std::stringstream msg;
      msg << "kf::SetupBuilder::Load: intput setup file \"" << fileName << "\" was not found";
      throw std::runtime_error(msg.str());
    }
    try {
      boost::archive::binary_iarchive ia(ifs);
      ia >> setup;
    }
    catch (const std::exception& err) {
      std::stringstream msg;
      msg << "kf::SetupBuilder::Load: input setup file \"" << fileName
          << "\" has inconsistent format or was "
             "corrupted. The exception message: "
          << err.what();
      throw std::runtime_error(msg.str());
    }
    return Setup<T>(setup);
  }
}  // namespace cbm::algo::kf
