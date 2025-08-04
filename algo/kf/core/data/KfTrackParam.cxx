/* Copyright (C) 2007-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Igor Kulakov [committer], Maksym Zyzak */

#include "KfTrackParam.h"

#include "AlgoFairloggerCompat.h"

#include <iomanip>
#include <iostream>

namespace cbm::algo::kf
{
  template<typename DataT>
  std::string TrackParamBase<DataT>::ToString(int i) const
  {
    std::stringstream s;
    s.setf(std::ios::scientific, std::ios::floatfield);

    // print only one component of the SIMD vector
    if constexpr (std::is_same_v<DataT, fvec>) {
      if (i >= 0) {
        s << "T = ";
        s << " x " << GetX()[i];
        s << " y " << GetY()[i];
        s << " tx " << GetTx()[i];
        s << " ty " << GetTy()[i];
        s << " qp " << GetQp()[i];
        s << " t " << GetTime()[i];
        s << " vi " << GetVi()[i];

        s << " z " << GetZ()[i] << std::endl;
        s << "C = ";
        s << " c00 " << C00()[i];
        s << " c11 " << C11()[i];
        s << " c22 " << C22()[i];
        s << " c33 " << C33()[i];
        s << " c44 " << C44()[i] << std::endl;
        s << " c55 " << C55()[i] << std::endl;
        s << " c66 " << C66()[i] << std::endl;
        s << ToStringCorrelations(i);

        s << " chi2 " << fChiSq[i];
        s << " ndf " << fNdf[i] << std::endl;
        return s.str();
      }
    }

    {  // print all SIMD values
      s << "T = " << std::endl;
      s << " x " << GetX() << std::endl;
      s << " y " << GetY() << std::endl;
      s << " tx " << GetTx() << std::endl;
      s << " ty " << GetTy() << std::endl;
      s << " qp " << GetQp() << std::endl;
      s << " t " << GetTime() << std::endl;
      s << " vi " << GetVi() << std::endl;
      s << " z " << GetZ() << std::endl;
      s << "C = " << std::endl;
      s << " c00 " << C00() << std::endl;
      s << " c11 " << C11() << std::endl;
      s << " c22 " << C22() << std::endl;
      s << " c33 " << C33() << std::endl;
      s << " c44 " << C44() << std::endl;
      s << " c55 " << C55() << std::endl;
      s << " c66 " << C66() << std::endl;
      s << ToStringCorrelations(i);

      s << " chi2 " << fChiSq << std::endl;
      s << " ndf " << fNdf << std::endl;
    }
    return s.str();
  }

