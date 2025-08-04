/* Copyright (C) 2006-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Alex Bercuci, Ivan Kisel, Denis Bertini [committer] */

/** The CbmKFPrimaryVertexFinder class
 *
 * @author  S.Gorbunov, I.Kisel
 * @version 1.0
 * @since   06.02.06
 * 
 * Class to find primary vertex with the Kalman Filter method
 *
 */
#ifndef CBMKFPRIMARYVERTEXFINDER_H
#define CBMKFPRIMARYVERTEXFINDER_H

#include "CbmKFTrackInterface.h"
#include "CbmKFVertexInterface.h"

#include <tuple>
#include <vector>

class CbmKFPrimaryVertexFinder : public TObject {

  std::vector<std::tuple<CbmKFTrackInterface*, int32_t, bool>> Tracks;

 public:
  CbmKFPrimaryVertexFinder() : Tracks() { Clear(); };
  ~CbmKFPrimaryVertexFinder(){};

  virtual void Clear(Option_t* opt = "");
  void AddTrack(CbmKFTrackInterface* Track, int32_t idx = -1);
  void SetTracks(std::vector<CbmKFTrackInterface*>& vTracks);

  /** Return the list of indices og global tracks used for PV fit, if they were provided by the user */
  int GetUsedTracks(std::vector<uint32_t>& idx) const;
  void Fit(CbmKFVertexInterface& vtx);

  ClassDef(CbmKFPrimaryVertexFinder, 2);
};
#endif /* !CBMKFPRIMARYVERTEXFINDER_H */
