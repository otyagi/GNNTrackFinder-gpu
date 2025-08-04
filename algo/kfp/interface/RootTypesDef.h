/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   RootTypesDef.h
/// \date   11.02.2025
/// \brief  A compatibility header for the KFParticle code
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#if __has_include(<RtypesCore.h>)
#include <RtypesCore.h>
#else
using Bool_t  = bool;
using Int_t   = int;
using Float_t = float;
#endif
