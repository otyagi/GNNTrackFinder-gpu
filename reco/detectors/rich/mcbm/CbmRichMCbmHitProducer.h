/* Copyright (C) 2019-2024 UGiessen/JINR-LIT, Giessen/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Semen Lebedev [committer], Adrian Amatus Weber, Martin Beyer */

#ifndef CBM_RICH_MCBM_HIT_PRODUCER
#define CBM_RICH_MCBM_HIT_PRODUCER

#include "CbmDigiManager.h"  // for ROOTCLING
#include "CbmRichRecGeoPar.h"
#include "FairTask.h"

class TClonesArray;
class TVector3;
class CbmEvent;
class CbmRichDigi;
class CbmRichMCbmDenoiseCnn;

class CbmRichMCbmMappingData {
 public:
  UInt_t fTrbId;
  UInt_t fChannel;
  Double_t fX;
  Double_t fY;
  Double_t fZ;
};

class CbmRichMCbmHitProducer : public FairTask {
 public:
  /**
     * \brief Default constructor.
     */
  CbmRichMCbmHitProducer();

  /**
     * \brief Destructor.
     */
  virtual ~CbmRichMCbmHitProducer();

  /**
     * \brief Inherited from FairTask.
     */
  virtual void SetParContainers();

  /**
     * \brief Inherited from FairTask.
     */
  virtual InitStatus Init();


  /**
     * \brief Inherited from FairTask.
     */
  virtual void Exec(Option_t* option);

  /**
     * \brief Inherited from FairTask.
     */
  virtual void Finish();


  /**
     * Processblock of data either event-by-event or CbmEvent
     */
  void ProcessData(CbmEvent* event);


  /**
     * Process RichDigi. CbmEvent can be NULL.
     */
  void ProcessDigi(CbmEvent* event, Int_t digiIndex);


  /**
     * Set mapping file path.
     */
  void SetMappingFile(const std::string& mappingFile) { fMappingFile = mappingFile; }


  /**
     * Set ICD base-file path.
     */
  void SetIcdFilenameBase(const std::string& icdFileBase) { fIcdFilenameBase = icdFileBase; }


  /**
    * Set ToT Limits.
    */
  void setToTLimits(double low, double high)
  {
    fToTLimitLow  = low;
    fToTLimitHigh = high;
  }


  /**
    * Set low ToT Limit.
    */
  void setToTLimitLow(double low) { fToTLimitLow = low; }


  /**
    * Set high ToT Limit.
    */
  void setToTLimitHigh(double high) { fToTLimitHigh = high; }


  /**
    * Apply ToT Cut
    */
  void applyToTCut() { fDoToT = true; }


  /**
    * Apply correction of the inter channel delay (ICD)
    */
  void applyICDCorrection(bool val = true) { fDoICD = val; }


  /**
    * Apply restriction to Mar2019 mRICH Acceptance (for Simulations)
    */
  void DoRestrictToAcc(bool val = true) { fRestrictToAcc = val; }

  /**
    * Apply restriction to full mRICH Acceptance (for Simulations)
    */
  void DoRestrictToFullAcc(bool val = true) { fRestrictToFullAcc = val; }

#if HAVE_ONNXRUNTIME
  void applyDenoiseNN(bool val = true) { fUseDenoiseNN = val; }
  void SetClassifierThreshold(float val) { fDenoiseCnnThreshold = val; }
#endif

 private:
#if HAVE_ONNXRUNTIME
  bool fUseDenoiseNN                                 = true;
  float fDenoiseCnnThreshold                         = 0.5;
  std::unique_ptr<CbmRichMCbmDenoiseCnn> fDenoiseCnn = nullptr;
  double fDenoiseCnnTime                             = 0.;
#endif

  CbmDigiManager* fDigiMan = nullptr;
  TClonesArray* fRichHits;                      // RICH hits
  TClonesArray* fCbmEvents          = nullptr;  // CbmEvent for time-based simulations
  bool fDoToT                       = false;
  bool fDoICD                       = false;
  bool fRestrictToAcc               = false;
  bool fRestrictToFullAcc           = false;
  bool fRestrictToAerogelAccDec2019 = false;
  double fToTLimitLow               = 0.;
  double fToTLimitHigh              = 1000.;

  std::map<Int_t, CbmRichMCbmMappingData> fRichMapping;

  double fTotalTime       = 0.;
  double fHitProducerTime = 0.;

  Int_t fNofTs;

  Int_t fNofDigis = 0;
  Int_t fNofHits  = 0;

  Int_t fNofEvents     = 0;
  Int_t fTotalNofDigis = 0;
  Int_t fTotalNofHits  = 0;

  Double_t fHitError;

  std::string fMappingFile;

  std::string fIcdFilenameBase = "";
  std::array<Double_t, 2304> fICD_offset_read;

  void InitMapping();

  bool isInToT(const double ToT);

  bool RestrictToAcc(TVector3& pos);
  bool RestrictToAcc(Double_t x, Double_t y);

  bool RestrictToFullAcc(TVector3& pos);
  bool RestrictToFullAcc(Double_t x, Double_t y);

  bool RestrictToAerogelAccDec2019(TVector3& pos);
  bool RestrictToAerogelAccDec2019(Double_t x, Double_t y);

  /**
     * \brief Add hit to the output array (and) CbmEvent if it is not NULL.
     */

  void AddHit(CbmEvent* event, TVector3& posHit, const CbmRichDigi* digi, Int_t index, Int_t PmtId);

  /**
     * function for loading of a created inter channel delay correction file.
     */
  void read_ICD(std::array<Double_t, 2304>& offsets, unsigned int iteration);


  /**
     * \brief Copy constructor.
     */
  CbmRichMCbmHitProducer(const CbmRichMCbmHitProducer&);

  /**
     * \brief Assignment operator.
     */
  CbmRichMCbmHitProducer& operator=(const CbmRichMCbmHitProducer&);

  ClassDef(CbmRichMCbmHitProducer, 1)
};

#endif
