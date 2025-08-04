/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmQaCheckerObjectHandler.h
/// @brief  Base handler class (declaration)
/// @author S. Zharko <s.zharko@gsi.de>
/// @since  08.02.2023

#ifndef CbmQaCheckerObjectHandler_h
#define CbmQaCheckerObjectHandler_h 1

#include "CbmQaCheckerObjectDB.h"
#include "CbmQaCheckerTypedefs.h"
#include "Rtypes.h"

#include <memory>
#include <string>

class TDirectory;
class TNamed;
class TObject;
class TCanvas;

namespace cbm::qa::checker
{
  /// @brief Base abstract class for object handler.
  ///
  /// The class provides interface for handling objects of the same type, obtained under different versions of the code
  /// base.
  class ObjectHandler {
   public:
    /// @brief Default constructor
    /// @param iObject  Index of object
    /// @param iFile    Index of file
    /// @param iDataset Index of dataset
    /// @param objType  Type of the handled objects
    ObjectHandler(int iObject, int iFile, int iDataset, const char* objType = "");

    /// @brief Destructor
    virtual ~ObjectHandler();

    /// @brief Adds vector of pointer to objects
    /// @param vpObj  Vector of pointers to TNamed objects
    void AddObjects(const std::vector<TNamed*>& vpObj);

    /// @brief Creates object comparison canvas
    /// @param opt  Canvas options
    virtual void CreateCanvases(Option_t*){};

    /// @brief Compares objects to default
    /// @param iVersion     Version index
    /// @return  Comparison inference
    virtual ECmpInference Compare(int iVersion) const = 0;

    /// @brief   Compares different versions with default
    /// @return  Vector of comparison inferences for different versions
    std::vector<ECmpInference> CompareWithDefault();

    /// @brief Sets folder to store output
    /// @param pDir  Pointer to folder instance
    void SetOutputDirectory(TDirectory* pDir);

    /// @brief Sets objects database
    /// @param  pObjDB  Shared pointer to object database
    void SetObjectDB(std::shared_ptr<ObjectDB>& pObjDB) { fpObjDB = pObjDB; }

    /// @brief Sets bit flag to control handler behaviour
    /// @param bit  Bit index
    ///
    /// The bit flags should be defined in an enumeration of the default class
    void SetComparisonMethod(ECmpMethod method) { fCmpBits.set(static_cast<uint8_t>(method)); }

    /// @brief Writes objects to file
    void Write();

   protected:
    std::string fsObjType{""};                   ///< Base type of the object to be handled
    std::string fsBaseName{""};                  ///< Base names of the object
    std::vector<TNamed*> fvpObjects;             ///< Vector of objects
    std::shared_ptr<ObjectDB> fpObjDB{nullptr};  ///< Pointer to object database
    std::shared_ptr<TCanvas> fpCanvas{nullptr};  ///< Comparison canvas
    TDirectory* fpOutDir{nullptr};               ///< Pointer to directory
    int fObjectID{-1};                           ///< Index of object
    int fFileID{-1};                             ///< Index of file
    int fDatasetID{-1};                          ///< Index of dataset

    std::bitset<static_cast<size_t>(ECmpMethod::END)> fCmpBits;  ///< Bitset for comparison methods
  };
}  // namespace cbm::qa::checker

#endif  // CbmQaCheckerObjectHandler_h
