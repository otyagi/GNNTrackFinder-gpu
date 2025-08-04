/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaIO.h
/// \brief  Module for ROOT objects IO interface (header)
/// \author S.Zharko <s.zharko@gsi.de>
/// \since  29.03.2023

#ifndef CbmQaIO_h
#define CbmQaIO_h 1

#include "CbmQaCanvas.h"
#include "CbmQaEff.h"
#include "CbmQaTable.h"
#include "Logger.h"
#include "TCanvas.h"
#include "TEfficiency.h"
#include "TH1.h"
#include "TH2.h"
#include "TH3.h"
#include "TObject.h"
#include "TParameter.h"
#include "TProfile.h"
#include "TProfile2D.h"
#include "TProfile3D.h"
#include "TROOT.h"
#include "TString.h"
#include "yaml/Property.h"
#include "yaml/Yaml.h"

#include <limits>
#include <optional>
#include <type_traits>
#include <vector>

#include <yaml-cpp/yaml.h>

class TFile;

/// \brief  ROOT object IO interface for QA
///
/// The class provides interface to write ROOT object into resulted files
class CbmQaIO {
 public:
  using ObjList_t = std::vector<std::pair<TObject*, TString>>;

  enum class EStoringMode
  {
    kSAMEDIR,  ///< Objects of different type will be stored to root directory
    kSUBDIR    ///< Objects of different type will be stored in different subdirectories like histograms/ canvases/
  };

  /// \brief Constructor
  /// \param prefixName    Name of the unique prefix
  /// \param pObjList      List of already created objects
  CbmQaIO(TString prefixName, std::shared_ptr<ObjList_t> pObjList = nullptr);

  /// \brief Destructor
  virtual ~CbmQaIO();

  /// \brief Copy constructor
  CbmQaIO(const CbmQaIO&) = delete;

  /// \brief Move constructor
  CbmQaIO(CbmQaIO&&) = delete;

  /// \brief Copy assignment operator
  CbmQaIO& operator=(const CbmQaIO&) = delete;

  /// \brief Move assignment operator
  CbmQaIO& operator=(CbmQaIO&&) = delete;


  /// \brief Gets config name
  const char* GetConfigName() const { return fsConfigName.Data(); }

  /// \brief Creates a QA (ROOT) object, using properties defined with a tag in config
  /// \tparam T       Type of the object
  /// \param sName    Name of the object
  /// \param sTitle   Title of the object
  /// \param args...  The rest of the arguments, which will be passed to the histogram constructor
  /// \note Tag is defined after the last ';' symbol in the nameBase string
  /// \example
  ///    nambeBase = "/stations/station0/xy_occupancy;xy_station0" will be decayed into:
  ///    1) subdirectory "stations/station0"
  ///    2) name of histogram "catrd_hit_xy_occupancy"
  ///    3) tag for configuration file "xy_station0"
  /// If configuration file or tag are not defined, default parameters will be used
  template<typename T, typename... Args>
  T* MakeQaObject(TString sName, TString sTitle, Args... args);

  /// \brief Creates a directory in the output file.
  ///        Only needed to place directories in a preffered order when doing Write()
  ///        Call it prior to MakeQaObject() calls.
  /// \param dirName    Name of the directory
  /// \note Tag is defined after the last ';' symbol in the nameBase string
  /// \example
  ///    dirName = "/stations/station0"
  void MakeQaDirectory(TString sName);

  /// \brief Creates a ROOT object
  /// \brief Sets config name
  /// \param name  A path to the config
  void SetConfigName(const char* path);

  /// \brief Sets a common root path to the objects in the output file
  /// \param path  A path to the object
  void SetRootFolderName(const TString& path) { fsRootFolderName = path.Strip(TString::kBoth, '/'); }

  /// \brief Set storing mode
  void SetStoringMode(EStoringMode mode) { fStoringMode = mode; }

 protected:
  /// \brief Function to check, if a property is defined
  /// \param property  A property to be tested
  /// \param name      A name of property (used for logging)
  /// \note  Throws an exception, if property is undefined
  template<typename T>
  void CheckProperty(T&& property, const char* name) const;

