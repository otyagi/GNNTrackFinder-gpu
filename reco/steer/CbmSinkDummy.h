/* Copyright (C) 2023-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmSinkDummy.cxx
/// \date   15.06.2024
/// \brief  A dummy FairSink class for the FairRunAna instance, if no output is really needed
/// \author S.Zharko <s.zharko@gsi.de>

#pragma once

#include <FairSink.h>

/// \class CbmSinkDummy
/// \brief A dummy sink class, which is to be passed to a FairRunAna, if no output is required
class CbmSinkDummy : public FairSink {
 public:
  /// \brief Default constructor
  CbmSinkDummy() = default;

  /// \brief Copy constructor
  CbmSinkDummy(const CbmSinkDummy&) = default;

  /// \brief Move constructor
  CbmSinkDummy(CbmSinkDummy&&) = default;

  /// \brief Destructor
  ~CbmSinkDummy() = default;

  /// \brief Copy assignment operator
  CbmSinkDummy& operator=(const CbmSinkDummy&) = default;

  /// \brief Copy assignment operator
  CbmSinkDummy& operator=(CbmSinkDummy&&) = default;

  // FairSink virtual function implementations
  Bool_t InitSink() { return kTRUE; }
  void Close() {}
  void Reset() {}
  Sink_Type GetSinkType() { return kFILESINK; }
  void SetOutTree(TTree*) {}
  void Fill() {}
  Int_t Write(const char*, Int_t, Int_t) { return 0; }
  void RegisterImpl(const char*, const char*, void*) {}
  void RegisterAny(const char*, const std::type_info&, const std::type_info&, void*) {}
  void WriteFolder() {}
  bool CreatePersistentBranchesAny() { return true; }
  void WriteObject(TObject*, const char*, Int_t) {}
  void WriteGeometry() {}  // TODO: if(gGeoManager) gGeoManager->Write();
  FairSink* CloneSink() { return new CbmSinkDummy(*this); }
};
