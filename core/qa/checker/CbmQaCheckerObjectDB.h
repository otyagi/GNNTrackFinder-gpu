/* Copyright (C) 2023-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmQaCheckerObjectDB.h
/// @brief  Database for processed objects in the QA checker framework (header)
/// @author S. Zharko <s.zharko@gsi.de>
/// @since  15.02.2023

#ifndef CbmQaCheckerObjectDB_h
#define CbmQaCheckerObjectDB_h 1

#include "CbmQaCheckerTypedefs.h"

#include <set>
#include <string>
#include <utility>
#include <vector>

class TString;
class TDirectory;

namespace cbm::qa::checker
{
  /// @brief A data base class for processed objects
  ///
  /// The data base contains information on datasets, file-object pairs and versions, which are compared.
  class ObjectDB {
   public:
    /// @brief Default constructor
    ObjectDB() = default;

    /// @brief Destructor
    ~ObjectDB() = default;

    /// @brief Copy constructor
    ObjectDB(const ObjectDB&) = delete;

    /// @brief Move constructor
    ObjectDB(ObjectDB&&) = delete;

    /// @brief Copy assignment operator
    ObjectDB& operator=(const ObjectDB&) = delete;

    /// @brief Move assignment operator
    ObjectDB& operator=(ObjectDB&&) = delete;

    /// @brief Adds version
    /// @param label      Label of version
    /// @param path       Path to output files made for this version
    void AddVersion(const char* label, const char* path);

    /// @brief Adds dataset
    /// @param dataset  Name of dataset
    void AddDataset(const char* dataset);

    /// @brief Clears content
    void Clear();

    /// @brief Gets name of dataset
    /// @param iDataset  Index of dataset
    /// @return  Name of dataset
    const std::string& GetDataset(int iDataset) const { return fvDatasets[iDataset]; }

    /// @brief Gets index of default version
    /// @return  Index of default version
    int GetDefaultID() const { return fDefVersionID; }

    /// @brief Gets label of file
    /// @param iFile  Index of file
    /// @return  Label of file
    const std::string& GetFileLabel(int iFile) const { return fvFileLabels[iFile]; }

    /// @brief Gets name of file from indexes of version, file and dataset
    /// @param iVersion  Index of version
    /// @param iFile     Index of file
    /// @param iDataset  Index of dataset
    /// @return  Name of input file
    std::string GetInputFileName(int iVersion, int iFile, int iDataset) const;

    /// @brief Gets object name by its local index and index of file
    /// @param iFile    Index of file
    /// @param iObject  Index of object for a given file
    /// @return  Name of object
    const std::string& GetObject(int iFile, int iObject) const { return fvObjects[iFile][iObject]; }

    /// @brief Gets number of datasets
    int GetNofDatasets() const { return fvDatasets.size(); }

    /// @brief Gets total number of objects
    int GetNofObjects() const { return fvObjects.size(); }

    /// @brief Gets number of objects in file
    /// @param iFile  Index of file
    int GetNofObjects(int iFile) const { return fvObjects[iFile].size(); }

    /// @brief Gets number of files
    int GetNofFiles() const { return fvFiles.size(); }

    /// @brief Gets number of versions
    int GetNofVersions() const { return fvVersionLabels.size(); }

    /// @brief Gets output path
    const std::string& GetOutputPath() const { return fsOutputPath; }

    /// @brief Gets p-value threshold
    double GetPvalThreshold() const { return fPvalThresh; }

    /// @brief Gets upper bound of the accepted ratio range
    double GetRatioRangeMax() const { return fRatioMax; }

    /// @brief Gets lower bound of the accepted ratio range
    double GetRatioRangeMin() const { return fRatioMin; }

    /// @brief Gets version label
    /// @param iVersion  Index of version
    const std::string& GetVersionLabel(int iVersion) const { return fvVersionLabels[iVersion]; }

    /// @brief Gets version path
    /// @param iVersion  Index of version
    const std::string& GetVersionPath(int iVersion) const { return fvVersionPaths[iVersion]; }

    /// @brief Initializes the database
    void Init();

    /// @brief Reads DB from YAML node
    /// @param config Root node of the YAML file
    void ReadFromYAML(const char* configName);

    /// @brief String representation of the content
    /// @param  verbose  Verbosity level:
    ///                    0: no output,
    ///                    1: minimal output (versions, datasets and file names),
    ///                    2: objects as well
    /// @return A string representation of the DB contents
    std::string ToString(int verbose = 1) const;

    /// @brief  Sets default version label
    /// @param  defaultLabel  Name of default label
    ///
    /// If the default version is not provided as well as the provided, the first version will be used as the
    /// default one.
    void SetDefaultLabel(const char* defaultLabel) { fsDefaultLabel = defaultLabel; }

    /// @brief Sets root path to input files
    /// @param pathName  Relative or absolute root path to input the input directories
    void SetInputRootPath(const char* pathName) { fsInputRootPath = pathName; }

    /// @brief Sets the output path
    /// @param path  Path to the output ROOT-file
    void SetOutputPath(const char* path) { fsOutputPath = path; }

    /// @brief Sets P-value threshold
    /// @param pVal  P-value threshold
    void SetPvalThreshold(double pVal);

    /// @brief Sets ratio accepted range
    /// @param min  Lower boundary
    /// @param max  Upper boundary
    void SetRatioRange(double min, double max);

   private:
    /// @brief Reads list of histograms from file
    /// @param iFile  Index of file
    /// @note  Accumulates all possible histograms for different datasets
    void ReadObjectList(int iFile);

    /// @brief Loops over ROOT-file and collects object paths
    /// @param[in]  pDir        Pointer on directory
    /// @param[in]  parentPath  Parent path within file
    /// @param[out] paths       A set of object paths
    static void CollectObjectPaths(TDirectory* pDir, const TString& parentPah, std::set<std::string>& paths);

    int fDefVersionID = -1;  ///< Index of default version

    std::string fsInputRootPath = "";  ///< Root path for input files
    std::string fsOutputPath    = "";  ///< Path to the output file
    std::string fsDefaultLabel  = "";  ///< Name of default version label

    std::vector<std::string> fvDatasets;              ///< Container of dataset names
    std::vector<std::string> fvFiles;                 ///< Container of file names
    std::vector<char> fvbProcessWholeFile;            ///< If the whole file should be processed
    std::vector<int> fvObjectFirstGlobIndex;          ///< First global index of object in a file
    std::vector<std::string> fvFileLabels;            ///< Container of file labels (used in output)
    std::vector<std::vector<std::string>> fvObjects;  ///< Container of object names vs file id
    std::vector<std::string> fvVersionLabels;         ///< Container of version labels
    std::vector<std::string> fvVersionPaths;          ///< Container of version paths

    double fPvalThresh{0.05};  ///< P-value threshold for histograms equality
    double fRatioMax{1.05};    ///< Upper boundary for ratio deviation
    double fRatioMin{0.95};    ///< Lower boundary for ratio deviation
  };
}  // namespace cbm::qa::checker

#endif  // CbmQaCheckerObjectDB_h
