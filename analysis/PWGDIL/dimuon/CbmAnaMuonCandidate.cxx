/* Copyright (C) 2009-2020 Petersburg Nuclear Physics Institute named by B.P.Konstantinov of National Research Centre "Kurchatov Institute", Gatchina
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Anna Senger, Evgeny Kryshen [committer] */

//----------------------------------------
//
// 2019 A. Senger a.senger@gsi.de
//
//----------------------------------------

#include "CbmAnaMuonCandidate.h"

CbmAnaMuonCandidate::CbmAnaMuonCandidate()
  : TObject()
  , track()
  , fMom()
  , fNStsHits(4)
  , fNMuchHits(4)
  , fNTrdHits(0)
  , fNTofHits(0)
  , fChiToVertex(100)
  , fSign(0)
  , fChiMuch(100)
  , fChiSts(100)
  , fChiTrd(100)
  , fChiGlobal(100)
  , fMu(0)
  , fPdg(13)
  , fId(-1)
  , fM(1e8)
{
}

ClassImp(CbmAnaMuonCandidate);
