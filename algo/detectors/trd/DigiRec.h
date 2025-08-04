/* Copyright (C) 2018-2020 Horia Hulubei National Institute of Physics and Nuclear Engineering, Bucharest
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Alexandru Bercuci[committer] */

#pragma once

#include "CbmTrdDigi.h"
#include "compat/RTypes.h"

/** @class DigiRec
 ** @brief Extend the TRD(2D) digi class to incorporate FEE calibration.
 ** @author Alexandru Bercucic <abercuci@niham.nipne.ro>
 ** @since 01.10.2021
 ** @date 01.10.2021
 **
 ** The digi class contains the information as it is produced by the FEE (ASIC/GETS)
 ** The variation from channel to channel is captured by running the pulser on anode wires
 ** using various signal values, frequencies, etc. The calibrated baselines, gains, jitter,
 ** etc. are transported via the parameter files and are applied to the data within the digRec
 ** class which is in the end used to calculate the TRD hit parameters.
 **/

namespace cbm::algo::trd
{
  class DigiRec : public CbmTrdDigi {
    friend class Cluster2D;
    friend class HitFinder2D;

   public:
    /** \brief Default constructor*/
    DigiRec();
    /** \brief Wrap CbmTrdDigi constructor*/
    DigiRec(const CbmTrdDigi& d, double* g = NULL, double* t = NULL);
    /** \brief Constructor and merger*/
    DigiRec(const CbmTrdDigi& dt, const CbmTrdDigi& dr, double* g = NULL, double* t = NULL);

    virtual ~DigiRec() { ; }

    /** \brief Return calibrated tilt signal
   * \param[out] on flag signal exists */
    double GetTiltCharge(bool& on) const { return GetCharge(0, on); }
    /** \brief Return calibrated tilt time [ns]*/
    double GetTiltTime() const { return GetTime(0); }
    /** \brief Return calibrated rect signal
   * \param[out] on flag signal exists */
    double GetRectCharge(bool& on) const { return GetCharge(1, on); }
    /** \brief Return calibrated rect time [ns]*/
    double GetRectTime() const { return GetTime(1); }
    /** \brief Return calibrated signal
   * \param[in] typ tilt [0], rect [1]
   * \param[out] on flag signal exists
   */
    double GetCharge(int typ, bool& on) const;
    /** \brief Return calibrated time
   * \param[in] typ tilt [0], rect [1]
  */
    double GetTime(int typ) const;
    bool HasRectOvf() const { return TESTBIT(fStatus, 1); }
    bool HasTiltOvf() const { return TESTBIT(fStatus, 0); }
    /** \brief Init FEE gain and time walk corrections */
    void Init(double g[2], double t[3]);

    static float GetBaselineCorr() { return 4095. * fgBaseline / fgOutGain; }

   private:
    unsigned char fStatus;  //< bit map to store calibration flags
    double fG[2];           //< FEE gain correction for channel T & R
    double fT[3];           //< FEE time walk  correction as function of charge

    static float fgBaseline;  ///< FASP baseline [V]
    static float fgOutGain;   ///< FASP -> ADC gain [V/4095 ADC]
  };

}  // namespace cbm::algo::trd
