/* Copyright (C) 2011-2014 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Andrey Lebedev [committer] */

/**
 * \file CbmLitClusteringQaStudyReport.cxx
 * \author Andrey Lebedev <andrey.lebedev@gsi.de>
 * \date 2011
 */
#include "CbmLitClusteringQaStudyReport.h"

#include "CbmHistManager.h"
#include "CbmReportElement.h"
#include "CbmUtils.h"
#include "TH1.h"

#include <boost/assign/list_of.hpp>

#include <vector>
using boost::assign::list_of;
using Cbm::NumberToString;
using Cbm::Split;
using std::endl;
using std::vector;

CbmLitClusteringQaStudyReport::CbmLitClusteringQaStudyReport() : CbmStudyReport() { SetReportName("clustering_qa"); }

CbmLitClusteringQaStudyReport::~CbmLitClusteringQaStudyReport() {}

void CbmLitClusteringQaStudyReport::Create()
{
  Out().precision(3);
  Out() << R()->DocumentBegin();
  Out() << R()->Title(0, GetTitle());

  PrintNofObjects();

  // Out() << PrintImages(".*clustering_qa_.*png");

  Out() << R()->DocumentEnd();
}

string CbmLitClusteringQaStudyReport::PrintNofObjects() const
{
  Int_t nofStudies = HM().size();
  vector<vector<TH1*>> histos(nofStudies);
  for (Int_t i = 0; i < nofStudies; i++) {
    histos[i] = HM(i)->H1Vector("hno_NofObjects_.+_Event");
  }
  string str = R()->TableBegin("Number of objects", list_of(string("")).range(GetStudyNames()));
  for (UInt_t iHist = 0; iHist < histos[0].size(); iHist++) {
    string cellName = Split(histos[0][iHist]->GetName(), '_')[2];
    vector<string> cells(nofStudies);
    for (Int_t i = 0; i < nofStudies; i++) {
      cells[i] = NumberToString<Double_t>(histos[i][iHist]->GetMean());
    }
    str += R()->TableRow(list_of(cellName).range(cells));
  }
  str += R()->TableEnd();
  return str;
}

void CbmLitClusteringQaStudyReport::Draw() {}

ClassImp(CbmLitClusteringQaStudyReport)
