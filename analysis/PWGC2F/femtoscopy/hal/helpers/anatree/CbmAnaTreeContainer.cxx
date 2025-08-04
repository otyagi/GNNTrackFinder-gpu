/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "CbmAnaTreeContainer.h"

#include "AnalysisTree/Matching.hpp"

#include <TChain.h>
#include <TFile.h>

#include <AnalysisTree/Configuration.hpp>

#include <Hal/Cout.h>

Bool_t CbmAnaTreeRecoSourceContainer::ConnectToTree(TChain* tree)
{
  fEvent     = new AnalysisTree::EventHeader();
  fVtxTracks = new AnalysisTree::TrackDetector();
  fTofHits   = new AnalysisTree::HitDetector();
  fVtx2Tof   = new AnalysisTree::Matching();
  fVtx2Mc    = new AnalysisTree::Matching();
  if (tree->GetBranch("VtxTracks.") == nullptr) {
    Hal::Cout::PrintInfo("Lack of VtxTracks in AT", Hal::EInfo::kError);
    return kFALSE;
  }
  if (tree->GetBranch("RecEventHeader.") == nullptr) {
    Hal::Cout::PrintInfo("Lack of RecEventHeader in AT", Hal::EInfo::kError);
    return kFALSE;
  }
  if (tree->GetBranch("TofHits.") == nullptr) {
    Hal::Cout::PrintInfo("Lack of TofHits in AT", Hal::EInfo::kError);
    return kFALSE;
  }
  if (tree->GetBranch("VtxTracks2TofHits.") == nullptr) {
    Hal::Cout::PrintInfo("Lack of VtxTracks2TofHits tracks in AT", Hal::EInfo::kError);
    return kFALSE;
  }
  tree->SetBranchAddress("VtxTracks.", &fVtxTracks);
  tree->SetBranchAddress("RecEventHeader.", &fEvent);
  tree->SetBranchAddress("TofHits.", &fTofHits);
  tree->SetBranchAddress("VtxTracks2TofHits.", &fVtx2Tof);
  tree->SetBranchStatus("VtxTracks.", 1);
  tree->SetBranchStatus("RecEventHeader.", 1);
  tree->SetBranchStatus("TofHits.", 1);
  tree->SetBranchStatus("VtxTracks2TofHits.", 1);
  if (tree->GetBranch("VtxTracks2SimParticles.")) {
    tree->SetBranchAddress("VtxTracks2SimParticles.", &fVtx2Mc);
    tree->SetBranchStatus("VtxTracks2SimParticles.", 1);
  }

  return kTRUE;
}

Bool_t CbmAnaTreeMcSourceContainer::ConnectToTree(TChain* tree)
{
  fEvent     = new AnalysisTree::EventHeader();
  fParticles = new AnalysisTree::Particles();
  if (tree->GetBranch("SimEventHeader.") == nullptr) return kFALSE;
  if (tree->GetBranch("SimParticles.") == nullptr) return kFALSE;
  tree->SetBranchAddress("SimEventHeader.", &fEvent);
  tree->SetBranchAddress("SimParticles.", &fParticles);
  tree->SetBranchStatus("SimEventHeader.", 1);
  tree->SetBranchStatus("SimParticles.", 1);
  return kTRUE;
}

void CbmAnaTreeRecoSourceContainer::LoadFields(TString file)
{
  TFile* oldFile     = gFile;
  TDirectory* oldDir = gDirectory;

  TFile* f                          = new TFile(file);
  AnalysisTree::Configuration* conf = (AnalysisTree::Configuration*) f->Get("Configuration");
  GetFieldIds().vtx_px              = conf->GetBranchConfig("VtxTracks").GetFieldId("px");
  GetFieldIds().vtx_py              = conf->GetBranchConfig("VtxTracks").GetFieldId("py");
  GetFieldIds().vtx_pz              = conf->GetBranchConfig("VtxTracks").GetFieldId("pz");
  GetFieldIds().vtx_dcax            = conf->GetBranchConfig("VtxTracks").GetFieldId("dcax");
  GetFieldIds().vtx_dcay            = conf->GetBranchConfig("VtxTracks").GetFieldId("dcay");
  GetFieldIds().vtx_dcaz            = conf->GetBranchConfig("VtxTracks").GetFieldId("dcaz");

  GetFieldIds().vtx_chi2    = conf->GetBranchConfig("VtxTracks").GetFieldId("chi2");
  GetFieldIds().vtx_vtxchi2 = conf->GetBranchConfig("VtxTracks").GetFieldId("vtx_chi2");
  GetFieldIds().vtx_q       = conf->GetBranchConfig("VtxTracks").GetFieldId("q");
  GetFieldIds().vtx_nhits   = conf->GetBranchConfig("VtxTracks").GetFieldId("nhits");
  GetFieldIds().vtx_mvdhits = conf->GetBranchConfig("VtxTracks").GetFieldId("nhits_mvd");


  GetFieldIds().tof_mass2 = conf->GetBranchConfig("TofHits").GetFieldId("mass2");

  GetFieldIds().vtx_x    = conf->GetBranchConfig("VtxTracks").GetFieldId("x");
  GetFieldIds().vtx_cx0  = conf->GetBranchConfig("VtxTracks").GetFieldId("cx0");
  GetFieldIds().vtx_cov1 = conf->GetBranchConfig("VtxTracks").GetFieldId("cov1");
  gFile                  = oldFile;
  gDirectory             = oldDir;

  f->Close();
  delete f;
}

void CbmAnaTreeMcSourceContainer::LoadFields(TString inFile)
{
  TFile* oldFile     = gFile;
  TDirectory* oldDir = gDirectory;

  TFile* f = new TFile(inFile);

  AnalysisTree::Configuration* conf = (AnalysisTree::Configuration*) f->Get("Configuration");

  GetFieldIds().px        = conf->GetBranchConfig("SimParticles").GetFieldId("px");
  GetFieldIds().py        = conf->GetBranchConfig("SimParticles").GetFieldId("py");
  GetFieldIds().pz        = conf->GetBranchConfig("SimParticles").GetFieldId("pz");
  GetFieldIds().mass      = conf->GetBranchConfig("SimParticles").GetFieldId("mass");
  GetFieldIds().pdg       = conf->GetBranchConfig("SimParticles").GetFieldId("pid");
  GetFieldIds().motherId  = conf->GetBranchConfig("SimParticles").GetFieldId("mother_id");
  GetFieldIds().event_b   = conf->GetBranchConfig("SimEventHeader").GetFieldId("b");
  GetFieldIds().event_psi = conf->GetBranchConfig("SimEventHeader").GetFieldId("psi_RP");
  GetFieldIds().freezX    = conf->GetBranchConfig("SimParticles").GetFieldId("xfreez");
  GetFieldIds().freezY    = conf->GetBranchConfig("SimParticles").GetFieldId("yfreez");
  GetFieldIds().freezZ    = conf->GetBranchConfig("SimParticles").GetFieldId("zfreez");
  GetFieldIds().freezT    = conf->GetBranchConfig("SimParticles").GetFieldId("tfreez");
  gFile                   = oldFile;
  gDirectory              = oldDir;

  f->Close();
  delete f;
}
