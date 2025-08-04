/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */


#ifndef __CLING__
#include "AnaFile.h"
#include "Femto1DCF.h"
#include "FemtoCorrFuncKt.h"

#include <TCanvas.h>
#include <TH1.h>
#endif


void show_hbt()
{
  Hal::AnaFile* f             = new Hal::AnaFile("test2.root");
  Hal::FemtoCorrFuncKt* cf_kt = (Hal::FemtoCorrFuncKt*) f->GetMainObject(2);
  TH1* h                      = cf_kt->GetKtNum();
  Hal::Femto1DCF* cf          = (Hal::Femto1DCF*) cf_kt->GetCF(0);

  TH1* mon = f->GetHistogramPassed(Hal::ECutUpdate::kTrack, 0, 3);

  TCanvas* c = new TCanvas();
  c->Divide(2, 2);
  c->cd(1);
  h->Draw();
  c->cd(2);
  mon->Draw("colz");
  c->cd(3);
  cf->Draw();
}