  template<typename DataT>
  std::string TrackParamBase<DataT>::ToStringCorrelations(int i) const
  {
    std::stringstream s;
    s << std::setprecision(6);

    // print only one component of the SIMD vector
    if constexpr (std::is_same_v<DataT, fvec>) {
      if (i >= 0) {
        float s0 = sqrt(C00()[i]);
        float s1 = sqrt(C11()[i]);
        float s2 = sqrt(C22()[i]);
        float s3 = sqrt(C33()[i]);
        float s4 = sqrt(C44()[i]);
        float s5 = sqrt(C55()[i]);
        float s6 = sqrt(C66()[i]);

        s << "K = " << std::endl;
        s << " " << C10()[i] / s1 / s0 << std::endl;
        s << " " << C20()[i] / s2 / s0 << " " << C21()[i] / s2 / s1 << std::endl;
        s << " " << C30()[i] / s3 / s0 << " " << C31()[i] / s3 / s1 << " " << C32()[i] / s3 / s2 << std::endl;
        s << " " << C40()[i] / s4 / s0 << " " << C41()[i] / s4 / s1 << " " << C42()[i] / s4 / s2 << " "
          << C43()[i] / s4 / s3 << std::endl;
        s << " " << C50()[i] / s5 / s0 << " " << C51()[i] / s5 / s1 << " " << C52()[i] / s5 / s2 << " "
          << C53()[i] / s5 / s3 << " " << C54()[i] / s5 / s4 << std::endl;
        s << " " << C60()[i] / s6 / s0 << " " << C61()[i] / s6 / s1 << " " << C62()[i] / s6 / s2 << " "
          << C63()[i] / s6 / s3 << " " << C64()[i] / s6 / s4 << " " << C65()[i] / s6 / s5 << std::endl;
        return s.str();
      }
    }

    // print all SIMD values
    {
      DataT s0 = sqrt(C00());
      DataT s1 = sqrt(C11());
      DataT s2 = sqrt(C22());
      DataT s3 = sqrt(C33());
      DataT s4 = sqrt(C44());
      DataT s5 = sqrt(C55());
      DataT s6 = sqrt(C66());

      s << "K = " << std::endl;
      s << " k10 " << C10() / s1 / s0 << std::endl;

      s << "\n k20 " << C20() / s2 / s0 << std::endl;
      s << " k21 " << C21() / s2 / s1 << std::endl;

      s << "\n k30 " << C30() / s3 / s0 << std::endl;
      s << " k31 " << C31() / s3 / s1 << std::endl;
      s << " k32 " << C32() / s3 / s2 << std::endl;

      s << "\n k40 " << C40() / s4 / s0 << std::endl;
      s << " k41 " << C41() / s4 / s1 << std::endl;
      s << " k42 " << C42() / s4 / s2 << std::endl;
      s << " k43 " << C43() / s4 / s3 << std::endl;

      s << "\n k50 " << C50() / s5 / s0 << std::endl;
      s << " k51 " << C51() / s5 / s1 << std::endl;
      s << " k52 " << C52() / s5 / s2 << std::endl;
      s << " k53 " << C53() / s5 / s3 << std::endl;
      s << " k54 " << C54() / s5 / s4 << std::endl;

      s << "\n k60 " << C60() / s6 / s0 << std::endl;
      s << " k61 " << C61() / s6 / s1 << std::endl;
      s << " k62 " << C62() / s6 / s2 << std::endl;
      s << " k63 " << C63() / s6 / s3 << std::endl;
      s << " k64 " << C64() / s6 / s4 << std::endl;
      s << " k65 " << C65() / s6 / s5 << std::endl;
    }

    return s.str();
  }

  template<typename DataT>
  double GetEntry(DataT v, int)
  {
    return (double) v;
  }

  template<>
  double GetEntry<fvec>(fvec v, int k)
  {
    return (double) v[k];
  }

  template<typename DataT>
  bool TrackParamBase<DataT>::IsFinite(bool printWhenWrong) const
  {
    // verify that all the numbers in the object are valid floats
    bool ret = true;

    auto check = [&](const std::string& s, DataT val) {
      if (!utils::IsFinite(val)) {
        ret = false;
        if (printWhenWrong) {
          LOG(warning) << " TrackParam parameter " << s << " is undefined: " << val;
        }
      }
    };

    check("x", fX);
    check("y", fY);
    check("tx", fTx);
    check("ty", fTy);
    check("qp", fQp);
    check("t", fT);
    check("vi", fVi);
    check("z", fZ);
    check("chi2", fChiSq);
    check("ndf", fNdf);
    check("chi2time", fChiSqTime);
    check("ndfTime", fNdfTime);

    for (int i = 0; i < kNtrackParam; i++) {
      for (int j = 0; j <= i; j++) {
        DataT val = C(i, j);
        if (!utils::IsFinite(val)) {
          ret = false;
          if (printWhenWrong) {
            LOG(warning) << " TrackParam Cov matrix element (" << i << "," << j << ")  is undefined: " << val;
          }
        }
      }
    }

    return ret;
  }

