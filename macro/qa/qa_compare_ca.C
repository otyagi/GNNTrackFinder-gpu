/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   qa_compare_ca.C
/// @author Sergei Zharko <s.zharko@gsi.de>
/// @since  22.06.2023
/// @brief  ROOT macro to run QA-Checker framework comparison of the old and new ROOT-file versions (CA).

/// @brief  Function to compare two different versions of ROOT-files
/// @param configName          Name of config containing file-object map
/// @param outputName          Name of the QA-Checker output (default is "QaCheckerResult.root")
/// @return  Result flag:
///          - 0: all objects are the same within defined comparison procedure (point-by-point for now)
///          - 1: some of the objects differ


/// NOTE: Disable clang formatting to keep easy parameters overview
/* clang-format off */
int qa_compare_ca( 
                const char* configName = "objects2.yaml",
                const char* outputName = "QACheckerOutput.root"  // TODO: Add tag of a merge request or commit
              )
/* clang-format on */
{
  // ----- Logger settings
  FairLogger::GetLogger()->SetLogScreenLevel("INFO");
  FairLogger::GetLogger()->SetColoredLog(true);

  //// ----- Style settings
  gStyle->SetPalette(kRainBow);

  //// ----- Configure QA-Checker
  auto pQaChecker = std::make_unique<cbm::qa::checker::Core>();
  pQaChecker->RegisterOutFile(outputName);  // Set name of the output file
  pQaChecker->SetFromYAML(configName);      // Read file-object map

  //// ----- Run comparision routine
  int res = pQaChecker->Process("UES");
  std::cout << "Macro finished successfully." << std::endl;
  return res;
}
