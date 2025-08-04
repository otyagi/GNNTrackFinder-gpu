/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Pierre-Alain Loizeau, Norbert Herrmann */

#ifndef CBMTASKTOFCLUSTERIZER_H
#define CBMTASKTOFCLUSTERIZER_H 1

// TOF Classes and includes
// Input/Output
class CbmEvent;
class CbmDigiManager;
class CbmTsEventHeader;

#include "CbmMatch.h"
#include "CbmTofDigi.h"
#include "tof/Calibrate.h"
#include "tof/Clusterizer.h"
#include "tof/Hitfind.h"

// ROOT Classes and includes
#include "Math/Rotation3D.h"
#include "Math/Vector3Dfwd.h"

// FAIR classes and includes
#include "FairTask.h"

// ROOT Classes and includes
class TClonesArray;

// C++ Classes and includes
#include <vector>

class CbmTaskTofClusterizer : public FairTask {

 public:
  /**
       ** @brief Constructor.
       **/
  CbmTaskTofClusterizer();

  /**
       ** @brief Constructor.
       **/
  CbmTaskTofClusterizer(const char* name, int32_t verbose = 1, bool writeDataInOut = true);
  /**
       ** @brief Destructor.
       **/
  virtual ~CbmTaskTofClusterizer();

  /**
       ** @brief Inherited from FairTask.
       **/
  virtual InitStatus Init();

  /**
       ** @brief Inherited from FairTask.
       **/
  virtual void SetParContainers(){};

  /**
       ** @brief Inherited from FairTask.
       **/
  virtual void Exec(Option_t* option);
  virtual void ExecEvent(Option_t* option);

  /**
       ** @brief Inherited from FairTask.
       **/
  virtual void Finish();
  virtual void Finish(double calMode);

  ///////////// Empty functions for interface compatibility
  void SetCalMode(int32_t /*iMode*/) {}
  void SetDutId(int32_t /*Id*/) {}
  void PosYMaxScal(double /*val*/) {}
  void SetTotMax(double /*val*/) {}
  void SetTotMin(double /*val*/) {}
  void SetTotMean(double /*val*/) {}
  void SetMaxTimeDist(double /*val*/) {}
  void SetChannelDeadtime(double /*val*/) {}
  void SetCalParFileName(TString /*CalParFileName*/) {}
  double GetTotMean() { return 0.0; }
  ////////////
  int GetNbHits() { return fiNbHits; }

  void SwapChannelSides(bool bSwap) { fbSwapChannelSides = bSwap; }
  void SetFileIndex(int32_t iIndex) { fiFileIndex = iIndex; }
  void SetWriteDigisInOut(bool bDigis) { fbWriteDigisInOut = bDigis; }
  void SetWriteHitsInOut(bool bHits) { fbWriteHitsInOut = bHits; }
  void SetDeadStrips(int32_t iDet, uint32_t ival);

 protected:
 private:
  int32_t iNbTs                = 0;
  int fiHitStart               = 0;
  bool bAddBeamCounterSideDigi = true;

  std::vector<CbmTofDigi>* fBmonDigiVec = nullptr;  //! Bmon Digis

  /**
       ** @brief Copy constructor.
       **/
  CbmTaskTofClusterizer(const CbmTaskTofClusterizer&);
  /**
       ** @brief Copy operator.
       **/
  CbmTaskTofClusterizer& operator=(const CbmTaskTofClusterizer&);

  // Functions common for all clusters approximations
  /**
       ** @brief Recover pointer on input TClonesArray: TofPoints, TofDigis...
       **/
  bool RegisterInputs();
  /**
       ** @brief Create and register output TClonesArray of Tof Hits.
       **/
  bool RegisterOutputs();

  /**
       ** @brief Build clusters out of ToF Digis and store the resulting info in a TofHit.
       **/
  bool BuildClusters();

  /**
       ** @brief Create one algo object for each RPC
       **/
  bool InitAlgos();

  // Hit finder algo
  std::unique_ptr<cbm::algo::tof::Hitfind> fAlgo;

  // Calibrator algo
  std::unique_ptr<cbm::algo::tof::Calibrate> fCalibrate;

  const CbmTsEventHeader* fTsHeader;

  // Input variables
  std::vector<CbmTofDigi> fTofDigiVec{};  //! TOF Digis
  CbmDigiManager* fDigiMan;               // TOF Input Digis
  TClonesArray* fEventsColl;              // CBMEvents (time based)

  // Output variables
  bool fbWriteHitsInOut;
  bool fbWriteDigisInOut;
  std::vector<CbmTofDigi>* fTofCalDigiVec = nullptr;     //! // Calibrated TOF Digis
  TClonesArray* fTofHitsColl;                            // TOF hits
  TClonesArray* fTofDigiMatchColl;                       // TOF Digi Links
  std::vector<CbmTofDigi>* fTofCalDigiVecOut = nullptr;  //! // Calibrated TOF Digis
  TClonesArray* fTofHitsCollOut;                         // TOF hits
  TClonesArray* fTofDigiMatchCollOut;                    // TOF Digi Links
  int32_t fiNbHits;                                      // Index of the CbmTofHit TClonesArray

  double fdEvent;
  double fProcessTime = 0.0;
  uint64_t fuNbDigis  = 0;
  uint64_t fuNbHits   = 0;

  bool fbSwapChannelSides;
  int32_t fiOutputTreeEntry;
  int32_t fiFileIndex;

  ClassDef(CbmTaskTofClusterizer, 1);
};

#endif  // CBMTASKTOFCLUSTERIZER_H
