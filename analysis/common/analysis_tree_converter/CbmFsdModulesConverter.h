/* Copyright (C) 2020-2021 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Viktor Klochkov [committer] */

#ifndef ANALYSIS_TREE_FSDMODULESCONVERTER_H_
#define ANALYSIS_TREE_FSDMODULESCONVERTER_H_

#include "AnalysisTree/Detector.hpp"
#include "CbmConverterTask.h"

class TClonesArray;

class CbmFsdModulesConverter final : public CbmConverterTask {
 public:
  explicit CbmFsdModulesConverter(std::string out_branch_name, std::string match_to = "")
    : CbmConverterTask(std::move(out_branch_name), std::move(match_to)){};

  ~CbmFsdModulesConverter() final;

  void Init() final;
  void ProcessData(CbmEvent* event) final;
  void Finish() final;

 private:
  AnalysisTree::ModuleDetector* fsd_modules_{nullptr};
  TClonesArray* cbm_fsd_hits_{nullptr};

  ClassDef(CbmFsdModulesConverter, 1)
};

#endif  // ANALYSIS_TREE_FSDMODULESCONVERTER_H_
