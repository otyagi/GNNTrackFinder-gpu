/* Copyright (C) 2020-2021 Physikalisches Institut, Eberhard Karls Universität Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Viktor Klochkov [committer] */

#ifndef ANALYSIS_TREE_CONVERTERTASK_H_
#define ANALYSIS_TREE_CONVERTERTASK_H_

#include <CbmLink.h>

#include <map>
#include <string>

#include "AnalysisTree/Task.hpp"

class FairRootManager;
class CbmEvent;

class CbmConverterTask : public AnalysisTree::Task {
  using MapType = std::map<int, int>;

public:
  CbmConverterTask() = default;
  explicit CbmConverterTask(std::string out_branch_name, std::string match_to = "")
  {
    out_branch_ = std::move(out_branch_name);
    match_to_   = std::move(match_to);
  };

  ~CbmConverterTask() override = default;

  virtual void ProcessData(CbmEvent* event) = 0;

  void Exec() final { throw std::runtime_error("Should not be used!"); };

  const MapType& GetOutIndexesMap() const { return out_indexes_map_; }

  void SetIndexesMap(std::map<std::string, MapType>* indexes_map) { indexes_map_ = indexes_map; }

  const std::string& GetOutputBranchName() const { return out_branch_; }


protected:
  MapType out_indexes_map_ {};  ///< CbmRoot to AnalysisTree indexes map for output branch
  std::string out_branch_ {};
  std::map<std::string, MapType>* indexes_map_ {};  ///< CbmRoot to AnalysisTree indexes map for branches
  ///< from other tasks
  std::string match_to_ {};  ///< AT branch to match
};

#endif  // ANALYSIS_TREE_CONVERTERTASK_H_
