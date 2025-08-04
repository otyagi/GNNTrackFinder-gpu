/* Copyright (C) 2012-2023 UGiessen/JINR-LIT, Giessen/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Semen Lebedev [committer], Martin Beyer */

/**
* \file CbmRichEventDisplay.h
*
* \brief Event display for the RICH detector.
*
* \author Semen Lebedev
* \date 2012
**/

#ifndef CBM_RICH_EVENT_DISPLAY
#define CBM_RICH_EVENT_DISPLAY

#include "CbmDigiManager.h"
#include "CbmHistManager.h"

#include <FairTask.h>

#include <TClonesArray.h>

#include <string>

class CbmRichRing;
class CbmEvent;

/**
* \class CbmRichEventDisplay
*
* \brief Event display for the RICH detector.
*
* \author Semen Lebedev
* \date 2012
**/
class CbmRichEventDisplay : public FairTask {
public:
  /** Default constructor */
  CbmRichEventDisplay() : FairTask("CbmRichEventDisplay") {};

  /** Destructor */
  ~CbmRichEventDisplay() = default;

  /** Copy constructor (disabled) */
  CbmRichEventDisplay(const CbmRichEventDisplay&) = delete;

  /** Assignment operator (disabled) */
  CbmRichEventDisplay& operator=(const CbmRichEventDisplay&) = delete;

  /** Inherited from FairTask */
  InitStatus Init();

  /** Inherited from FairTask */
  void Exec(Option_t* opt);

  /** Inherited from FairTask */
  void Finish();

  /** Draw rings (Enabled by default) */
  void SetDrawRings(bool b) { fDrawRings = b; }

  /** Draw hits (Enabled by default) */
  void SetDrawHits(bool b) { fDrawHits = b; }

  /** Draw points (Disabled by default) */
  void SetDrawPoints(bool b) { fDrawPoints = b; }

  /** Draw projections (Enabled by default) */
  void SetDrawProjections(bool b) { fDrawProjections = b; }

  /**
    * \brief Set output directory where you want to write results (figures and json).
    * \param[in] dir Path to the output directory.
    */
  void SetOutputDir(const std::string& dir) { fOutputDir = dir; }

  /**
    * \brief Set output file format (any combination of png, gif, eps, svg and pdf).
    * \param[in] format File format, e.g. png or png,pdf. (Default: png,pdf)
    */
  void SetOutputFormat(const std::string& format) { fOutputFormat = format; }

private:
  int fEventNum {};

  CbmDigiManager* fDigiManager {nullptr};
  CbmHistManager* fHM {nullptr};

  TClonesArray* fCbmEvents {nullptr};
  TClonesArray* fRichPoints {nullptr};
  TClonesArray* fRichHits {nullptr};
  TClonesArray* fRichRings {nullptr};
  TClonesArray* fRichMatches {nullptr};
  TClonesArray* fRichProjections {nullptr};

  std::string fOutputDir {"RichEventDisplays"};
  std::string fOutputFormat {"png,pdf"};

  bool fDrawRings {true};
  bool fDrawHits {true};
  bool fDrawPoints {false};
  bool fDrawProjections {true};

  void DrawOneEvent(CbmEvent* event);

  void DrawOnePmtPlane(const std::string& plane, CbmEvent* event);

  void DrawEllipse(CbmRichRing* ring);

  ClassDef(CbmRichEventDisplay, 2);
};

#endif
