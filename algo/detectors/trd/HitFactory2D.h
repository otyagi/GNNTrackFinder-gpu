/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Alexandru Bercuci */

#pragma once

#include "compat/RTypes.h"

#include <cstdint>
#include <vector>

#define NBINSCORRX 50  //! no of bins in the discretized correction LUT
#define NBINSCORRY 4   //! no of bins in the parametrization correction

using std::vector;

namespace cbm::algo::trd
{

  // working representation of a hit on which the reconstruction is performed
  class HitFactory2D {

   public:
    struct signal {
      double s;   //! working copy of signals from cluster
      double se;  //! working copy of signal errors from cluster
      char t;     //! working copy of signal relative timing
      double x;   //! working copy of signal relative positions
      double xe;  //! working copy of signal relative position errors
      signal(double _s, double _se, char _t, double _x, double _xe) : s(_s), se(_se), t(_t), x(_x), xe(_xe) {}
      signal() : s(0), se(0), t(0), x(0), xe(0) {}
    };

    std::vector<signal> fSignal;
    int nCols;

    uint64_t vt0 = 0;  //! start time of current hit [clk]
    uint8_t vcM  = 0;  //! maximum col
    uint8_t vrM  = 0;  //! maximum row
    uint8_t viM  = 0;  //! index of maximum signal in the projection
    uint16_t vyM = 0;  //! bit map for cluster topology classification

    HitFactory2D(int ncols) : nCols(ncols), vt0(0), vcM(0), vrM(0), viM(0), vyM(0) {}

    void reset()
    {
      fSignal.clear();
      vt0 = 0;
      vcM = 0;
      vrM = 0;
      viM = 0;
      vyM = 0;
    }

    bool HasLeftSgn() const { return TESTBIT(vyM, 3); }
    bool HasOvf() const { return TESTBIT(vyM, 12); }
    bool IsBiasX() const { return TESTBIT(vyM, 4); }
    bool IsBiasXleft() const { return TESTBIT(vyM, 5); }
    bool IsBiasXmid() const { return TESTBIT(vyM, 6); }
    bool IsBiasXright() const { return TESTBIT(vyM, 7); }
    bool IsBiasY() const { return TESTBIT(vyM, 8); }
    bool IsBiasYleft() const { return TESTBIT(vyM, 9); }
    bool IsLeftHit() const { return TESTBIT(vyM, 2); }
    bool IsMaxTilt() const { return TESTBIT(vyM, 0); }
    bool IsOpenLeft() const { return (viM % 2 && !IsMaxTilt()) || (!(viM % 2) && IsMaxTilt()); }
    inline bool IsOpenRight() const;
    bool IsSymmHit() const { return TESTBIT(vyM, 1); }
    void SetBiasX(bool set = 1) { set ? SETBIT(vyM, 4) : CLRBIT(vyM, 4); }
    void SetBiasXleft(bool set = 1) { set ? SETBIT(vyM, 5) : CLRBIT(vyM, 5); }
    void SetBiasXmid(bool set = 1) { set ? SETBIT(vyM, 6) : CLRBIT(vyM, 6); }
    void SetBiasXright(bool set = 1) { set ? SETBIT(vyM, 7) : CLRBIT(vyM, 7); }
    void SetBiasY(bool set = 1) { set ? SETBIT(vyM, 8) : CLRBIT(vyM, 8); }
    void SetBiasYleft(bool set = 1) { set ? SETBIT(vyM, 9) : CLRBIT(vyM, 9); }
    void SetBiasYmid(bool set = 1) { set ? SETBIT(vyM, 10) : CLRBIT(vyM, 10); }
    void SetBiasYright(bool set = 1) { set ? SETBIT(vyM, 11) : CLRBIT(vyM, 11); }
    void SetLeftSgn(bool set = 1) { set ? SETBIT(vyM, 3) : CLRBIT(vyM, 3); }
    void SetLeftHit(bool set = 1) { set ? SETBIT(vyM, 2) : CLRBIT(vyM, 2); }
    void SetSymmHit(bool set = 1) { set ? SETBIT(vyM, 1) : CLRBIT(vyM, 1); }
    void SetMaxTilt(bool set = 1) { set ? SETBIT(vyM, 0) : CLRBIT(vyM, 0); }
    void SetOvf(bool set = 1) { set ? SETBIT(vyM, 12) : CLRBIT(vyM, 12); }
    uint16_t GetHitMap() const { return vyM; }

    std::pair<double, double> GetDxDy(const int n0);
    double GetXoffset(int n0 = 0) const;
    double GetYoffset(int n0 = 0) const;

    void RecenterXoffset(double& dx);
    /** \brief Shift graph representation to [-0.5, 0.5]
   * \param[in] dy offset wrt center pad [ph]
   */
    void RecenterYoffset(double& dy);
    /** \brief Hit classification wrt center pad
   * \return hit type : center hit [0]; side hit [1]
   */

    int GetHitClass() const;
    /** \brief Hit classification wrt signal bias
   * \return hit type : center hit [0]; side hit [1]
   */
    int GetHitRcClass(int a0) const;

    double GetXcorr(double dx, int typ, int cls = 0) const;
    /** \brief y position correction based on LUT
   * \param[in] dy offset computed on charge sharing expressed in [ph]
   * \param[in] cls correction class
   * \return correction expresed in [cm]
   */
    double GetYcorr(double dy, int cls = 0) const;
    /** \brief Shift graph representation to [-0.5, 0.5]
   * \param[in] dx offset wrt center pad [pw]
   */

    std::pair<double, double> CorrectPosition(double dx, double dy, const double xcorr, const double padSizeX,
                                              const double padSizeY);

    static float fgCorrXdx;                         //! step of the discretized correction LUT
    static float fgCorrXval[3][NBINSCORRX];         //! discretized correction LUT
    static float fgCorrYval[NBINSCORRY][2];         //! discretized correction params
    static float fgCorrRcXval[2][NBINSCORRX];       //! discretized correction LUT
    static float fgCorrRcXbiasXval[3][NBINSCORRX];  //! discretized correction LUT

    // Nint function copied from TMath until better option is available
    template<typename T>
    inline int Nint(T x) const
    {
      int i;
      if (x >= 0) {
        i = int(x + 0.5);
        if (i & 1 && x + 0.5 == T(i)) i--;
      }
      else {
        i = int(x - 0.5);
        if (i & 1 && x - 0.5 == T(i)) i++;
      }
      return i;
    }
  };

}  // namespace cbm::algo::trd
