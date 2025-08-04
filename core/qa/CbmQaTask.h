/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaTask.h
/// \brief  A base class for CBM QA task logic
/// \author S.Zharko <s.zharko@gsi.de>
/// \since  12.01.2023

#pragma once

#include "CbmEvent.h"
#include "CbmQaCompare.h"
#include "CbmQaIO.h"
#include "CbmQaTable.h"
#include "FairTask.h"
#include "Logger.h"
#include "TCanvas.h"
#include "TEfficiency.h"
#include "TFile.h"
#include "TFolder.h"
#include "TH1.h"
#include "TH2.h"
#include "TH3.h"
#include "TParameter.h"
#include "TProfile.h"
#include "TProfile2D.h"
#include "TProfile3D.h"
#include "TROOT.h"
#include "TString.h"

#include <algorithm>
#include <map>
#include <regex>
#include <string_view>
#include <tuple>
#include <type_traits>

#include <yaml-cpp/yaml.h>

class CbmEvent;
class TClonesArray;

/// Class CbmQaTask is to be inherited with a particular QA-task. It provides mechanisms for storage and management
/// of QA canvases and histograms management
class CbmQaTask : public FairTask, public CbmQaIO {
  /// \struct CheckFlags
  /// \brief Contains a check result and its activeness status of the check-list entry
  ///
  /// If the status is set to false, the check result is ignored in the QA verdict.
  struct CheckFlags {
    std::string fMsg = "";     ///< Supporting message for the check
    bool fResult     = false;  ///< Check result storage
    bool fStatus     = false;  ///< Status of the check
  };

  /// \struct ObjectComparisonConfig
  /// \brief  Contains configuration to compare two root objects (histograms)
  struct ObjectComparisonConfig {
    uint8_t fPoint       = 0;
    uint8_t fRatio       = 0;
    uint8_t fStat        = 0;
    double fRatioMin     = 0;
    double fRatioMax     = std::numeric_limits<double>::max();
    double fChi2NdfMax   = std::numeric_limits<double>::max();
    std::string fStatOpt = "UU";
    std::string fCanvOpt = "";
  };

 public:
  /// \brief  Constructor from parameters
  /// \param  name     Name of the task
  /// \param  verbose  Verbose level
  /// \param  isMCUsed Flag: true - MC information is used, false - only reconstructed data QA is processed
  /// \param  recoMode Reconstruction mode (see documentation for the CbmQaTask::SetRecoMode function)
  CbmQaTask(const char* name, int verbose, bool isMCUsed, ECbmRecoMode recoMode = ECbmRecoMode::Timeslice);

  /// \brief Default constructor
  CbmQaTask() = delete;  // TODO: Let's see, what can happen, if one deletes default constructor

  /// \brief Destructor
  virtual ~CbmQaTask() = default;

  /// \brief Copy constructor
  CbmQaTask(const CbmQaTask&) = delete;

  /// \brief Move constructor
  CbmQaTask(CbmQaTask&&) = delete;

  /// \brief Copy assignment operator
  CbmQaTask& operator=(const CbmQaTask&) = delete;

  /// \brief Move assignment operator
  CbmQaTask& operator=(CbmQaTask&&) = delete;

  /// \brief  Function to check, if the QA results are acceptable
  ///
  /// In the function one should provide the check flags with the function StoreCheckResult.
  virtual void Check() = 0;

  /// \brief Process ROOT objects comparison
  /// \return true  All the object are consistent
  /// \return false Some of the objects are inconsistent
  bool CompareQaObjects();

  /// \brief Disables event-by-event execution
  ///
  /// By default, if the branch
  void DisableEventMode();


  /// \brief Gets check-list
  const std::map<std::string, CheckFlags>& GetCheckList() const { return fmCheckList; }

  /// \brief Gets default tag
  const TString& GetDefaultTag() const { return fsDefaultTag; }

  /// \brief Gets name of the setup
  const std::string& GetSetupName() const { return fsSetupName; }

  /// \brief Gets version tag
  const TString& GetVersionTag() const { return fsVersionTag; }


  /// \brief Returns flag, whether MC information is used or not in the task
  bool IsMCUsed() const { return fbUseMC; }

  /// \brief Reads check-list from the configuration file
  void ReadCheckListFromConfig();


  /// \brief Sets check-file
  /// \param pCheckFile  Shared pointer to the cross-check file
  void SetCheckFile(const std::shared_ptr<TFile>& pCheckFile) { fpBenchmarkInput = pCheckFile; }

  /// \brief Sets compare output file
  /// \param pCompareOutput  Shared pointer to the comparison output file
  void SetCompareOutput(const std::shared_ptr<TFile>& pCompareOutput) { fpBenchmarkOutput = pCompareOutput; }

  /// \brief Sets version tag
  void SetVersionTag(const TString& tag) { fsVersionTag = tag; }

