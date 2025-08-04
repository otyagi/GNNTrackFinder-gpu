/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Alexandru Bercuci */

#ifndef CBMTASKTRDHITFINDER_H
#define CBMTASKTRDHITFINDER_H

#include "CbmTrdCluster.h"
#include "CbmTrdDigi.h"
#include "CbmTrdHit.h"
#include "FairTask.h"
#include "trd/Hit.h"
#include "trd/Hitfind.h"

#include <gsl/span>
#include <map>
#include <set>
#include <vector>

/** CbmTaskTrdHitFinder.h
 *@author Dominik Smith <d.smith@gsi.de>
 **
 ** Task to build TRD and TRD2D hits from yaml configuratin. 
 **/
class CbmTaskTrdHitFinder : public FairTask {

 public:
  /**
   * \brief Default constructor.
   */
  CbmTaskTrdHitFinder();

  /**
   * \brief Default destructor.
   */
  ~CbmTaskTrdHitFinder();

  /** Initialisation **/
  virtual InitStatus Init();
  virtual void SetParContainers();

  /** \brief Executed task **/
  virtual void Exec(Option_t* option);

  /** Finish task **/
  virtual void Finish();

 private:
  CbmTaskTrdHitFinder(const CbmTaskTrdHitFinder&);
  CbmTaskTrdHitFinder& operator=(const CbmTaskTrdHitFinder&);

  /**
   * @brief Build hits from clusters for a given module
   */
  template<class TModule, class TCluster>
  void BuildHits(TModule* mod, std::vector<TCluster>* clusters);

  template<class TCluster>
  void AddClusters(std::vector<TCluster>* clusters);
  void AddHits(gsl::span<cbm::algo::trd::Hit> hits);

  /** @brief Create one algo object for each RPC **/
  bool InitAlgos();

  /** Output array of CbmTrdCluster **/
  std::vector<CbmTrdCluster>* fClusters = nullptr;

  /** @brief Output array of CbmTrdHit */
  std::vector<CbmTrdHit>* fHits = nullptr;

  /** @brief Hit finding algorithm */
  std::unique_ptr<cbm::algo::trd::Hitfind> fAlgo;

  /** @brief Number of processed time slices */
  UInt_t fNrTs = 0;

  /** @brief Number of processed events (without CbmEvent corresponds to nr of exec calls) */
  UInt_t fNrEvents = 0;

  /** @brief Number of digis as input for the hit production. */
  UInt_t fNrDigis = 0;

  /** @brief Number of produced clusters. */
  UInt_t fNrClusters = 0;

  /** @brief Number of produced hits. */
  UInt_t fNrHits = 0;

  /** @brief Total processing time [RealTime]. */
  Float_t fProcessTime = 0;

  ClassDef(CbmTaskTrdHitFinder, 1);
};
#endif
