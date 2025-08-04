/* Copyright (C) 2013-2023 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Andrey Lebedev [committer], Volker Friese */

/**
 * \file CbmMatchRecoToMC.h
 * \brief FairTask for matching RECO data to MC.
 * \author Andrey Lebedev <andrey.lebedev@gsi.de>
 * \date 2013
 */

#ifndef CBMMATCHRECOTOMC_H_
#define CBMMATCHRECOTOMC_H_

#include "CbmDefs.h"  // for ECbmModuleId
#include "CbmMatch.h"
#include "CbmTofDigi.h"

#include <FairTask.h>  // for FairTask, InitStatus

#include <Rtypes.h>      // for THashConsistencyHolder, ClassDef
#include <RtypesCore.h>  // for Int_t, Bool_t, Option_t, kFALSE, kTRUE

#include <utility>  // for pair
#include <vector>   // for vector

class CbmDigiManager;
class CbmMCDataArray;
class CbmRichHit;
class TClonesArray;

class CbmMatchRecoToMC : public FairTask {
 public:
  /**
    * \brief Constructor.
    */
  CbmMatchRecoToMC();

  /**
    * \brief Destructor.
    */
  virtual ~CbmMatchRecoToMC();

  /**
     * \brief Derived from FairTask.
     */
  virtual InitStatus Init();

  /**
     * \brief Derived from FairTask.
     */
  virtual void Exec(Option_t* opt);

  /**
     * \brief Derived from FairTask.
     */
  virtual void Finish();

 private:
  /**
     * \brief Read and create data branches.
     */
  void ReadAndCreateDataBranches();


  /** @brief Generic creation of cluster match objects
     ** @param digiMatches  Array of match objects for digis
     ** @param clusters     Arrays of clusters
     ** @param clusterMatches  Array of match objects for clusters
     **
     ** The cluster match objects are created by summing up the match objects
     ** of all digis belonging to the cluster.
     **/
  void MatchClusters(const TClonesArray* digiMatches, const TClonesArray* clusters, TClonesArray* clusterMatches);


  /** @brief Generic creation of cluster match objects, using CbmDigiManager
     ** @param clusters     Arrays of clusters
     ** @param clusterMatches  Array of match objects for clusters
     **
     ** The cluster match objects are created by summing up the match objects
     ** of all digis belonging to the cluster.
     **/
  void MatchClusters(ECbmModuleId systemId, const TClonesArray* clusters, TClonesArray* clusterMatches);


  void MatchHits(const TClonesArray* matches, const TClonesArray* hits, TClonesArray* hitMatches);

  /** @brief Match STS hits, using cluster match objects
     ** @param clusterMatches   TClonesArray with cluster matches
     ** @param hits             TClonesArray with CbmStsHit
     ** @param hitMatches       TClonesArray with hit matches (to be filled)
     **
     ** Since a StsHit is constructed from two StsClusters (from front and back
     ** side of a sensor), its match object must also be constructed from the two
     ** match objects corresponding to the clusters. This makes it different from
     ** the method MatchHits, which just copies the cluster match object to the
     ** hit match object.
     */
  void MatchHitsSts(const TClonesArray* clusterMmatches, const TClonesArray* hits, TClonesArray* hitMatches);

  void MatchHitsMvd(const TClonesArray* hits, TClonesArray* hitMatches);

  void MatchHitsFsd(const TClonesArray* hits, TClonesArray* hitMatches);

  void MatchHitsTof(const TClonesArray* HitDigiMatches, const TClonesArray* hits, TClonesArray* hitMatches);

  void MatchHitsToPoints(CbmMCDataArray* points, const TClonesArray* hits, TClonesArray* hitMatches);

  void MatchTracks(const TClonesArray* hitMatches, CbmMCDataArray* points, const TClonesArray* tracks,
                   TClonesArray* trackMatches);

  //Special case for STS: now evbased compatible
  void MatchStsTracks(const TClonesArray* mvdHitMatches, const TClonesArray* stsHitMatches, CbmMCDataArray* mvdPoints,
                      CbmMCDataArray* stsPoints, const TClonesArray* tracks, TClonesArray* trackMatches);

  void MatchRichRings(const TClonesArray* richRings, const TClonesArray* richHits, CbmMCDataArray* richMcPoints,
                      CbmMCDataArray* mcTracks, TClonesArray* ringMatches);

 public:
  /**
    * \brief Get CbmLinks of Mother MC Tracks for RICH hit
    */
  static std::vector<CbmLink> GetMcTrackMotherIdsForRichHit(CbmDigiManager* digiMan, const CbmRichHit* hit,
                                                            CbmMCDataArray* richPoints, CbmMCDataArray* mcTracks);
  /**
    * \brief Get [EventId, MCTrackId] pair of Mother MC Tracks for RICH hit.
    * This matching is only for TimeBased simulation
    */
  [[deprecated]] static std::vector<std::pair<Int_t, Int_t>>
  GetMcTrackMotherIdsForRichHit(CbmDigiManager* digiMan, const CbmRichHit* hit, CbmMCDataArray* richPoints,
                                CbmMCDataArray* mcTracks, Int_t eventNumber);

