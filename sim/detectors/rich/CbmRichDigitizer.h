/* Copyright (C) 2015-2024 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Semen Lebedev, Andrey Lebedev [committer], Martin Beyer */

/**
* \file CbmRichDigitizer.h
*
* \brief Class for producing RICH digis from from MCPoints.
*
* \author S.Lebedev
* \date 2015
**/

#ifndef CBM_RICH_DIGITIZER
#define CBM_RICH_DIGITIZER

#include "CbmDefs.h"
#include "CbmDigitize.h"
#include "CbmRichDigi.h"
#include "CbmRichPmt.h"
#include "CbmRichPmtType.h"

#include <map>

class TClonesArray;
class CbmRichPoint;
class CbmLink;

/**
* \class CbmRichDigitizer
*
* \brief Class for producing RICH digis from from MCPoints.
*
* \author S.Lebedev
* \date 2015
**/
class CbmRichDigitizer : public CbmDigitize<CbmRichDigi> {
public:
  /** Default constructor */
  CbmRichDigitizer() : CbmDigitize<CbmRichDigi>("RichDigitize") {};

  /** Destructor */
  virtual ~CbmRichDigitizer() = default;

  /** Copy constructor (disabled) */
  CbmRichDigitizer(const CbmRichDigitizer&) = delete;

  /** Assignment operator (disabled) */
  CbmRichDigitizer& operator=(const CbmRichDigitizer&) = delete;

  /** 
    * \brief Detector system ID
    * \return kRich
    */
  ECbmModuleId GetSystemId() const { return ECbmModuleId::kRich; }

  /** Inherited from FairTask */
  virtual InitStatus Init();

  /** Inherited from FairTask */
  virtual void Exec(Option_t* option);

  /** Inherited from FairTask */
  virtual void Finish();

  /** Set detector type */
  void SetDetectorType(CbmRichPmtTypeEnum detType) { fDetectorType = detType; }

  /** Set time resolution for digis */
  void SetTimeResolution(Double_t dt) { fTimeResolution = dt; }

  /**
    * \brief Set pixel dead time between signals (without time smearing). 
    * The resulting dead time at digi level is approximately:
    * pixel dead time - 2*5*time resolution (5 sigma from gauss smearing for 2 digis each).
    */
  void SetPixelDeadTime(Double_t dt) { fPixelDeadTime = dt; }

  /** 
   * \brief Set crosstalk probability for horizontal, vertical and diagonal directions.
   * \param horizontal probability of crosstalk for direct horizontal pixels
   * \param vertical probability of crosstalk for direct vertical pixels
   * \param diagonal probability of crosstalk for direct diagonal pixels
   */
  void SetCrossTalkProbability(Double_t horizontal, Double_t vertical, Double_t diagonal)
  {
    fCrossTalkProbability[0] = horizontal;
    fCrossTalkProbability[1] = vertical;
    fCrossTalkProbability[2] = diagonal;
  }

  /**
    * \brief Set event noise rate per McRichPoint / per  pixel / per second :
    * nofNoiseSignals = nofRichPoints * nofPixels * event noise interval * (event noise rate / 1.e9);
    */
  void SetEventNoiseRate(Double_t noise) { fEventNoiseRate = noise; }

  /**
    * \brief Set event noise interval in ns.
    * Add noise signals from: current event time to current event time + event noise interval.
    */
  void SetEventNoiseInterval(Double_t dT) { fEventNoiseInterval = dT; }

  /** Set dark rate per pixel in Hz */
  void SetDarkRatePerPixel(Double_t darkRate) { fDarkRatePerPixel = darkRate; }

  /**
    * \brief Set charged particle cluster signal probability for direct neighbors.
    * The probability for pixels further away decreases with 1/distance.
    * Produced signals are limited to the same PMT as the source signal pixel and the defined cluster size. 
    */
  void SetClusterSignalProbability(Double_t intensity) { fClusterSignalProbability = intensity; }

  /**
    * \brief Set cluster size for charged particle clusters. 
    * Resulting cluster size : (2*size + 1)*(2*size + 1) pixels.
    */
  void SetClusterSize(UInt_t size) { fClusterSize = size; }

