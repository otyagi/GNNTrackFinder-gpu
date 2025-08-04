/* Copyright (C) 2019-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Valentina Akishina, Sergey Gorbunov[committer] */

#include "CaTriplet.h"

#include <sstream>
#include <string>

std::string cbm::algo::ca::Triplet::ToString(int indentLevel) const
{
  /// print the triplet parameters
  std::stringstream ss{};
  constexpr char indentChar = '\t';
  std::string indent(indentLevel, indentChar);

  ss << indent << "Triplet: station L/M/R " << GetLSta() << "/" << GetMSta() << "/" << GetRSta() << "\n"
     << indent << "          hit L/M/R " << fHitL << "/" << fHitM << "/" << fHitR << "\n"
     << indent << "          level " << fLevel << " first neighbor " << fFirstNeighbour << " Nneighbors "
     << fNneighbours << "\n"
     << indent << "          qp " << fQp << " Cqp " << fCqp << " chi2 " << fChi2 << "\n"
     << indent << "          tx " << fTx << " Ctx " << fCtx << " ty " << fTy << " Cty " << fCty << std::endl;

  return ss.str();
}
