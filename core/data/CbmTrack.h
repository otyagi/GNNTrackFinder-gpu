/* Copyright (C) 2007-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig, Volker Friese [committer] */

// -------------------------------------------------------------------------
// -----                        CbmTrack header file                   -----
// -----                  Created 29/11/07  by V. Friese               -----
// -----                  Modified 26/05/09  by A. Lebedev             -----
// -------------------------------------------------------------------------

/**  CbmTrack.h
 *@author V.Friese <v.friese@gsi.de>
 **
 ** Base class for local tracks in the CBM detectors.
 ** Holds a list of CbmHits and the fitted track parameters.
 ** The fit parameters are of type FairTrackParam
 ** and can only be accessed and modified via this class.
 **/

#ifndef CBMTRACK_H_
#define CBMTRACK_H_ 1

#include "CbmHit.h"  // for HitType

#include <FairTrackParam.h>  // for FairTrackParam

#include <Rtypes.h>      // for THashConsistencyHolder, ClassDef
#include <RtypesCore.h>  // for Double32_t
#include <TObject.h>     // for TObject

#include <cstdint>
#include <string>  // for string
#include <vector>  // for vector

class CbmMatch;

class CbmTrack : public TObject {
public:
  /** Default constructor **/
  CbmTrack();

  /** Destructor **/
  virtual ~CbmTrack();

  /** Copy Constructor **/
  CbmTrack(const CbmTrack&);

  /** Assignment Operator **/
  CbmTrack& operator=(const CbmTrack&);

  /** Add a hit to the list, using index and HitType
	 * @param index Index of the hit in the array
	 * @param type Type of the hit to be added
	 **/
  void AddHit(int32_t index, HitType type);

  /** Accessors  **/
  virtual int32_t GetNofHits() const { return fHitIndex.size(); }
  int32_t GetHitIndex(int32_t iHit) const { return fHitIndex[iHit]; }
  HitType GetHitType(int32_t iHit) const { return fHitType[iHit]; }
  int32_t GetPidHypo() const { return fPidHypo; }
  int32_t GetFlag() const { return fFlag; }
  double GetChiSq() const { return fChiSq; }
  int32_t GetNDF() const { return fNDF; }
  double GetChiSqTime() const { return fChiSqTime; }
  int32_t GetNdfTime() const { return fNdfTime; }
  int32_t GetPreviousTrackId() const { return fPreviousTrackId; }
  const FairTrackParam* GetParamFirst() const { return &fParamFirst; }
  const FairTrackParam* GetParamLast() const { return &fParamLast; }
  CbmMatch* GetMatch() const { return fMatch; }
  double GetStartTime() const { return fStartTime; }
  double GetStartTimeError() const { return fStartTimeError; }
  double GetFirstHitTime() const { return fFirstHitTime; }
  double GetFirstHitTimeError() const { return fFirstHitTimeError; }
  double GetLastHitTime() const { return fLastHitTime; }
  double GetLastHitTimeError() const { return fLastHitTimeError; }

  /** Modifiers  **/
  void SetPidHypo(int32_t pid) { fPidHypo = pid; }
  void SetFlag(int32_t flag) { fFlag = flag; }
  void SetChiSq(double chiSq) { fChiSq = chiSq; }
  void SetNDF(int32_t ndf) { fNDF = ndf; }
  void SetChiSqTime(double chiSq) { fChiSqTime = chiSq; }
  void SetNdfTime(int32_t ndf) { fNdfTime = ndf; }
  void SetPreviousTrackId(int32_t previousTrackId) { fPreviousTrackId = previousTrackId; }
  void SetParamFirst(const FairTrackParam* par) { fParamFirst = *par; }
  void SetParamLast(const FairTrackParam* par) { fParamLast = *par; }
  void SetParamFirst(const FairTrackParam& par) { fParamFirst = par; }
  void SetParamLast(const FairTrackParam& par) { fParamLast = par; }
  void SetMatch(CbmMatch* match);
  void SetStartTime(double time) { fStartTime = time; }
  void SetStartTimeError(double error) { fStartTimeError = error; }
  void SetFirstHitTime(double time) { fFirstHitTime = time; }
  void SetFirstHitTimeError(double error) { fFirstHitTimeError = error; }
  void SetLastHitTime(double time) { fLastHitTime = time; }
  void SetLastHitTimeError(double error) { fLastHitTimeError = error; }

  virtual std::string ToString() const;

protected:
  /** Array contains the hit indices of the hits attached to the track **/
  std::vector<int32_t> fHitIndex;

  /** Array contains the hit types of the hits attached to the track **/
  std::vector<HitType> fHitType;

  /** PID hypothesis used by the track fitter **/
  int32_t fPidHypo;

  /** Track parameters at first and last fitted hit **/
  FairTrackParam fParamFirst;
  FairTrackParam fParamLast;

  /** Track start time **/
  double fStartTime;
  double fStartTimeError;

  /** Track time at the first hit **/
  double fFirstHitTime;
  double fFirstHitTimeError;

  /** Track time at the last hit **/
  double fLastHitTime;
  double fLastHitTimeError;

  /** Quality flag **/
  int32_t fFlag;

  /** Chi square and NDF of track fit **/
  Double32_t fChiSq;
  int32_t fNDF;

  /** Index of previous track segment **/
  int32_t fPreviousTrackId;

  /** Monte-Carlo information **/
  CbmMatch* fMatch;

  /** Chi square and NDF of track fit in time **/
  Double32_t fChiSqTime {0.};
  int32_t fNdfTime {0};

  ClassDef(CbmTrack, 6);
};

#endif