  template<typename DataT>
  bool TrackParamBase<DataT>::IsEntryConsistent(bool printWhenWrong, int k) const
  {
    // verify that all the numbers in the object are valid floats

    bool ok = true;

    auto check = [&](const std::string& s, DataT val) {
      double dval = GetEntry(val, k);
      if (!std::isfinite(dval)) {
        ok = false;
        if (printWhenWrong) {
          LOG(warning) << " TrackParam parameter " << s << ", vector entry " << k << " is not finite: " << dval;
        }
      }
    };

    check("x", fX);
    check("y", fY);
    check("tx", fTx);
    check("ty", fTy);
    check("qp", fQp);
    check("t", fT);
    check("vi", fVi);
    check("z", fZ);
    check("chi2", fChiSq);
    check("ndf", fNdf);
    check("chi2time", fChiSqTime);
    check("ndfTime", fNdfTime);

    // verify diagonal elements.
    // Cii is a squared dispersion of i-th parameter, it must be positive

    for (int i = 0; i < 7; i++) {
      double val = GetEntry(C(i, i), k);
      if (val <= 0.) {
        ok = false;
        if (printWhenWrong) {
          LOG(warning) << " TrackParam: C[" << i << "," << i << "], vec entry " << k << " is not positive: " << val
                       << std::endl;
        }
      }
    }

    // verify non-diagonal elements.
    // Cij/sqrt(Cii*Cjj) is a correlation between i-th and j-th parameter,
    // it must belong to [-1,1]

    for (int i = 1; i < 7; i++) {
      for (int j = 0; j < i; j++) {
        double tolerance = 1.0;
        double cij       = GetEntry(C(i, j), k);
        double cii       = GetEntry(C(i, i), k);
        double cjj       = GetEntry(C(j, j), k);
        if (cij * cij > tolerance * (cii * cjj)) {
          ok = false;
          if (printWhenWrong) {
            LOG(warning) << " TrackParam: correlation [" << i << "," << j << "], vec entry " << k
                         << " is too large: " << cij / sqrt(cii * cjj) << std::endl;
          }
        }
      }
    }

    // verify triplets of correlations
    // Kxy * Kxy + Kxz * Kxz + Kyz * Kyz <= 1 + 2 * Kxy * Kxz * Kyz

    for (int i = 2; i < 7; i++) {
      for (int j = 1; j < i; j++) {
        for (int m = 0; m < j; m++) {
          double tolerance = 1.0;
          double Cxx       = GetEntry(C(i, i), k);
          double Cyy       = GetEntry(C(j, j), k);
          double Czz       = GetEntry(C(m, m), k);
          double Cxy       = GetEntry(C(i, j), k);
          double Cxz       = GetEntry(C(i, m), k);
          double Cyz       = GetEntry(C(j, m), k);
          if (Cxx * Cyz * Cyz + Cyy * Cxz * Cxz + Czz * Cxy * Cxy
              > tolerance * (Cxx * Cyy * Czz + 2. * Cxy * Cyz * Cxz)) {
            ok = false;
            if (printWhenWrong) {
              double Kxy = Cxy / sqrt(Cxx * Cyy);
              double Kxz = Cxz / sqrt(Cxx * Czz);
              double Kyz = Cyz / sqrt(Cyy * Czz);
              LOG(warning) << " TrackParam: correlations between parametetrs " << i << ", " << j << ", " << m
                           << ", vec entry " << k << " are wrong: " << Kxy << " " << Kxz << " " << Kyz << std::endl
                           << " inequation: " << Kxy * Kxy + Kxz * Kxz + Kyz * Kyz << " > " << 1 + 2 * Kxy * Kxz * Kyz
                           << std::endl;
            }
          }
        }
      }
    }

    if (!ok && printWhenWrong) {
      LOG(warning) << "TrackParam parameters are not consistent: " << std::endl;
      LOG(warning) << ToString(k);
    }
    return ok;
  }

  template<typename DataT>
  bool TrackParamBase<DataT>::IsConsistent(bool printWhenWrong, int nFilled) const
  {
    int size = 1;

    if constexpr (std::is_same_v<DataT, fvec>) {
      size = fvec::size();
    }

    assert(nFilled <= size);
    if (nFilled < 0) {
      nFilled = size;
    }

    bool ok = true;
    for (int i = 0; i < nFilled; ++i) {
      ok = ok && IsEntryConsistent(printWhenWrong, i);
    }

    if (!ok && printWhenWrong) {
      LOG(warning) << "TrackParam parameters are not consistent: " << std::endl;
      if (nFilled == size) {
        LOG(warning) << "  All vector elements are filled " << std::endl;
      }
      else {
        LOG(warning) << "  Only first " << nFilled << " vector elements are filled " << std::endl;
      }
      LOG(warning) << ToString(-1);
    }
    return ok;
  }

  template class TrackParamBase<fvec>;
  template class TrackParamBase<float>;
  template class TrackParamBase<double>;

}  // namespace cbm::algo::kf
