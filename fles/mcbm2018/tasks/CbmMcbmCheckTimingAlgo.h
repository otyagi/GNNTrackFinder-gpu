/* Copyright (C) 2020-2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer], Andreas Redelbach, Alexandru Bercuci */

#ifndef CBMMCBMCHECKTIMINGALGO_H
#define CBMMCBMCHECKTIMINGALGO_H

#include "CbmModuleList.h"
#include "CbmTofDigi.h"
#include "CbmTrdDigi.h"
#include "CbmTsEventHeader.h"

#include "TString.h"

#include <unordered_set>
#include <vector>

class TH1;
class TH2;
class CbmDigiManager;
class CbmStsDigi;
class CbmMuchDigi;
class CbmTrdDigi;
class CbmTofDigi;
class CbmRichDigi;
class CbmPsdDigi;
class CbmBmonDigi;
class CheckTimingDetector {
public:
  CheckTimingDetector() { ; }
  CheckTimingDetector(ECbmModuleId detIdIn, std::string sNameIn)
  {
    detId = detIdIn;
    sName = sNameIn;
  }

  /// Settings
  ECbmModuleId detId     = ECbmModuleId::kNotExist;
  std::string sName      = "Invalid";
  Double_t dTimeRangeBeg = -1000.0;
  Double_t dTimeRangeEnd = 1000.0;
  UInt_t uRangeNbBins    = 320;
  UInt_t uChargeCutMin =
    0;  /// Charge cut used for example to reject/select pulser, no effect if equal, select if min < max, reject if max < min
  UInt_t uChargeCutMax =
    0;  /// Charge cut used for example to reject/select pulser, no effect if equal, select if min < max, reject if max < min
  UInt_t uNviews                 = 1;     /// No of views for each detector
  std::vector<std::string> vName = {""};  /// view string definitions; to be defined by detectors
  /// Book-keeping variables
  Double_t dPrevTime      = 0.;
  Int_t iPrevRefFirstDigi = 0;
};

class CbmMcbmCheckTimingAlgo {
public:
  CbmMcbmCheckTimingAlgo();

  CbmMcbmCheckTimingAlgo(const CbmMcbmCheckTimingAlgo&) = delete;
  CbmMcbmCheckTimingAlgo operator=(const CbmMcbmCheckTimingAlgo&) = delete;

  /** Constructor with parameters (Optional) **/
  //  CbmMcbmCheckTimingAlgo(Int_t verbose);


  /** Destructor **/
  ~CbmMcbmCheckTimingAlgo();


  /** Initiliazation of task at the beginning of a run **/
  Bool_t Init();

  /** ReInitiliazation of task when the runID changes **/
  Bool_t ReInit();


  /** Executed for each event. **/
  void ProcessTs();

  /** Load the parameter container from the runtime database **/
  void SetParContainers();

  /** Finish task called at the end of the run **/
  void Finish();

  inline void SetOutFilename(TString sNameIn) { fOutFileName = sNameIn; }
  void WriteHistos();

  void SetReferenceDetector(ECbmModuleId refDetIn, std::string sNameIn, Double_t dTimeRangeBegIn = -1000.0,
                            Double_t dTimeRangeEndIn = 1000.0, UInt_t uRangeNbBinsIn = 320, UInt_t uChargeCutMinIn = 0,
                            UInt_t uChargeCutMaxIn = 0);
  void AddCheckDetector(ECbmModuleId detIn, std::string sNameIn, Double_t dTimeRangeBegIn = -1000.0,
                        Double_t dTimeRangeEndIn = 1000.0, UInt_t uRangeNbBinsIn = 320, UInt_t uChargeCutMinIn = 0,
                        UInt_t uChargeCutMaxIn = 0);
  void RemoveCheckDetector(ECbmModuleId detIn);
  void SetDetectorDifferential(ECbmModuleId detIn, std::vector<std::string> vName);

  void SetTrdPeakWidthNs(Double_t val = 120.) { fTrdPeakWidthNs = val; }
  void SetStsPeakWidthNs(Double_t val = 30.) { fStsPeakWidthNs = val; }
  void SetMuchPeakWidthNs(Double_t val = 100.) { fMuchPeakWidthNs = val; }
  void SetTofPeakWidthNs(Double_t val = 20.) { fTofPeakWidthNs = val; }
  void SetRichPeakWidthNs(Double_t val = 40.) { fRichPeakWidthNs = val; }
  void SetPsdPeakWidthNs(Double_t val = 20.) { fPsdPeakWidthNs = val; }

private:
  void CheckDataPresence(CheckTimingDetector detToCheck);
  void CreateHistos();