  /// \brief Reads the specific configuration structure from the YAML node
  /// \tparam Config  Type of the configuration class
  ///
  /// The function is to be called in the user-defined class method InitDataBranches
  template<class Config>
  std::optional<Config> ReadSpecificConfig() const
  {
    std::optional<Config> res = std::nullopt;
    const auto& node          = fConfigNode["specific"];
    if (node) {
      res = std::make_optional(cbm::algo::yaml::Read<Config>(node));
    }
    return res;
  }

  /// \brief Applies properties on the histogram created with the MakeQaObject function
  /// \param pHist  Pointer to the histogram
  virtual void SetTH1Properties(TH1* pHist) const;

  /// \brief Applies properties on the histogram created with the MakeQaObject function
  /// \param pHist  Pointer to the histogram
  virtual void SetTH2Properties(TH2* pHist) const;

  /// \brief Applies properties on the profile 2D  created with the MakeQaObject function
  /// \param pHist  Pointer to the profile
  virtual void SetTProfile2DProperties(TProfile2D* pHist) const;

  /// \brief Applies properties on the canvas created with the MakeQaObject funciton
  /// \param pCanv  Pointer to the canvas
  virtual void SetCanvasProperties(TCanvas* pCanv) const;

  /// \brief Writes objects into file
  /// \param pOutFile Pointer to output ROOT file
  void WriteToFile(TFile* pOutFile) const;

  TString fsRootFolderName = "";  ///< Name of root folder
  TString fsConfigName     = "";  ///< Name of configuration file
  TString fsPrefix         = "";  ///< Unique prefix for all writeable root

  EStoringMode fStoringMode             = EStoringMode::kSUBDIR;  ///< Objects storing mode
  std::shared_ptr<ObjList_t> fpvObjList = nullptr;                ///< List of registered ROOT objects

  YAML::Node fConfigNode{};  ///< Configuration node

 private:
  /// \brief Creates and registers a ROOT object
  /// \param name  A name of the ROOT object, which can contain a sub-directory
  /// \param args  Other arguments, passed to the ROOT object constructor
  template<typename T, typename... Args>
  T* ConstructAndRegisterQaObject(TString name, Args... args);
};