  /** Set collection efficiency for photoelectrons in PMT optics */
  void SetCollectionEfficiency(Double_t collEff) { fPmt.SetCollectionEfficiency(collEff); }

  /** Set additional smearing of MC Points due to light scattering in mirror */
  // void SetSigmaMirror(Double_t sigMirror) {fSigmaMirror = sigMirror;}

private:
  Int_t fEventNum {};

  TClonesArray* fRichPoints {nullptr};       // RICH MC points
  TClonesArray* fRichDigis {nullptr};        // RICH digis (output array)
  TClonesArray* fRichDigiMatches {nullptr};  // RICH digi matches (output array)
  TClonesArray* fMcTracks {nullptr};         // Monte-Carlo tracks

  Int_t fNofPoints {};   // total number of MCPoints processed
  Int_t fNofDigis {};    // total number of digis created
  Double_t fTimeTot {};  // sum of execution time

  CbmRichPmt fPmt {};
  CbmRichPmtTypeEnum fDetectorType {CbmRichPmtTypeCosy17NoWls};

  Double_t fTimeResolution {1.};              // in ns, time resolution for digis
  Double_t fPixelDeadTime {50.};              // in ns, deadtime for one pixel
  // probability of crosstalk depending on direction of neighbour pixel
  // index 0: horizontal, 1: vertical, 2: diagonal
  std::array<Double_t, 3> fCrossTalkProbability{0.02, 0.02, 0.005};
  Double_t fEventNoiseRate {5.};              // noise rate per McRichPoint / per  pixel / per second
  Double_t fEventNoiseInterval {50.};         // in ns, interval to generate event noise signals
  Double_t fDarkRatePerPixel {1000.};         // dark rate per pixel in Hz
  Double_t fClusterSignalProbability {0.37};  // cluster intensity
  UInt_t fClusterSize {0};                    // cluster size

  std::map<Int_t, std::vector<std::pair<Double_t, CbmLink*>>>
    fSignalBuffer {};  // first: pixel address, second: signal buffer

  std::map<Int_t, Double_t> fPixelsLastFiredTime {};  // first: pixel address, second: last fired time

  /**
    * \brief Process current MC event.
    * \return Number of processed RichPoints.
    */
  Int_t ProcessMcEvent();

  /** Process CbmRichPoint. Main method which is called for all CbmRichPoints. */
  void ProcessPoint(CbmRichPoint* point, Int_t pointId, Int_t eventNum, Int_t inputNum);

  /** Add signal to signal buffer */
  void AddSignalToBuffer(Int_t address, Double_t time, const CbmLink& link);

  /**
    * \brief Add crosstalk assuming fCrossTalkProbability.
    * Only add maximum one cross talk signal per MC point.
    * Limited to the same PMT as the source MC point.
    */
  void AddCrossTalk(Int_t address, Double_t time, const CbmLink& link);

  /**
    * \brief Add additional signals to nearby pixels if a charged particle
    * directly passes through the PMT, given fClusterSignalProbability and fClusterSize.
    * Limited to the same PMT. Currently independant of mass, momentum, etc.
    */
  void AddChargedParticleCluster(Int_t address, Double_t time, Int_t eventNum, Int_t inputNum);

  /**
    * \brief Add additional noise for each event based on fEventNoiseRate 
    * and fEventNoiseInterval.
    */
  void AddEventNoise(Int_t eventNum, Int_t inputNum);

  /** Add noise between events based on fDarkRatePerPixel */
  void AddDarkRateNoise(Double_t oldEventTime, Double_t newEventTime);

  /**
    * \brief Process signals in all buffers until processTime. 
    * New Digis are only created until processTime - fPixelDeadTime,
    * since potential links are not added to the buffer yet.
    * Links are potentially added until processTime.
    * \param processTime time in ns
    * \return Number of Digis sent to DAQ 
    */
  Int_t ProcessBuffers(Double_t processTime);

  ClassDef(CbmRichDigitizer, 3)
};

#endif
