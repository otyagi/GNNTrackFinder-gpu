/* Copyright (C) 2006-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Claudia Hoehne, Andrey Lebedev, Denis Bertini [committer], Semen Lebedev */

/**
 * \file CbmRichTrackExtrapolationKF.h
 *
 * \brief "TrackExtrapolation" from STS tracks (Kalman Fitter)
 * It reads the track array form STS and extrapolates those to
 * be projected to the Rich Photodetector to some z-Plane in RICH
 *
 * \author Claudia Hoehne
 * \date 206
 **/

#ifndef CBM_RICH_TRACK_EXTRAPOLATION_KF
#define CBM_RICH_TRACK_EXTRAPOLATION_KF

#include "CbmRichTrackExtrapolationBase.h"

class TClonesArray;

/**
 * \class CbmRichTrackExtrapolationKF
 *
 * \brief "TrackExtrapolation" from STS tracks (Kalman Fitter)
 * It reads the track array form STS and extrapolates those to
 * be projected to the Rich Photodetector to some z-Plane in RICH
 *
 * \author Claudia Hoehne
 * \date 206
 **/
class CbmRichTrackExtrapolationKF : public CbmRichTrackExtrapolationBase {
 public:
  /**
     * \brief Default constructor.
     */
  CbmRichTrackExtrapolationKF();

  /**
     * \brief Destructor.
     */
  virtual ~CbmRichTrackExtrapolationKF();

  /**
     * \brief Inherited from CbmRichTrackExtrapolationBase.
     */
  virtual void Init();

  /**
     * \brief Inherited from CbmRichTrackExtrapolationBase.
     */
  virtual void DoExtrapolation(CbmEvent* event, TClonesArray* globalTracks, TClonesArray* extrapolatedTrackParams,
                               double z);

 private:
  TClonesArray* fStsTracks = nullptr;

 private:
  /**
     * \brief Copy constructor.
     */
  CbmRichTrackExtrapolationKF(const CbmRichTrackExtrapolationKF&);

  /**
     * \brief Assignment operator.
     */
  void operator=(const CbmRichTrackExtrapolationKF&);
};

#endif
