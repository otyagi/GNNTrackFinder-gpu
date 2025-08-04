/* Copyright (C) 2012-2024 UGiessen/JINR-LIT, Giessen/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Andrey Lebedev, Semen Lebedev [committer], Martin Beyer */

#ifndef CBM_RICH_URQMD_TEST
#define CBM_RICH_URQMD_TEST

#include "FairTask.h"
class TClonesArray;
class CbmRichRing;
class TCanvas;
class CbmHistManager;
class CbmMCDataArray;
class CbmMCEventList;
class CbmDigiManager;
class CbmLink;

#include <map>
#include <vector>


class CbmRichUrqmdTest : public FairTask {

 public:
  /**
     * \brief Standard constructor.
     */
  CbmRichUrqmdTest();

  /**
     * \brief Standard destructor.
     */
  virtual ~CbmRichUrqmdTest();

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
     * \brief Set output directory where you want to write results (figures and json).
     * \param[in] dir Path to the output directory.
     */
  void SetOutputDir(const std::string& dir) { fOutputDir = dir; }


 private:
  CbmHistManager* fHM = nullptr;

  std::string fOutputDir = "";  // output dir for results

  TClonesArray* fRichHits        = nullptr;
  TClonesArray* fRichRings       = nullptr;
  CbmMCDataArray* fRichPoints    = nullptr;
  CbmMCDataArray* fMcTracks      = nullptr;
  TClonesArray* fRichRingMatches = nullptr;
  TClonesArray* fRichProjections = nullptr;
  CbmDigiManager* fDigiMan       = nullptr;
  CbmMCEventList* fEventList     = nullptr;

  int fEventNum   = 0;
  int fMinNofHits = 7;  // Min number of hits in ring for detector acceptance calculation.

  std::map<CbmLink, int> fNofHitsInRingMap;  // Number of hits in the MC RICH ring

  std::vector<std::pair<int, int>> fVertexZStsSlices;  // Slices (Zmin - Zmax) inside STS

  /**
     * \brief Initialize histograms.
     */
  void InitHistograms();

  void FillRichRingNofHits();

  /**
     * \brief
     */
  void NofRings();

  /**
     * \brief
     */
  void NofHitsAndPoints();

  /**
     * \brief
     */
  void NofProjections();

  /**
     * \brief
     */
  void Vertex();


  /**
     *  \brief Draw histograms.
     */
  void DrawHist();

  void PmtXYSource();

  /**
     * \brief Copy constructor.
     */
  CbmRichUrqmdTest(const CbmRichUrqmdTest&);

  /**
     * \brief Assignment operator.
     */
  CbmRichUrqmdTest& operator=(const CbmRichUrqmdTest&);

  ClassDef(CbmRichUrqmdTest, 1)
};

#endif
