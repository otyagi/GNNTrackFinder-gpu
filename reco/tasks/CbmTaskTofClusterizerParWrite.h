/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Pierre-Alain Loizeau, Norbert Herrmann */

#ifndef CBMTASKTOFCLUSTERIZERPARWRITE_H
#define CBMTASKTOFCLUSTERIZERPARWRITE_H 1

// TOF Classes and includes
// Geometry
class CbmTofDetectorId;
class CbmTofDigiPar;
class CbmTofDigiBdfPar;
class CbmTofCell;

#include "tof/Clusterizer.h"

// ROOT Classes and includes
#include "Math/Rotation3D.h"
#include "Math/Vector3Dfwd.h"

// FAIR classes and includes
#include "FairTask.h"

// ROOT Classes and includes
class TClonesArray;

// C++ Classes and includes
#include <vector>

class CbmTaskTofClusterizerParWrite : public FairTask {

 public:
  /**
       ** @brief Constructor.
       **/
  CbmTaskTofClusterizerParWrite();

  /**
       ** @brief Constructor.
       **/
  CbmTaskTofClusterizerParWrite(const char* name, int32_t verbose = 1, bool writeDataInOut = true);
  /**
       ** @brief Destructor.
       **/
  virtual ~CbmTaskTofClusterizerParWrite();

  /**
       ** @brief Inherited from FairTask.
       **/
  virtual InitStatus Init();

  /**
       ** @brief Inherited from FairTask.
       **/
  virtual void SetParContainers();

  /**
       ** @brief Inherited from FairTask.
       **/
  virtual void Exec(Option_t* /*option*/){};

  /**
       ** @brief Inherited from FairTask.
       **/
  virtual void Finish(){};

  inline void SetCalMode(int32_t iMode) { fCalMode = iMode; }
  inline void PosYMaxScal(double val) { fPosYMaxScal = val; }
  inline void SetTotMax(double val) { fTotMax = val; }
  inline void SetTotMin(double val) { fTotMin = val; }
  inline void SetTotMean(double val) { fTotMean = val; }
  inline void SetMaxTimeDist(double val) { fMaxTimeDist = val; }
  inline void SetChannelDeadtime(double val) { fdChannelDeadtime = val; }
  inline void SetCalParFileName(TString CalParFileName) { fCalParFileName = CalParFileName; }

  inline double GetTotMean() { return fTotMean; }

  void SwapChannelSides(bool bSwap) { fbSwapChannelSides = bSwap; }
  void SetDeadStrips(int32_t iDet, uint32_t ival);

 protected:
 private:
  bool bAddBeamCounterSideDigi = true;
  const int32_t nbClWalkBinX   = 50;  // was 100 (11.10.2018)
  const int32_t nbClWalkBinY   = 41;  // choose odd number to have central bin symmetric around 0

  /**
       ** @brief Copy constructor.
       **/
  CbmTaskTofClusterizerParWrite(const CbmTaskTofClusterizerParWrite&);
  /**
       ** @brief Copy operator.
       **/
  CbmTaskTofClusterizerParWrite& operator=(const CbmTaskTofClusterizerParWrite&);

  // Functions common for all clusters approximations
  /**
       ** @brief Initialize other parameters not included in parameter classes.
       **/
  bool InitParameters();
  /**
       ** @brief Initialize other parameters not included in parameter classes.
       **/
  bool InitCalibParameter();

  /**
       ** @brief Create one algo object for each RPC
       **/
  bool InitAlgos();

  // ToF geometry variables
  CbmTofDetectorId* fTofId;
  CbmTofDigiPar* fDigiPar;
  CbmTofDigiBdfPar* fDigiBdfPar;

  //Calibration parameters
  std::vector<std::vector<std::vector<std::vector<double>>>> fvCPTOff;     //[nSMT][nRpc][nCh][nbSide]
  std::vector<std::vector<std::vector<std::vector<double>>>> fvCPTotGain;  //[nSMT][nRpc][nCh][nbSide]
  std::vector<std::vector<std::vector<std::vector<double>>>> fvCPTotOff;   //[nSMT][nRpc][nCh][nbSide]
  std::vector<std::vector<std::vector<std::vector<std::vector<double>>>>>
    fvCPWalk;                                               //[nSMT][nRpc][nCh][nbSide][nbWalkBins]
  std::vector<std::vector<std::vector<double>>> fvCPTOffY;  //[nSMT][nRpc][nBin]
  std::vector<std::vector<double>> fvCPTOffYBinWidth;       //[nSMT][nRpc]
  std::vector<std::vector<double>> fvCPTOffYRange;          //[nSMT][nRpc]

  std::vector<uint32_t> fvDeadStrips;  //[nbDet]

  // Calib
  int32_t fCalMode;

  double fPosYMaxScal;
  double fTotMax;
  double fTotMin;
  double fTotOff;
  double fTotMean;
  double fMaxTimeDist;
  double fdChannelDeadtime;

  TString fCalParFileName;  // name of the file name with Calibration Parameters

  double fdTOTMax;
  double fdTOTMin;
  double fdTTotMean;

  double fdMaxTimeDist;   // Isn't this just a local variable? Why make it global and preset?!?
  double fdMaxSpaceDist;  // Isn't this just a local variable? Why make it global and preset?!?
  double fdModifySigvel;

  bool fbSwapChannelSides;

  ClassDef(CbmTaskTofClusterizerParWrite, 1);
};

#endif  // CBMTASKTOFCLUSTERIZER_H
