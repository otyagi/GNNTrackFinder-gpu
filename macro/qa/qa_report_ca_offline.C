/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   qa_report_ca_offline.C
/// \brief  QA report for CA tracking (offline)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  18.04.2024

/* clang-format off */
void qa_report_ca_offline(
  TString qaFile = "",
  TString setup  = "",
  TString report = "./report"
)
/* clang-format on */
{
  using cbm::qa::report::Builder;
  using cbm::qa::report::Figure;
  using cbm::qa::report::Section;
  using cbm::qa::report::Table;

  if (qaFile.Length() == 0) {
    return;
  }

  TFile* pFile = TFile::Open(qaFile.Data(), "READONLY");
  if (!pFile->IsOpen()) {
    std::cerr << "- E: Input file \"" << qaFile << "\" cannot be opened\n";
  }

  // Report header
  auto pReport = std::make_unique<Builder>("CA Tracking QA Status", report.Data());
  pReport->GetHeader()->SetSubtitle("The CBM Collaboration");
  pReport->GetHeader()->SetAuthor(gSystem->Getenv("USER"));
  pReport->GetHeader()->SetSetup(setup.Data());
  pReport->SetFigureExtention("pdf");

  // ----- Setup
  {
    std::string sTaskName = "CbmCaInputQaSetup";

    auto pSection = std::make_shared<Section>(sTaskName, "CA Tracking Setup");
    pReport->AddSection(pSection);
    {
      auto* pCanv = pFile->Get<TCanvas>((sTaskName + "/Summary/c_setup_hits").c_str());
      auto pFig   = std::make_shared<Figure>("setup");
      pFig->AddPlot(pReport->SaveCanvas(pCanv, sTaskName + "/setup_hits"));
      pSection->Add(pFig);
    }
  }

  // ----- Input QA
  {
    std::vector<std::pair<std::string, std::string>> vDetNames = {{"Mvd", "MVD"},
                                                                  {"Sts", "STS"},
                                                                  {"Much", "MuCh"},
                                                                  {"Trd", "TRD"},
                                                                  {"Tof", "TOF"}};
    for (const auto& [detTag, detName] : vDetNames) {
      std::string sTaskName = Form("CbmCaInputQa%s", detTag.c_str());
      if (pFile->Get(sTaskName.c_str())) {
        int nStations = 0;
        while (pFile->Get(Form("%s/Station %d", sTaskName.c_str(), nStations))) {
          ++nStations;
        }
        std::string sectionName = Form("CA Tracking Input QA for %s", detName.c_str());
        auto pSection           = std::make_shared<Section>(sTaskName, sectionName);
        pReport->AddSection(pSection);

        // Occupancy
        {
          std::string secLabel = Form("%s:occupancy", sTaskName.c_str());
          auto pSubSection     = std::make_shared<Section>(secLabel, "Hits and MC Points occupancy");
          pSection->Add(pSubSection);
          std::vector<std::pair<std::string, std::string>> vFigAtts = {
            {"hit_xy", Form("%s hit occupancy in xy-plane.", detName.c_str())},
            {"hit_zx", Form("%s hit occupancy in xz-plane.", detName.c_str())},
            {"hit_zy", Form("%s hit occupancy in yz-plane.", detName.c_str())},
            {"point_xy", Form("%s MC-point occupancy in xy-plane.", detName.c_str())},
            {"point_zx", Form("%s MC-point occupancy in xz-plane.", detName.c_str())},
            {"point_zy", Form("%s MC-point occupancy in yz-plane.", detName.c_str())}};
          for (const auto& [name, caption] : vFigAtts) {
            auto* pCanv       = pFile->Get<TCanvas>((sTaskName + "/Summary/vs Station/" + name).c_str());
            std::string label = Form("%s:%s", sTaskName.c_str(), name.c_str());
            auto pFig         = std::make_shared<Figure>(label);
            pFig->AddPlot(pReport->SaveCanvas(pCanv, sTaskName + "/" + name));
            pFig->SetCaption(caption);
            pSubSection->Add(pFig);
          }
        }
        // Efficiencies
        {
          std::string secLabel = Form("%s:efficiency", sTaskName.c_str());
          auto pSubSection     = std::make_shared<Section>(secLabel, "Detector efficiency");
          pSection->Add(pSubSection);

          {
            std::string name    = "reco_eff_vs_xy";
            std::string caption = "Hit efficency in xy-plane.";
            std::string label   = Form("%s:%s", sTaskName.c_str(), name.c_str());
            auto* pCanv         = pFile->Get<TCanvas>((sTaskName + "/Summary/vs Station/" + name).c_str());
            auto pFig           = std::make_shared<Figure>(label);
            pFig->AddPlot(pReport->SaveCanvas(pCanv, sTaskName + "/" + name));
            pFig->SetCaption(caption);
            pSubSection->Add(pFig);
          }

          {
            std::string name  = "eff_table";
            std::string label = Form("%s:%s", sTaskName.c_str(), name.c_str());
            auto* pQaTable    = pFile->Get<CbmQaTable>((sTaskName + "/Summary/vs Station/" + name).c_str());
            auto pTable       = std::make_shared<Table>(label);
            pTable->Set(pQaTable);
            pSubSection->Add(pTable);
          }
        }
        // Residuals and pulls
        {
          std::string secLabel = Form("%s:pulls", sTaskName.c_str());
          auto pSubSection     = std::make_shared<Section>(secLabel, "Residuals and pulls");
          pSection->Add(pSubSection);

          std::vector<std::pair<std::string, std::string>> vFigAtts = {
            {"res_x", Form("Hit residuals for hit x-coordinate in %s", detName.c_str())},
            {"res_y", Form("Hit residuals for hit y-coordinate in %s", detName.c_str())},
            {"res_t", Form("Hit residuals for hit time in %s", detName.c_str())},
            {"pull_x", Form("Hit pulls for hit x-coordinate in %s", detName.c_str())},
            {"pull_y", Form("Hit pulls for hit y-coordinate in %s", detName.c_str())},
            {"pull_t", Form("Hit pulls for hit time in %s", detName.c_str())}};
          for (const auto& [name, caption] : vFigAtts) {
            auto* pCanv       = pFile->Get<TCanvas>((sTaskName + "/Summary/vs Station/" + name).c_str());
            std::string label = Form("%s:%s", sTaskName.c_str(), name.c_str());
            auto pFig         = std::make_shared<Figure>(label);
            pFig->AddPlot(pReport->SaveCanvas(pCanv, sTaskName + "/" + name));
            pFig->SetCaption(caption);
            pSubSection->Add(pFig);
          }

          for (const std::string& name : {"residuals_mean", "pulls_rms"}) {
            std::string label = Form("%s:%s", sTaskName.c_str(), name.c_str());
            auto* pQaTable    = pFile->Get<CbmQaTable>((sTaskName + "/Summary/vs Station/" + name).c_str());
            auto pTable       = std::make_shared<Table>(label);
            pTable->Set(pQaTable);
            pSubSection->Add(pTable);
          }
        }
      }
    }
  }

  // ----- Output QA
  if (pFile->Get("CbmCaOutputQa")) {
    std::string sTaskName = "CbmCaOutputQa";
    auto pSection         = std::make_shared<Section>(sTaskName, "CA Tracking Output QA");
    pReport->AddSection(pSection);

    // Tracking summary table
    {
      auto pSubSection = std::make_shared<Section>("ca::Summary", "Tracking summary");
      pSection->Add(pSubSection);
      auto* pSummaryTable = pFile->Get<CbmQaTable>((sTaskName + "/Summary/summary_table").c_str());
      auto pTable         = std::make_shared<Table>("casummary");
      pTable->Set(pSummaryTable, "{:.04}|{:.02}|{:.02}|{:.02}|{:.02}|{:.02}|{:.02}|{:.02}");
      pSubSection->Add(pTable);
    }
    // Track distributions
    {
      auto pSubSection = std::make_shared<Section>("ca::trkDistr", "Track Distributions");
      pSection->Add(pSubSection);
      std::vector<std::pair<std::string, std::string>> vFigAtts = {
        {"reco_eta", "Pseudorapidity distributions of reconstructed tracks for different track groups."},
        {"reco_etaMC", "MC pseudorapidity distributions of reconstructed tracks for different track groups."},
        {"reco_pMC", "MC momentum distributions of reconstructed tracks for different track groups."},
        {"reco_yMC", "MC rapidity distributions of reconstructed tracks for different track groups."},
        {"mc_pMC", "MC momentum distributions of MC-tracks for different track groups."},
        {"mc_yMC", "MC rapidity distributions of MC-tracks for different track groups."},
        {"mc_ptMC_yMC", "MC tracks vs. MC transverse momentum and MC rapidity."}};
      for (const auto& [name, caption] : vFigAtts) {
        if (auto* pCanv = pFile->Get<TCanvas>((sTaskName + "/Summary/" + name).c_str())) {
          auto pFig = std::make_shared<Figure>(name);
          pFig->AddPlot(pReport->SaveCanvas(pCanv, sTaskName + "/" + name));
          pFig->SetCaption(caption);
          pSubSection->Add(pFig);
        }
      }
    }
    // Track distributions
    {
      auto pSubSection = std::make_shared<Section>("ca::Eff", "Tracking Efficiency");
      pSection->Add(pSubSection);
      std::vector<std::pair<std::string, std::string>> vFigAtts = {
        {"eff_pMC", "Tracking efficiency vs. MC momentum for different track groups."},
        {"eff_yMC", "Tracking efficiency vs. MC rapidity for different track groups."},
        {"eff_thetaMC", "Tracking efficiency vs. MC theta for different track groups."},
        {"eff_phiMC", "Tracking efficiency vs. MC azimuthal angle for different track groups."},
        {"eff_etaMC", "Tracking efficiency vs. MC pseudorapidity for different track groups."}};
      for (const auto& [name, caption] : vFigAtts) {
        if (auto* pCanv = pFile->Get<TCanvas>((sTaskName + "/Summary/" + name).c_str())) {
          auto pFig = std::make_shared<Figure>(name);
          pFig->AddPlot(pReport->SaveCanvas(pCanv, sTaskName + "/" + name));
          pFig->SetCaption(caption);
          pSubSection->Add(pFig);
        }
      }
    }
  }

  cbm::qa::report::BeamerEngine latex;
  pReport->CreateScript(latex);
}
