/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmQaCheckerCore.h
/// @brief  Core class of the QA checking framework (declaration)
/// @author S. Zharko <s.zharko@gsi.de>
/// @date   06.02.2023

#ifndef CbmQaCheckerCore_h
#define CbmQaCheckerCore_h 1

#include "CbmQaCheckerObjectDB.h"
#include "CbmQaCheckerTypedefs.h"
#include "Rtypes.h"

#include <memory>
#include <string>
#include <vector>

class TFile;

namespace cbm::qa::checker
{
  /// @brief Core class for CBM QA checker framework (declaration)
  ///
  /// Class CbmQaCheckerCore defines a core of the QA checker framework and provides a user interface for
  /// the comparison routine execution
  ///
  class Core {
   public:
    /// @brief Default ctor
    Core();

    /// @brief Destructor
    ~Core() = default;

    /// @brief Copy constructor
    Core(const Core&) = delete;

    /// @brief Move constructor
    Core(Core&&) = delete;

    /// @brief Copy assignment operator
    Core& operator=(const Core&) = delete;

    /// @brief Move assignment operator
    Core& operator=(Core&&) = delete;

    // ----- User interface
    /// @brief Adds a version of QA output for a comparison
    /// @param label  Label of the version
    /// @param path   Path to the QA output directory for this version
    void AddVersion(const char* label, const char* path);

    /// @brief Adds a dataset name
    /// @param datasetName  Name of dataset
    void AddDataset(const char* datasetName);

    /// @brief Runs checking routine
    /// @param opt  Option:
    ///             "B": suppress canvas creation
    ///             "F": forces canvas creation (even if there is no difference)
    ///             "R": draw ratio on canvas
    ///             "D": draw difference on canvas
    ///             "E": enables exact comparison
    ///             "S": enables statistical hypothesis test, where is possible
    ///             "U": enables ratio comparison
    /// @return 0:  All versions are identical
    /// @return 1:  Some checks for some histograms did not pass, but the histograms are consistent
    /// @return 2:  Some histograms are different
    int Process(Option_t* comparisonMethod = "E");

    /// @brief Registers root-file for storing output
    /// @param filename  Name of file
    void RegisterOutFile(const char* filename);

    /// @brief  Sets default version label
    /// @param  defaultLabel  Name of default label
    ///
    /// If the default version is not provided, the first version will be used as the default one.
    void SetDefaultVersion(const char* defaultLabel) { fpObjDB->SetDefaultLabel(defaultLabel); }

    /// @brief Sets checker configuration from YAML file
    /// @param configName  Name of YAML configuration file
    void SetFromYAML(const char* configName);

    /// @brief Sets root path to input files
    /// @param pathName  Relative or absolute root path to input the input directories
    void SetInputRootPath(const char* pathName) { fpObjDB->SetInputRootPath(pathName); }

    /// @brief Sets P-value threshold
    /// @param pVal  P-value threshold
    void SetPvalThreshold(double pVal) { fpObjDB->SetPvalThreshold(pVal); }

    /// @brief Sets ratio accepted range
    /// @param min  Lower boundary
    /// @param max  Upper boundary
    void SetRatioRange(double min, double max) { fpObjDB->SetRatioRange(min, max); }

   private:
    /// @brief Prepares output file (creates directory structure)
    void PrepareOutputFile();

    std::shared_ptr<ObjectDB> fpObjDB = nullptr;  ///< Database of names
  };
}  // namespace cbm::qa::checker

#endif  // CbmQaCheckerCore_h