// ---------------------------------------------------------------------------------------------------------------------
//
template<typename T>
void CbmQaIO::CheckProperty(T&& property, const char* name) const
{
  bool bPropertyUndefined = false;
  if constexpr (std::is_signed_v<T>) {
    bPropertyUndefined = property < 0;
  }
  else if constexpr (std::is_floating_point_v<T>) {
    bPropertyUndefined = std::isnan(property);
  }

  if (bPropertyUndefined) {
    std::stringstream msg;
    msg << "Property " << name << " is undefined in the configuration file";
    throw std::runtime_error(msg.str());
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename T, typename... Args>
T* CbmQaIO::ConstructAndRegisterQaObject(TString sName, Args... args)
{
  if constexpr (std::is_base_of_v<CbmQaTable, T>) {
    static_assert(sizeof...(Args) == 3);
  }

  // Resolve directory name and object name
  auto iLastSlashPos = static_cast<int>(sName.Last('/'));
  TString sDirectory = TString(sName(0, iLastSlashPos)).Strip(TString::kBoth, '/');
  sName              = sName(iLastSlashPos + 1, sName.Length() - iLastSlashPos);

  // Create a new object
  T* pObj = new T(sName.Data(), args...);

  // Take object ownership from ROOT
  if constexpr (std::is_base_of_v<TH1, T>) {  // all kind of histograms
    pObj->SetDirectory(nullptr);
  }

  // apply user-defined properties
  if constexpr (std::is_same_v<TH1F, T> || std::is_same_v<TH1D, T> || std::is_same_v<TProfile, T>) {  // histograms
    SetTH1Properties(pObj);
  }
  else if constexpr (std::is_same_v<TH2F, T> || std::is_same_v<TH2D, T>) {  // histograms
    SetTH2Properties(pObj);
  }
  else if constexpr (std::is_same_v<TProfile2D, T>) {  // profile 2D
    SetTProfile2DProperties(pObj);
  }
  else if constexpr (std::is_base_of_v<TCanvas, T>) {  // canvases
    auto* pListOfCanvases = gROOT->GetListOfCanvases();
    if (-1 != pListOfCanvases->IndexOf(pObj)) {
      pListOfCanvases->Remove(pObj);
    }
    SetCanvasProperties(pObj);
  }

  // Define a "summary" subdirectory of canvases and tables
  if constexpr (std::is_base_of_v<CbmQaTable, T> || std::is_base_of_v<TCanvas, T>) {
    if (EStoringMode::kSUBDIR == fStoringMode) {
      sDirectory = TString("Summary/") + sDirectory;
    }
  }

  // Add parent directory
  if (fsRootFolderName.Length() != 0) {
    if (sDirectory.Length() != 0) {
      sDirectory = fsRootFolderName + "/" + sDirectory;
    }
    else {
      sDirectory = fsRootFolderName;
    }
  }

  // Register the object in the list
  fpvObjList->push_back(std::make_pair(static_cast<TObject*>(pObj), sDirectory));

  return pObj;
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename T, typename... Args>
T* CbmQaIO::MakeQaObject(TString sName, TString sTitle, Args... args)
{
  if constexpr (std::is_base_of_v<CbmQaTable, T>) {
    static_assert(sizeof...(Args) == 2);
  }

  // Resolve configuration tag, if any
  auto iLastSepPos = static_cast<int>(sName.Last(';'));
  TString sTagName = sName(iLastSepPos + 1, sName.Length() - iLastSepPos);
  if (iLastSepPos != -1) {
    sName = sName(0, iLastSepPos);
  }

  bool bUseConfig = false;
  // Check, if parameters are provided for this tag
  if (fConfigNode) {
    if constexpr (std::is_base_of_v<TH1, T>) {
      if (fConfigNode["histograms"]) {
        if (fConfigNode["histograms"][sTagName.Data()]) {
          bUseConfig = true;
        }
      }
    }
    else if constexpr (std::is_base_of_v<TCanvas, T>) {
      if (fConfigNode["canvases"]) {
        if (fConfigNode["canvases"][sTagName.Data()]) {
          bUseConfig = true;
        }
      }
    }
  }

  //LOG(info) << "CbmQaIO: init ROOT object \"" << sName << '\"';  // Debug

  const char* title = sTitle.Data();
  if (bUseConfig) {
    if constexpr (std::is_base_of_v<TH1, T>) {
      const auto& tagNode = fConfigNode["histograms"][sTagName.Data()];

      [[maybe_unused]] int nBinsX  = -1;
      [[maybe_unused]] double minX = std::numeric_limits<double>::signaling_NaN();
      [[maybe_unused]] double maxX = std::numeric_limits<double>::signaling_NaN();
      [[maybe_unused]] int nBinsY  = -1;
      [[maybe_unused]] double minY = std::numeric_limits<double>::signaling_NaN();
      [[maybe_unused]] double maxY = std::numeric_limits<double>::signaling_NaN();
      [[maybe_unused]] int nBinsZ  = -1;
      [[maybe_unused]] double minZ = std::numeric_limits<double>::signaling_NaN();
      [[maybe_unused]] double maxZ = std::numeric_limits<double>::signaling_NaN();

      if (const auto& axisNode = tagNode["x"]) {
        nBinsX = axisNode["nbins"].as<int>();
        minX   = axisNode["min"].as<double>();
        maxX   = axisNode["max"].as<double>();
      }
      if (const auto& axisNode = tagNode["y"]) {
        nBinsY = axisNode["nbins"].as<int>();
        minY   = axisNode["min"].as<double>();
        maxY   = axisNode["max"].as<double>();
      }
      if (const auto& axisNode = tagNode["z"]) {
        nBinsZ = axisNode["nbins"].as<int>();
        minZ   = axisNode["min"].as<double>();
        maxZ   = axisNode["max"].as<double>();
      }

      if constexpr (std::is_base_of_v<CbmQaTable, T>) {
        return ConstructAndRegisterQaObject<T>(sName, title, args...);
      }
      else if constexpr (std::is_base_of_v<TProfile2D, T>) {
        CheckProperty(nBinsX, Form("qa/histograms/%s/x/nbins", sTagName.Data()));
        CheckProperty(minX, Form("qa/histograms/%s/x/min", sTagName.Data()));
        CheckProperty(maxX, Form("qa/histograms/%s/x/max", sTagName.Data()));
        CheckProperty(nBinsY, Form("qa/histograms/%s/y/nbins", sTagName.Data()));
        CheckProperty(minY, Form("qa/histograms/%s/y/min", sTagName.Data()));
        CheckProperty(maxY, Form("qa/histograms/%s/y/max", sTagName.Data()));
        CheckProperty(minZ, Form("qa/histograms/%s/z/min", sTagName.Data()));
        CheckProperty(maxZ, Form("qa/histograms/%s/z/max", sTagName.Data()));
        return ConstructAndRegisterQaObject<T>(sName, title, nBinsX, minX, maxX, nBinsY, minY, maxY, minZ, maxZ);
      }
      else if constexpr (std::is_base_of_v<TProfile, T>) {
        CheckProperty(nBinsX, Form("qa/histograms/%s/x/nbins", sTagName.Data()));
        CheckProperty(minX, Form("qa/histograms/%s/x/min", sTagName.Data()));
        CheckProperty(maxX, Form("qa/histograms/%s/x/max", sTagName.Data()));
        CheckProperty(minY, Form("qa/histograms/%s/y/min", sTagName.Data()));
        CheckProperty(maxY, Form("qa/histograms/%s/y/max", sTagName.Data()));
        return ConstructAndRegisterQaObject<T>(sName, title, nBinsX, minX, maxX, minY, maxY);
      }
      else if constexpr (std::is_base_of_v<TH3, T>) {
        CheckProperty(nBinsX, Form("qa/histograms/%s/x/nbins", sTagName.Data()));
        CheckProperty(minX, Form("qa/histograms/%s/x/min", sTagName.Data()));
        CheckProperty(maxX, Form("qa/histograms/%s/x/max", sTagName.Data()));
        CheckProperty(nBinsY, Form("qa/histograms/%s/y/nbins", sTagName.Data()));
        CheckProperty(minY, Form("qa/histograms/%s/y/min", sTagName.Data()));
        CheckProperty(maxY, Form("qa/histograms/%s/y/max", sTagName.Data()));
        CheckProperty(nBinsZ, Form("qa/histograms/%s/z/nbins", sTagName.Data()));
        CheckProperty(minZ, Form("qa/histograms/%s/z/min", sTagName.Data()));
        CheckProperty(maxZ, Form("qa/histograms/%s/z/max", sTagName.Data()));
        return ConstructAndRegisterQaObject<T>(sName, title, nBinsX, minX, maxX, nBinsY, minY, maxY, nBinsZ, minZ,
                                               maxZ);
      }
      else if constexpr (std::is_base_of_v<TH2, T>) {
        CheckProperty(nBinsX, Form("qa/histograms/%s/x/nbins", sTagName.Data()));
        CheckProperty(minX, Form("qa/histograms/%s/x/min", sTagName.Data()));
        CheckProperty(maxX, Form("qa/histograms/%s/x/max", sTagName.Data()));
        CheckProperty(nBinsY, Form("qa/histograms/%s/y/nbins", sTagName.Data()));
        CheckProperty(minY, Form("qa/histograms/%s/y/min", sTagName.Data()));
        CheckProperty(maxY, Form("qa/histograms/%s/y/max", sTagName.Data()));
        return ConstructAndRegisterQaObject<T>(sName, title, nBinsX, minX, maxX, nBinsY, minY, maxY);
      }
      else if constexpr (std::is_base_of_v<TH1, T>) {
        CheckProperty(nBinsX, Form("qa/histograms/%s/x/nbins", sTagName.Data()));
        CheckProperty(minX, Form("qa/histograms/%s/x/min", sTagName.Data()));
        CheckProperty(maxX, Form("qa/histograms/%s/x/max", sTagName.Data()));
        return ConstructAndRegisterQaObject<T>(sName, title, nBinsX, minX, maxX);
      }
    }
  }

  return ConstructAndRegisterQaObject<T>(sName, title, args...);
}

#endif  // CbmQaIO_h
