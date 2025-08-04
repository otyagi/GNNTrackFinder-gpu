/* Copyright (C) 2020-2021 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Viktor Klochkov [committer] */

#ifndef ANALYSIS_TREE_SIMEVENTHEADERCONVERTER_H_
#define ANALYSIS_TREE_SIMEVENTHEADERCONVERTER_H_

#include "CbmConverterTask.h"
#include "CbmMCEventList.h"

#include "AnalysisTree/EventHeader.hpp"

class FairMCEventHeader;
class CbmVertex;
class CbmMCDataManager;
class CbmMCDataArray;
class CbmMCDataObject;

class CbmSimEventHeaderConverter final : public CbmConverterTask {
public:
  explicit CbmSimEventHeaderConverter(std::string out_branch_name) : CbmConverterTask(std::move(out_branch_name)) {};
  ~CbmSimEventHeaderConverter() final = default;

  void Init() final;
  void ProcessData(CbmEvent* event) final;
  void Finish() final { delete sim_event_header_; };

private:
  AnalysisTree::EventHeader* sim_event_header_ {nullptr};
  FairMCEventHeader* cbm_header_ {nullptr};
  CbmMCDataManager* cbm_mc_manager_ {nullptr};
  CbmMCDataObject* cbm_header_obj_ {nullptr};
  CbmMCEventList* cbm_mc_event_list_ {nullptr};

  int ipsi_RP_ {AnalysisTree::UndefValueInt};
  int ib_ {AnalysisTree::UndefValueInt};
  int istart_time_ {AnalysisTree::UndefValueInt};
  int iend_time_ {AnalysisTree::UndefValueInt};
  int irun_id_ {AnalysisTree::UndefValueInt};
  int ievent_id_ {AnalysisTree::UndefValueInt};
  int iT0_{AnalysisTree::UndefValueInt};

  ClassDef(CbmSimEventHeaderConverter, 1)
};

#endif  // ANALYSIS_TREE_SIMEVENTHEADERCONVERTER_H_
