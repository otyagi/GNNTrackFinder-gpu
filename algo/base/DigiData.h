/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#ifndef CBM_ALGO_BASE_DIGI_DATA_H
#define CBM_ALGO_BASE_DIGI_DATA_H

#include "CbmBmonDigi.h"
#include "CbmDigiData.h"
#include "CbmDigiEvent.h"
#include "CbmEventTriggers.h"
#include "CbmFsdDigi.h"
#include "CbmMuchDigi.h"
#include "CbmPsdDigi.h"
#include "CbmRichDigi.h"
#include "CbmStsDigi.h"
#include "CbmTofDigi.h"
#include "CbmTrdDigi.h"
#include "PODVector.h"

namespace cbm::algo
{
  /**
   * @brief Collection of digis from all detector systems
   *
   * Very similar to CbmDigiData. CbmDigiData is meant for file storage,
   * while this is used for the actual processing. Seperate classes
   * allow for more flexibility and easier optimization.
   *
   * @see CbmDigiData
   * @note Uses PODVector for storage, so memory is not initialized by default.
   */
  struct DigiData {
    PODVector<CbmStsDigi> fSts;    ///< Unpacked STS digis
    PODVector<CbmMuchDigi> fMuch;  ///< Unpacked MUCH digis
    PODVector<CbmTofDigi> fTof;    ///< Unpacked TOF digis
    PODVector<CbmBmonDigi> fBmon;  ///< Unpacked Bmon digis
    PODVector<CbmTrdDigi> fTrd;    ///< Unpacked TRD digis
    PODVector<CbmTrdDigi> fTrd2d;  ///< Unpacked TRD2D digis
    PODVector<CbmRichDigi> fRich;  ///< Unpacked RICH digis
    PODVector<CbmPsdDigi> fPsd;    ///< Unpacked PSD digis
    PODVector<CbmFsdDigi> fFsd;    ///< Unpacked FSD digis

    DigiData();
    ~DigiData();

    explicit DigiData(const CbmDigiData& storable);

    /**
     * @brief Get the number of digis for a given subsystem
     *
     * @param system Subsystem to get the number of digis for
     * @todo Should use fles::Subsystem instead ECbmModuleId
     */
    size_t Size(ECbmModuleId system) const;

    /**
     * @brief Get the total number of digis across all subsystems.
     */
    size_t TotalSize() const;

    /**
     * @brief Get the total number of bytes used by all digis.
     */
    size_t TotalSizeBytes() const;

    /**
     * @brief Convert to CbmDigiData for file storage
     *
     * @note This is a very expensive operation, as it copies all data.
     */
    CbmDigiData ToStorable() const;
  };

  /**
   * @brief Event data with event number and trigger time
   *
   * @see CbmDigitEvent
   * @note Uses PODVector for storage, so memory is not initialized by default.
   */
  struct DigiEvent : public DigiData {
    // FIXME: Event number not set yet!
    uint64_t fNumber = -1;  ///< Event identifier
    double fTime     = 0;   ///< Event trigger time [ns]
    CbmEventTriggers fSelectionTriggers;

    static std::vector<DigiEvent> FromCbmDigiEvents(const std::vector<CbmDigiEvent>& events);
    static std::vector<CbmDigiEvent> ToCbmDigiEvents(const std::vector<DigiEvent>& events);

    DigiEvent() = default;

    explicit DigiEvent(const CbmDigiEvent& storable);

    /**
   * @brief Convert to CbmDigiEvent for file storage
   *
   * @note This is a very expensive operation, as it copies all data.
   */
    CbmDigiEvent ToStorable() const;
  };

}  // namespace cbm::algo

#endif  // CBM_ALGO_BASE_DIGI_DATA_H