  /// \brief Sets default tag
  void SetDefaultTag(const TString& tag) { fsDefaultTag = tag; }

  /// \brief Sets data processing (reconstruction) mode
  ///
  /// The reconstruction mode can be either ECbmRecoMode::Timeslice or ECbmRecoMode::EventByEvent. The first
  /// variant implies, that the data are processed within entire time-slice (in the event-by-event simulation within
  /// the entire event). For the second variant the data are processed within a loop of CbmEvent objects, if the
  /// branch of latter is accessible. By default the ECbmRecoMode::Timeslice is used.
  void SetRecoMode(ECbmRecoMode recoMode) { fRecoMode = recoMode; }

  /// \brief Sets name of the setup
  void SetSetupName(const char* setup) { fsSetupName = setup; }

 protected:
  /// \brief De-initialize the task
  virtual void DeInit() {}

  /// \brief FairTask: Defines action of the task in the event/TS
  void Exec(Option_t* /*option*/) override final;

  /// \brief FairTask: Defines action of the task in the end of run
  void Finish() override final;

  /// \brief FairTask: Task initialization in the beginning of the run
  InitStatus Init() override final;

  /// \brief FairTask: Task reinitialization
  InitStatus ReInit() override final;

  /// \brief Initializes the task
  virtual InitStatus InitQa() { return kSUCCESS; }

  /// \brief Initializes QA-task summary: canvases, tables etc.
  virtual void CreateSummary() {}

  /// \brief Method to fill histograms per event or time-slice
  virtual void ExecQa() {}

  /// \brief Get current event number
  int GetEventNumber() const { return fNofEvents.GetVal(); }

  // ***********************
  // ** Utility functions **
  // ***********************

  /// \brief Checks range of variable
  /// \param name  Name of the variable
  /// \param var   Variable to check
  /// \param lo    Lower limit of the variable
  /// \param hi    Upper limit of the variable
  /// \return  False, if variable exits the range
  template<typename T>
  bool CheckRange(std::string_view name, T var, T lo, T hi) const;

  /// \brief Checks range of variable
  /// \param name    Name of the variable
  /// \param var     Variable to check
  /// \param varErr  Standard Error of the variable
  /// \param lo      Lower limit of the variable
  /// \param hi      Upper limit of the variable
  /// \return        False, if variable exits the range
  template<typename T>
  bool CheckRange(std::string_view name, T var, T varErr, T lo, T hi) const;

  /// \brief   Checks ranges for mean and standard deviation
  /// \return  False, if variable exits the range
  bool CheckRange(TH1* h, double meanMax, double rmsMin, double rmsMax);

  /// \brief Puts setup title on the canvas
  void PutSetupNameOnPad(double xMin, double yMin, double xMax, double yMax);

  /// \brief Gets pointer to current event
  CbmEvent* GetCurrentEvent() { return fpCurrentEvent; }

  /// \brief Stores check flag to the check-list
  /// \param tag    The flag name
  /// \param result Check result
  /// \param msg    Supporting message (optional)
  void StoreCheckResult(const std::string& tag, bool result, const std::string& msg = "");

 private:
  /// \brief De-initializes this task
  void DeInitBase();

  /// \brief Process object comparison
  /// \tparam Obj Class of the ROOT object
  /// \param  pObjL   First object (this)
  /// \param  pObjR   Second object (default)
  /// \param  objName Name of the object (contains full path to the object)
  /// \param  cfg     Comparison configuration entry
  /// \return true  Histograms are equal within selected method
  /// \return false Histograms differ within one of the comparison methods
  template<class Obj>
  bool CompareTwoObjects(const TObject* pObjL, const TObject* pObjR, const TString& objName,
                         const ObjectComparisonConfig& cfg);

  bool fbUseMC = false;  ///< Flag, if MC is used

  ECbmRecoMode fRecoMode = ECbmRecoMode::Timeslice;  ///< Reconstruction mode

  /// \brief A QA check-list map
  ///
  /// The check list is updated with new entries with the StoreCheckResult(tag, result) function, which is to be called
  /// from the Check() function. The result is stored to CheckFlags::fResult field, while the CheckFlags::fStatus stays
  /// uninitialized. Than, when the ReadCheckListFromConfig() function is called, the fStatus is updated according to
  /// the check list entries in the qa_tasks_config_mcbm.yaml/qa_tasks_config_cbm.yaml. If the fStatus is false
  /// (default), the check is ignored in the final verdict making.
  std::map<std::string, CheckFlags> fmCheckList;

  TClonesArray* fpBrEvents = nullptr;  ///< Pointer to CbmEvent branch
  CbmEvent* fpCurrentEvent = nullptr;  ///< Pointer to the current event
  std::string fsSetupName = "";  ///< Name of the setup (to draw on the canvases)
  TParameter<int> fNofEvents{"nEvents", 0};  ///< Number of processed events

