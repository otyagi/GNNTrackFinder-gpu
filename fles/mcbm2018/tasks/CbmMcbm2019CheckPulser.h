/* Copyright (C) 2019-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#ifndef CBMMCBM2019CHECKPULSER_H
#define CBMMCBM2019CHECKPULSER_H

/// CBMROOT headers
#include "CbmDefs.h"
#include "CbmTofDigi.h"

#include "TimesliceMetaData.h"

/// FAIRROOT headers
#include "FairTask.h"

/// External headers (ROOT, boost, ...)
#include "TString.h"

/// C/C++ headers
#include <vector>

class TClonesArray;
class TH1;
class TH2;
class TProfile;
class CbmDigiManager;

class CbmMcbm2019CheckPulser : public FairTask {
public:
  CbmMcbm2019CheckPulser();

  CbmMcbm2019CheckPulser(const CbmMcbm2019CheckPulser&) = delete;
  CbmMcbm2019CheckPulser operator=(const CbmMcbm2019CheckPulser&) = delete;

  /** Constructor with parameters (Optional) **/
  //  CbmMcbm2019CheckPulser(Int_t verbose);


  /** Destructor **/
  ~CbmMcbm2019CheckPulser();


  /** Initiliazation of task at the beginning of a run **/
  virtual InitStatus Init();

  /** ReInitiliazation of task when the runID changes **/
  virtual InitStatus ReInit();


  /** Executed for each event. **/
  virtual void Exec(Option_t*);

  /** Load the parameter container from the runtime database **/
  virtual void SetParContainers();

  /** Finish task called at the end of the run **/
  virtual void Finish();

  void SetOffsetSearchRange(Int_t val = 1000) { fOffsetRange = val; }

  void SetStsOffsetSearchRange(Int_t val = 1000) { fStsOffsetRange = val; }

  void SetMuchOffsetSearchRange(Int_t val = 1000) { fMuchOffsetRange = val; }

  void SetTrdOffsetSearchRange(Int_t val = 1000) { fTrdOffsetRange = val; }

  void SetTofOffsetSearchRange(Int_t val = 1000) { fTofOffsetRange = val; }

  void SetRichOffsetSearchRange(Int_t val = 1000) { fRichOffsetRange = val; }

  void SetPsdOffsetSearchRange(Int_t val = 1000) { fPsdOffsetRange = val; }

  inline void SetBmonPulserTotLimits(UInt_t uMin, UInt_t uMax)
  {
    fuMinTotPulserBmon = uMin;
    fuMaxTotPulserBmon = uMax;
  }
  inline void SetStsPulserAdcLimits(UInt_t uMin, UInt_t uMax)
  {
    fuMinAdcPulserSts = uMin;
    fuMaxAdcPulserSts = uMax;
  }
  inline void SetMuchPulserAdcLimits(UInt_t uMin, UInt_t uMax)
  {
    fuMinAdcPulserMuch = uMin;
    fuMaxAdcPulserMuch = uMax;
  }
  inline void SetTrdPulserChargeLimits(UInt_t uMin, UInt_t uMax)
  {
    fuMinChargePulserTrd = uMin;
    fuMaxChargePulserTrd = uMax;
  }
  inline void SetTofPulserTotLimits(UInt_t uMin, UInt_t uMax)
  {
    fuMinTotPulserTof = uMin;
    fuMaxTotPulserTof = uMax;
  }
  inline void SetRichPulserTotLimits(UInt_t uMin, UInt_t uMax)
  {
    fuMinTotPulserRich = uMin;
    fuMaxTotPulserRich = uMax;
  }
  inline void SetPsdPulserAdcLimits(UInt_t uMin, UInt_t uMax)
  {
    fuMinAdcPulserPsd = uMin;
    fuMaxAdcPulserPsd = uMax;
  }

  inline void SetOutFilename(TString sNameIn) { fOutFileName = sNameIn; }

  inline void SetStsAddress(UInt_t uAddress) { fuStsAddress = uAddress; }
  inline void SetMuchAsic(UInt_t uAsic) { fuMuchAsic = uAsic; }
  inline void SetMuchChanRange(UInt_t uFirstChan, UInt_t uLastChan = kuNbChanSMX)
  {
    fuMuchFirstCha = uFirstChan;
    fuMuchLastChan = uLastChan;
  }
  inline void SetTrdAddress(UInt_t uAddress) { fuTrdAddress = uAddress; }
  inline void SetPsdAddress(UInt_t uAddress) { fuPsdAddress = uAddress; }

