/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmCaIdealHitProducer.h
/// @brief  Base class for the ideal hit producer task (implementation)
/// @author S.Zharko
/// @since  01.06.2023

#include "CbmCaIdealHitProducerDet.h"

//templateClassImp(cbm::ca::IdealHitProducerDet);

template class cbm::ca::IdealHitProducerDet<L1DetectorID::kMvd>;
template class cbm::ca::IdealHitProducerDet<L1DetectorID::kSts>;
template class cbm::ca::IdealHitProducerDet<L1DetectorID::kMuch>;
template class cbm::ca::IdealHitProducerDet<L1DetectorID::kTrd>;
template class cbm::ca::IdealHitProducerDet<L1DetectorID::kTof>;