  /**
    * \brief Get MCTrackId of Mother MC Tracks for RICH hit.
    * This matching is only for EventBased simulation
    */
  [[deprecated]] static std::vector<Int_t> GetMcTrackMotherIdsForRichHit(CbmDigiManager* digiMan, const CbmRichHit* hit,
                                                                         const TClonesArray* richPoints,
                                                                         const TClonesArray* mcTracks);

  /**
   ** \brief  Suppresses cluster and hit matching procedures
   **
   ** The function sets a flag, which suppresses the cluster and hit matching procedures for MVD, STS, MuCh, TRD and 
   ** TOF, if and only if all the corresponding match branches are presented in the simulation tree. If at least one
   ** of the branch is absent (and the corresponding cluster and hit branches are present), the flag will be set to 
   ** false, and the cluster/hit matching will be executed.
   **/
  void SuppressHitReMatching() { fbSuppressHitReMatching = true; }

 private:
  static Int_t fEventNumber;

  Bool_t fIsMvdActive          = kTRUE;  // is the Mvd module active
  Bool_t fbDigiExpUsed         = kTRUE;  // Usage of CbmTofDigiExp instead of CbmTofDigi
  bool fbSuppressHitReMatching = false;  // Suppression of MC->hit matching, if the matches are already there

  CbmMCDataArray* fMCTracks    = nullptr;  //! Monte-Carlo tracks
  CbmDigiManager* fDigiManager = nullptr;  //! Interface to digi branches

  // MVD
  CbmMCDataArray* fMvdPoints       = nullptr;  //! MC points [in]
  TClonesArray* fMvdCluster        = nullptr;  //! Clusters [in]
  TClonesArray* fMvdHits           = nullptr;  //! Hits [in]
  TClonesArray* fMvdClusterMatches = nullptr;  //! Cluster matches [out]
  TClonesArray* fMvdHitMatches     = nullptr;  //! Hit matches [out]

  // STS
  CbmMCDataArray* fStsPoints       = nullptr;  //! MC points [in]
  TClonesArray* fStsClusters       = nullptr;  //! Clusters [in]
  TClonesArray* fStsHits           = nullptr;  //! Hits [in]
  TClonesArray* fStsTracks         = nullptr;  //! Tracks [in]
  TClonesArray* fStsClusterMatches = nullptr;  //! Cluster matches [out]
  TClonesArray* fStsHitMatches     = nullptr;  //! Hit matches [out]
  TClonesArray* fStsTrackMatches   = nullptr;  //! Track matches [out]

  // RICH
  CbmMCDataArray* fRichMcPoints   = nullptr;  //! MC points [in]
  TClonesArray* fRichHits         = nullptr;  //! Hits [in]
  TClonesArray* fRichRings        = nullptr;  //! Rings [in]
  TClonesArray* fRichTrackMatches = nullptr;  //! Match Ring -> MC track [out]

  // MUCH
  CbmMCDataArray* fMuchPoints        = nullptr;  //! MC points [in]
  TClonesArray* fMuchClusters        = nullptr;  //! Clusters [in]
  TClonesArray* fMuchPixelHits       = nullptr;  //! Hits [in]
  TClonesArray* fMuchTracks          = nullptr;  //! Tracks [in]
  TClonesArray* fMuchClusterMatches  = nullptr;  //! Cluster matches [out]
  TClonesArray* fMuchPixelHitMatches = nullptr;  //! Hit matches [out]
  TClonesArray* fMuchTrackMatches    = nullptr;  //! Track matches [out]

  // TRD
  CbmMCDataArray* fTrdPoints       = nullptr;  //! MC points [in]
  TClonesArray* fTrdClusters       = nullptr;  //! Clusters [in]
  TClonesArray* fTrdHits           = nullptr;  //! Hits [in]
  TClonesArray* fTrdTracks         = nullptr;  //! Tracks [in]
  TClonesArray* fTrdClusterMatches = nullptr;  //! Cluster matches [out]
  TClonesArray* fTrdHitMatches     = nullptr;  //! Hit matches [out]
  TClonesArray* fTrdTrackMatches   = nullptr;  //! Track matches [out]

  // TOF
  CbmMCDataArray* fTofPoints                 = nullptr;  //! CbmTofPoint array
  const std::vector<CbmTofDigi>* fTofDigis   = nullptr;  // TOF MC point matches
  const std::vector<CbmMatch>* fTofDigiMatch = nullptr;  // TOF MC point matches
  TClonesArray* fTofHits                     = nullptr;  //! CbmTofHit array
  TClonesArray* fTofHitDigiMatches           = nullptr;  //! Match Hit -> Digi [out]
  TClonesArray* fTofHitMatches               = nullptr;  //! Match Hit -> MC point [out]

  // FSD
  CbmMCDataArray* fFsdPoints   = nullptr;  //! MC points [in]
  TClonesArray* fFsdHits       = nullptr;  //! Hits [in]
  TClonesArray* fFsdHitMatches = nullptr;  //! Hit matches [out]

  CbmMatchRecoToMC(const CbmMatchRecoToMC&);
  CbmMatchRecoToMC& operator=(const CbmMatchRecoToMC&);

  ClassDef(CbmMatchRecoToMC, 1);
};

#endif /* CBMMATCHRECOTOMC_H_ */