private:
  void CheckTimeOrder();
  Int_t CheckIfSorted(TClonesArray*, TH1*, Double_t&, TString);

  void CheckInterSystemOffset();

  template<class Digi>
  Int_t FillSystemOffsetHistos(TH1* histo, TH2* histoEvo, TH2* histoEvoLong, TProfile* profMeanEvo, TH2* histoAFCK,
                               const Double_t T0Time, const Int_t offsetRange, Int_t iStartDigi,
                               ECbmModuleId iDetId = ECbmModuleId::kLastModule);

  Int_t CalcNrBins(Int_t);
  void CreateHistos();
  void WriteHistos();


  /** Digi data **/
  CbmDigiManager* fDigiMan                     = nullptr;  //!
  const std::vector<CbmTofDigi>* fBmonDigiVector = nullptr;  //!
  TClonesArray* fBmonDigiArray                   = nullptr;  //!
  TClonesArray* fTimeSliceMetaDataArray        = nullptr;  //!
  const TimesliceMetaData* pTsMetaData         = nullptr;

  /// Constants
  static const UInt_t kuNbChanSMX      = 128;
  static const UInt_t kuMaxNbStsDpbs   = 2;
  static const UInt_t kuMaxNbMuchDpbs  = 6;
  static const UInt_t kuMaxNbMuchAsics = 36;
  static const UInt_t kuDefaultAddress = 0xFFFFFFFF;
  static const UInt_t kuMaxChannelSts  = 3000;

  /// Variables to store the previous digi time
  Double_t fPrevTimeBmon = 0.;
  Double_t fPrevTimeSts  = 0.;
  Double_t fPrevTimeMuch = 0.;
  Double_t fPrevTimeTrd  = 0.;
  Double_t fPrevTimeTof  = 0.;
  Double_t fPrevTimeRich = 0.;
  Double_t fPrevTimePsd  = 0.;

  /// Variables to store the first digi fitting the previous Bmon hits
  /// => Time-order means the time window for following one can only be in a later digi
  Int_t fPrevBmonFirstDigiSts  = 0;
  Int_t fPrevBmonFirstDigiMuch = 0;
  Int_t fPrevBmonFirstDigiTrd  = 0;
  Int_t fPrevBmonFirstDigiTof  = 0;
  Int_t fPrevBmonFirstDigiRich = 0;
  Int_t fPrevBmonFirstDigiPsd  = 0;

  /// User settings: Data correction parameters
  /// Charge cut
  UInt_t fuMinTotPulserBmon   = 182;
  UInt_t fuMaxTotPulserBmon   = 190;
  UInt_t fuMinAdcPulserSts    = 90;
  UInt_t fuMaxAdcPulserSts    = 100;
  UInt_t fuMinAdcPulserMuch   = 5;
  UInt_t fuMaxAdcPulserMuch   = 15;
  UInt_t fuMinChargePulserTrd = 0;
  UInt_t fuMaxChargePulserTrd = 70000;
  UInt_t fuMinTotPulserTof    = 182;
  UInt_t fuMaxTotPulserTof    = 190;
  UInt_t fuMinTotPulserRich   = 90;
  UInt_t fuMaxTotPulserRich   = 105;
  UInt_t fuMinAdcPulserPsd    = 90;
  UInt_t fuMaxAdcPulserPsd    = 100;
  /// Channel selection
  UInt_t fuStsAddress   = kuDefaultAddress;
  UInt_t fuStsFirstCha  = kuMaxChannelSts;
  UInt_t fuStsLastChan  = kuMaxChannelSts;
  UInt_t fuMuchAsic     = kuMaxNbMuchAsics;
  UInt_t fuMuchFirstCha = kuNbChanSMX;
  UInt_t fuMuchLastChan = kuNbChanSMX;
  UInt_t fuTrdAddress   = kuDefaultAddress;
  UInt_t fuPsdAddress   = kuDefaultAddress;

  //
  Int_t fNrTs = 0;

  Int_t fOffsetRange     = 1000;
  Int_t fStsOffsetRange  = 1000;
  Int_t fMuchOffsetRange = 1000;
  Int_t fTrdOffsetRange  = 1000;
  Int_t fTofOffsetRange  = 1000;
  Int_t fRichOffsetRange = 1000;
  Int_t fPsdOffsetRange  = 1000;

  Int_t fBinWidth = 1;

  TH1* fBmonStsDiff       = nullptr;
  TH1* fBmonMuchDiff      = nullptr;
  TH1* fBmonTrdDiff       = nullptr;
  TH1* fBmonTofDiff       = nullptr;
  TH1* fBmonRichDiff      = nullptr;
  TH1* fBmonPsdDiff       = nullptr;
  TH2* fBmonPsdDiffCharge = nullptr;

  TH2* fBmonStsDiffEvo  = nullptr;
  TH2* fBmonMuchDiffEvo = nullptr;
  TH2* fBmonTrdDiffEvo  = nullptr;
  TH2* fBmonTofDiffEvo  = nullptr;
  TH2* fBmonRichDiffEvo = nullptr;
  TH2* fBmonPsdDiffEvo  = nullptr;

  TH2* fBmonStsDiffEvoLong  = nullptr;
  TH2* fBmonMuchDiffEvoLong = nullptr;
  TH2* fBmonTrdDiffEvoLong  = nullptr;
  TH2* fBmonTofDiffEvoLong  = nullptr;
  TH2* fBmonRichDiffEvoLong = nullptr;
  TH2* fBmonPsdDiffEvoLong  = nullptr;

  Double_t fdStartTime     = -1;
  TProfile* fBmonStsMeanEvo  = nullptr;
  TProfile* fBmonMuchMeanEvo = nullptr;
  TProfile* fBmonTrdMeanEvo  = nullptr;
  TProfile* fBmonTofMeanEvo  = nullptr;
  TProfile* fBmonRichMeanEvo = nullptr;
  TProfile* fBmonPsdMeanEvo  = nullptr;

  TH1* fBmonBmonDiff = nullptr;
  TH1* fStsStsDiff   = nullptr;
  TH1* fMuchMuchDiff = nullptr;
  TH1* fTrdTrdDiff   = nullptr;
  TH1* fTofTofDiff   = nullptr;
  TH1* fRichRichDiff = nullptr;
  TH1* fPsdPsdDiff   = nullptr;

  TH2* fBmonStsNb  = nullptr;
  TH2* fBmonMuchNb = nullptr;
  TH2* fBmonTrdNb  = nullptr;
  TH2* fBmonTofNb  = nullptr;
  TH2* fBmonRichNb = nullptr;
  TH2* fBmonPsdNb  = nullptr;

  Int_t fiBmonNb = 0;
  Int_t fiStsNb  = 0;
  Int_t fiMuchNb = 0;
  Int_t fiTrdNb  = 0;
  Int_t fiTofNb  = 0;
  Int_t fiRichNb = 0;
  Int_t fiPsdNb  = 0;

  TH1* fBmonAddress = nullptr;
  TH1* fBmonChannel = nullptr;

  TH2* fBmonStsDpbDiff = nullptr;
  TH2* fBmonStsDpbDiffEvo[kuMaxNbStsDpbs];
  TH1* fStsDpbCntsEvo[kuMaxNbStsDpbs];

  TH2* fBmonMuchRocDiff  = nullptr;
  TH2* fBmonMuchAsicDiff = nullptr;
  TH2* fBmonMuchAsicDiffEvo[kuMaxNbMuchAsics];

  TH2* fDigisPerAsicEvo = nullptr;
  Double_t fdLastMuchDigi[kuMaxNbMuchAsics][kuNbChanSMX];
  Double_t fdLastMuchDigiPulser[kuMaxNbMuchAsics][kuNbChanSMX];
  TH2* fSameChanDigisDistEvo = nullptr;

  Double_t fdLastBmonDigiPulser = 0;

  TH2* fDigiTimeEvoBmon = nullptr;
  TH2* fDigiTimeEvoSts  = nullptr;
  TH2* fDigiTimeEvoMuch = nullptr;
  TH2* fDigiTimeEvoTof  = nullptr;


  TString fOutFileName {"data/HistosPulserChecker.root"};

  ClassDef(CbmMcbm2019CheckPulser, 1);
};

#endif  // CBMMCBM2019CHECKPULSER_H
