/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportLinkDef.h
/// \brief  Linkage definitions for using in ROOT
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  24.02.2024

#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclasses;
#pragma link C++ nestedtypedef;

#pragma link C++ class cbm::qa::report::Builder + ;
#pragma link C++ class cbm::qa::report::Element + ;
#pragma link C++ class cbm::qa::report::CollapsibleElement + ;
#pragma link C++ class cbm::qa::report::ElementFactory + ;
#pragma link C++ class cbm::qa::report::Figure + ;
#pragma link C++ class cbm::qa::report::Header + ;
#pragma link C++ class cbm::qa::report::Section + ;
#pragma link C++ class cbm::qa::report::Table + ;
#pragma link C++ class cbm::qa::report::Tail + ;
#pragma link C++ class cbm::qa::report::LatexFactory + ;
#pragma link C++ class cbm::qa::report::LatexFormat + ;
#pragma link C++ class cbm::qa::report::LatexFigure + ;
#pragma link C++ class cbm::qa::report::LatexHeader + ;
#pragma link C++ class cbm::qa::report::LatexSection + ;
#pragma link C++ class cbm::qa::report::LatexTable + ;
#pragma link C++ class cbm::qa::report::LatexTail + ;


#endif