  template<class DigiRef>
  void CheckInterSystemOffset();
  template<class Digi>
  void FillTimeOffsetHistos(const Double_t dRefTime, const Double_t dRefCharge, UInt_t uDetIdx);
  /** @brief Retrieve digi (time,charge,addres) info.
   * SHOULD BE IMPLEMENTED BY DETECTORS IF MORE DIFFERENTIAL STUDIES ARE NEEDED
   * @param iDigi digi index in the digi array
   * @param vec on return contains the signal(s), time(s) and address pairs. Should be allocated by the user
   * @param detId if needed spec
   * @return size of vector
   */
  template<class Digi>
  UInt_t GetDigiInfo(UInt_t iDigi, std::vector<std::tuple<double, double, UInt_t>>* vec,
                     ECbmModuleId detId = ECbmModuleId::kNotExist);
  /** @brief Retrieve the detector view corresponding to the digi data (@see CheckTimingDetector::vName).
   * @param det detector definitions
   * @param info tuple of digi info (time, charge, address or more specific info)
   * @return the view index for the curent data or -1 if there is none
   */
  template<class Digi>
  int GetViewId(CheckTimingDetector det, std::tuple<double, double, UInt_t> info);

  /** Input array from previous already existing data level **/
  CbmDigiManager* fDigiMan = nullptr;  //!

  /** Bmon is not included in CbmDigiManager, so add it explicitly here **/
  const std::vector<CbmBmonDigi>* fpBmonDigiVec = nullptr;  //!

  /** @brief Pointer to the Timeslice start time used to write it to the output tree
      @remark since we hand this to the FairRootManager it also wants to delete it and we do not have to take care of deletion
   **/
  const CbmTsEventHeader* fCbmTsEventHeader = nullptr;

  //
  UInt_t fuNbTs = 0;

  CheckTimingDetector fRefDet {CheckTimingDetector(ECbmModuleId::kBmon, "BMON")};
  std::vector<CheckTimingDetector> fvDets {
    CheckTimingDetector(ECbmModuleId::kSts, "Sts"),   CheckTimingDetector(ECbmModuleId::kMuch, "Much"),
    CheckTimingDetector(ECbmModuleId::kTrd, "Trd"),   CheckTimingDetector(ECbmModuleId::kTof, "Tof"),
    CheckTimingDetector(ECbmModuleId::kRich, "Rich"), CheckTimingDetector(ECbmModuleId::kPsd, "Psd")};

  /// vectors storing histograms for each detector under investigation
  std::map<ECbmModuleId, std::vector<TH1*>> fvhDetSelfDiff           = {};
  std::map<ECbmModuleId, std::vector<TH1*>> fvhDetToRefDiff          = {};
  std::map<ECbmModuleId, std::vector<TH2*>> fvhDetToRefDiffRefCharge = {};
  std::map<ECbmModuleId, std::vector<TH2*>> fvhDetToRefDiffDetCharge = {};
  std::map<ECbmModuleId, std::vector<TH2*>> fvhDetToRefDiffEvo       = {};
  std::map<ECbmModuleId, std::vector<TH2*>> fvhDetToRefDiffEvoLong   = {};

  /** Name of the histogram output file **/
  TString fOutFileName = "data/HistosCheckTiming.root";
  Double_t DetPeakPosSingle;
  Double_t DetAverageSingle;
  Double_t fTrdPeakWidthNs   = 120.;
  Double_t fStsPeakWidthNs   = 30.;
  Double_t fMuchPeakWidthNs  = 100.;
  Double_t fTofPeakWidthNs   = 20.;
  Double_t fRichPeakWidthNs  = 40.;
  Double_t fPsdPeakWidthNs   = 20.;

  std::map<ECbmModuleId, std::unordered_set<std::string>> fUnimplementedView = {};

  ClassDefNV(CbmMcbmCheckTimingAlgo, 1);
};

#endif
