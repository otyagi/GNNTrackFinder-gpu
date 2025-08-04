/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmField.h"

#include <FairField.h>

HalCbmField::~HalCbmField() { fField = nullptr; }

HalCbmField::HalCbmField(FairField* f) : fField(f) {}

TVector3 HalCbmField::GetField(Double_t x, Double_t y, Double_t z) const
{
  return TVector3(fField->GetBx(x, y, z), fField->GetBy(x, y, z), fField->GetBz(x, y, z));
}

HalCbmField::HalCbmField(const HalCbmField& other)
{
  // TODO Auto-generated constructor stub
  fField = other.fField;
}
