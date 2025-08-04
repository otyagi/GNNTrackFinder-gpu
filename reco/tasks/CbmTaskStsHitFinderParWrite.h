/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#pragma once

#include <FairTask.h>

class CbmStsElement;
class CbmStsParAsic;
class CbmStsParModule;
class CbmStsParSensor;
class CbmStsParSensorCond;
class CbmStsParSetModule;
class CbmStsParSetSensor;
class CbmStsParSetSensorCond;
class CbmStsParSim;
class CbmStsSensor;
class CbmStsSetup;

class CbmTaskStsHitFinderParWrite : public FairTask {

 public:
  /** @brief Constructor **/
  CbmTaskStsHitFinderParWrite();

  /** @brief Copy constructor (disabled) **/
  CbmTaskStsHitFinderParWrite(const CbmTaskStsHitFinderParWrite&) = delete;

  /** @brief Assignment operator (disabled) **/
  CbmTaskStsHitFinderParWrite operator=(const CbmTaskStsHitFinderParWrite&) = delete;

  /** @brief Destructor  **/
  virtual ~CbmTaskStsHitFinderParWrite();

  /** @brief Initialisation **/
  virtual InitStatus Init();

  /** @brief Task execution **/
  virtual void Exec(Option_t*) {}

  /** @brief End-of-run action **/
  virtual void Finish() {}

  /** @brief Define the needed parameter containers **/
  virtual void SetParContainers();


  /** @brief User-defined module parameters
     ** @param parModule Module parameter object
     **
     ** If defined, these parameters will be used for all modules instead
     ** of those found in the runtimeDb.
     */
  void UseModulePar(CbmStsParModule* modulePar) { fUserParModule = modulePar; }


  /** @brief User-defined module parameter set
     ** @param parModule Module parameter set object
     **
     ** If defined, this parameter set will be used instead of that found in the runtimeDb.
     */
  void UseModuleParSet(CbmStsParSetModule* moduleParSet) { fUserParSetModule = moduleParSet; }


  /** @brief User-defined sensor condition parameters
      ** @param parModule Sensor condition parameter object
      **
      ** If defined, these condition parameters will be used for all sensors instead
      ** of those found in the runtimeDb.
      */
  void UseSensorCond(CbmStsParSensorCond* sensorCond) { fUserParCond = sensorCond; }

  /** @brief User-defined module parameter set
     ** @param parModule Module parameter set object
     **
     ** If defined, this parameter set will be used instead of that found in the runtimeDb.
     */
  void UseSensorCondSet(CbmStsParSetSensorCond* sensorCondSet) { fUserParSetCond = sensorCondSet; }


  /** @brief User-defined sensor parameters
     ** @param parModule Sensor parameter object
     **
     ** If defined, these parameters will be used for all sensors instead
     ** of those found in the runtimeDb.
     */
  void UseSensorPar(CbmStsParSensor* sensorPar) { fUserParSensor = sensorPar; }


  /** @brief User-defined module parameter set
     ** @param parModule Module parameter set object
     **
     ** If defined, this parameter set will be used instead of that found in the runtimeDb.
     */
  void UseSensorParSet(CbmStsParSetSensor* sensorParSet) { fUserParSetSensor = sensorParSet; }

 private:
  /** @brief Average Lorentz Shift in a sensor
     ** @param conditions  Sensor operating conditions
     ** @param dZ  Sensor thickness [cm]
     ** @param bY  y component of magnetic field in sensor centre
     ** @return Mean Lorentz shift front side and back side [cm]
     **
     ** The Lorentz shift will be corrected for in hit finding.
     **/
  std::pair<Double_t, Double_t> LorentzShift(const CbmStsParSensorCond& conditions, Double_t dZ, Double_t bY);


  /** @brief Instantiate reconstruction modules
     ** @value Number of modules created
     **/
  UInt_t CreateModules();


  /** @brief Get the sensor parameters
     ** @param geoSensor Pointer to setup sensor
     **/
  void GetSensorParameters(CbmStsElement* geoSensor);


  /** @brief Initialise parameters
   **
   ** For simulated data, the parameters for modules and sensors are retrieved from the
   ** runtimeDb. They can be overridden by user-specified parameter sets using the respective
   ** setters. This is necessary when processing experiment data without a prior simulation step.
   **
   **/
  void InitParams();

 private:
  // --- Setup and parameters
  CbmStsSetup* fSetup                 = nullptr;  //! Instance of STS setup
  CbmStsParSim* fParSim               = nullptr;  ///< Simulation settings
  CbmStsParSetModule* fParSetModule   = nullptr;  ///< Module parameters
  CbmStsParSetSensor* fParSetSensor   = nullptr;  ///< Sensor parameters
  CbmStsParSetSensorCond* fParSetCond = nullptr;  ///< Sensor conditions

  // --- User-defined parameters, not from database
  CbmStsParAsic* fUserParAsic       = nullptr;
  CbmStsParModule* fUserParModule   = nullptr;
  CbmStsParSensor* fUserParSensor   = nullptr;
  CbmStsParSensorCond* fUserParCond = nullptr;

  // --- User-defined parameter sets, not from database
  CbmStsParSetModule* fUserParSetModule   = nullptr;
  CbmStsParSetSensor* fUserParSetSensor   = nullptr;
  CbmStsParSetSensorCond* fUserParSetCond = nullptr;
};
