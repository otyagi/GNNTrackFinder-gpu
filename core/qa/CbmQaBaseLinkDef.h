/* Copyright (C) 2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer] */

#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclasses;
#pragma link C++ nestedtypedef;

// custom streamers are already implemented in the class body
#pragma link C++ class CbmQaCanvas - ;
#pragma link C++ class CbmQaPie - ;

// create streamers automatically
#pragma link C++ class cbm::algo::qa::H1D - ; 
#pragma link C++ class cbm::algo::qa::H2D - ;
#pragma link C++ class cbm::algo::qa::Prof1D - ; 
#pragma link C++ class cbm::algo::qa::Prof2D - ;
#pragma link C++ class cbm::qa::OnlineInterface + ;
#pragma link C++ class CbmQaEff + ;
#pragma link C++ class CbmQaPieSlice + ;
#pragma link C++ class CbmQaHist < TH1F> + ;
#pragma link C++ class CbmQaHist < TH1D> + ;
#pragma link C++ class CbmQaHist < TH1I> + ;
#pragma link C++ class CbmQaHist < TProfile> + ;
#pragma link C++ class CbmQaHist < TProfile2D> + ;
#pragma link C++ class CbmQaTable + ;
#pragma link C++ class CbmQaTask + ;
#pragma link C++ class CbmQaManager + ;

#pragma link C++ class cbm::qa::checker::Core + ;
#pragma link C++ class cbm::qa::checker::FileHandler + ;
#pragma link C++ class cbm::qa::checker::Hist1DHandler + ;
#pragma link C++ class cbm::qa::checker::Hist2DHandler + ;
#pragma link C++ class cbm::qa::checker::Profile1DHandler + ;
#pragma link C++ class cbm::qa::checker::ObjectHandler + ;
#pragma link C++ class cbm::qa::checker::ObjectDB + ;

#pragma link C++ class cbm::qa::report::Builder + ;
#pragma link C++ class cbm::qa::report::Element + ;
#pragma link C++ class cbm::qa::report::CollapsibleElement + ;
#pragma link C++ class cbm::qa::report::Engine + ;
#pragma link C++ class cbm::qa::report::Figure + ;
#pragma link C++ class cbm::qa::report::Header + ;
#pragma link C++ class cbm::qa::report::Section + ;
#pragma link C++ class cbm::qa::report::Table + ;
#pragma link C++ class cbm::qa::report::Tail + ;
#pragma link C++ class cbm::qa::report::LatexEngine + ;

#endif
