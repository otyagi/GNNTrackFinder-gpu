/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer] */

#ifndef TOFCALIBRATE_H
#define TOFCALIBRATE_H 1

#include "CbmTofDigi.h"
#include "PartitionedVector.h"
#include "tof/CalibrateSetup.h"
#include "tof/Clusterizer.h"

#include <gsl/span>
#include <optional>
#include <sstream>
#include <vector>

#include <xpu/host.h>

namespace cbm::algo::tof
{

  /** @struct CalibrateMonitorData
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 16 Oct 2023
   ** @brief Monitoring data for calibration
   **/
  struct CalibrateMonitorData {
    xpu::timings fTime;
    size_t fNumDigis            = 0;
    size_t fDigiCalibUnknownRPC = 0;
    size_t fDigiDeadTimeCount   = 0;

    std::string print() const
    {
      std::stringstream ss;
      ss << "Calibrate stats: processed digis " << fNumDigis << ", num unknown RPC " << fDigiCalibUnknownRPC
         << ", num deadtime skips " << fDigiDeadTimeCount << ", time " << fTime.wall() << " ms ( " << fTime.throughput()
         << " GB/s )" << std::endl;
      return ss.str();
    }
  };

  /** @class Calibrate
 ** @brief Algo class for calibration
 ** @author Dominik Smith <d.smith@gsi.de>
 ** @since 16.10.2023
 **
 **/
  class Calibrate {

   public:
    typedef std::pair<std::vector<CbmTofDigi>, CalibrateMonitorData> resultType;

    /** @brief Algorithm execution
     ** @param digis to calibrate
     ** @return pair: digi timeslice, monitoring data
     **
     ** @note Modifies input digis for time calibration
     **/
    resultType operator()(gsl::span<const CbmTofDigi> digiIn);

    /** @brief Constructor **/
    explicit Calibrate(tof::CalibrateSetup params);

   private:  // members
    /** @brief Parameters for TOF calibrator **/
    tof::CalibrateSetup fSetup;

    /** @brief Storage for deadtime check **/
    std::vector<double> mChannelDeadTime;

    /** @brief Offset of super module type **/
    std::vector<size_t> smTypeOffset;

    /** @brief Offset of super module **/
    std::vector<size_t> smOffset;

    /** @brief Offset of RPC **/
    std::vector<size_t> rpcOffset;
  };
}  // namespace cbm::algo::tof

#endif /* TOFCALIBRATE_H */