  TString fsVersionTag = "";  ///< Version tag (git SHA etc.)
  TString fsDefaultTag = "";  ///< Default tag (git SHA etc.)

  std::shared_ptr<TFile> fpBenchmarkInput  = nullptr;  ///< A file with default ROOT objects used for the cross-check
  std::shared_ptr<TFile> fpBenchmarkOutput = nullptr;  ///< An output file for histograms cross-check

  ClassDefOverride(CbmQaTask, 0);
};


// *************************************************
// ** Inline and template function implementation **
// *************************************************

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename T>
bool CbmQaTask::CheckRange(std::string_view name, T var, T lo, T hi) const
{
  return CheckRange<T>(name, var, 0., lo, hi);
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename T>
bool CbmQaTask::CheckRange(std::string_view name, T var, T varErr, T lo, T hi) const
{
  bool ret = true;
  if (var + 3.5 * varErr < lo) {
    LOG(error) << fName << ": " << name << " is found to be under the lower limit (" << var << " +- 3.5 x " << varErr
               << " < " << lo << ')';
    ret = false;
  }
  if (var - 3.5 * varErr > hi) {
    LOG(error) << fName << ": " << name << " is found to be above the upper limit (" << var << " +-3.5 x " << varErr
               << " > " << hi << ')';
    ret = false;
  }
  if (ret) {
    LOG(debug) << fName << ": " << name << " is found to be within limit (" << lo << " <= " << var << " +- 3.5 x "
               << varErr << " <= " << hi << ')';
  }
  return ret;
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<class Obj>
bool CbmQaTask::CompareTwoObjects(const TObject* pObjL, const TObject* pObjR, const TString& objName,
                                  const ObjectComparisonConfig& cfg)
{
  const auto* pHistL = static_cast<const Obj*>(pObjL);
  const auto* pHistR = dynamic_cast<const Obj*>(pObjR);
  if (!pHistR) {
    LOG(error) << fName << "::CompareObjects(): the default " << pObjR->GetName()
               << " has different type to the new object";
    return false;
  }
  std::string opt = "";
  if (cfg.fPoint > 0) {
    opt += "p";
  }
  if (cfg.fRatio > 0) {
    opt += "r";
  }
  if (cfg.fStat > 0) {
    opt += "s";
  }

  auto comparator = CbmQaCompare<Obj>(pHistL, pHistR, 0);
  auto out        = comparator(opt, cfg.fStatOpt);

  if (!out.fConsistent) {
    LOG(info) << fName << ": This and default versions of the object " << pObjL->GetName() << " are incompatible";
    return false;
  }

  // Collect status and print log
  bool res = true;
  {
    std::stringstream msg;
    using std::setw;
    constexpr const char* kEqual = "\e[1;32mequal\e[0m";
    constexpr const char* kDiff  = "\e[1;32mdifferent\e[0m";
    msg << "\tobject " << setw(12) << pObjL->ClassName() << ' ' << setw(50) << objName;
    if (cfg.fPoint) {
      msg << ": point-by-point -> " << (out.fPointByPoint ? kEqual : kDiff);
      if (cfg.fPoint == 2) {
        res &= out.fPointByPoint;
      }
    }
    if (cfg.fRatio) {
      bool bOk = (out.fRatioLo >= cfg.fRatioMin && out.fRatioUp <= cfg.fRatioMax);
      msg << ", ratio -> " << (bOk ? kEqual : kDiff) << "(lo: " << out.fRatioLo << ", up: " << out.fRatioUp << ')';
      if (cfg.fRatio == 2) {
        res &= bOk;
      }
    }
    if (cfg.fStat) {
      bool bOk = out.fChi2NDF <= cfg.fChi2NdfMax;
      msg << ", stat. test -> " << (bOk ? kEqual : kDiff) << "(chi2/NDF: " << out.fChi2NDF << ')';
      if (cfg.fStat == 2) {
        res &= bOk;
      }
    }
    LOG(info) << msg.str();
  }

  // Write comparison result
  if (fpBenchmarkOutput.get()) {
    fpBenchmarkOutput->mkdir(objName);
    auto* pDir = fpBenchmarkOutput->GetDirectory(objName);
    pDir->cd();
    pObjL->Write(Form("%s_%s", pObjL->GetName(), fsVersionTag.Data()));
    pObjR->Write(Form("%s_%s", pObjL->GetName(), fsDefaultTag.Data()));
    if (true || !res) {  // Save canvas only if the histograms are different
      auto* pCanv = comparator.GetComparisonCanvas(cfg.fCanvOpt);
      if (pCanv) {
        pCanv->Write(Form("%s_cmp_canvas", pObjL->GetName()));
        delete pCanv;
      }
    }
  }
  return res;
}
