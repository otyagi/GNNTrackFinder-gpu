/* Copyright (C) 2021 Goethe-University Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pascal Raisig [committer] */

#ifndef CbmTsEventHeader_H
#define CbmTsEventHeader_H

#include <FairEventHeader.h>

#include <cstddef>
#include <cstdint>

class CbmTsEventHeader : public FairEventHeader {

public:
  /** Default constructor */
  CbmTsEventHeader();

  /** Default destructor */
  virtual ~CbmTsEventHeader() {};

  /** @brief Add a number of digis from this Ts */
  void AddNDigisMuch(uint64_t value) { fNDigisMuch += value; }
  /** @brief Add a number of digis from this Ts */
  void AddNDigisPsd(uint64_t value) { fNDigisPsd += value; }
  /** @brief Add a number of digis from this Ts */
  void AddNDigisFsd(uint64_t value) { fNDigisFsd += value; }
  /** @brief Add a number of digis from this Ts */
  void AddNDigisRich(uint64_t value) { fNDigisRich += value; }
  /** @brief Add a number of digis from this Ts */
  void AddNDigisSts(uint64_t value) { fNDigisSts += value; }
  /** @brief Add a number of digis from this Ts */
  void AddNDigisTof(uint64_t value) { fNDigisTof += value; }
  /** @brief Add a number of digis from this Ts */
  void AddNDigisTrd1D(uint64_t value) { fNDigisTrd1D += value; }
  /** @brief Add a number of digis from this Ts */
  void AddNDigisTrd2D(uint64_t value) { fNDigisTrd2D += value; }
  /** @brief Add a number of digis from this Ts */
  void AddNDigisBmon(uint64_t value) { fNDigisBmon += value; }

  /** @brief Get the number of digis in this Ts */
  uint64_t GetNDigisMuch() const { return fNDigisMuch; }
  /** @brief Get the number of digis in this Ts */
  uint64_t GetNDigisPsd() const { return fNDigisPsd; }
  /** @brief Get the number of digis in this Ts */
  uint64_t GetNDigisFsd() const { return fNDigisFsd; }
  /** @brief Get the number of digis in this Ts */
  uint64_t GetNDigisRich() const { return fNDigisRich; }
  /** @brief Get the number of digis in this Ts */
  uint64_t GetNDigisSts() const { return fNDigisSts; }
  /** @brief Get the number of digis in this Ts */
  uint64_t GetNDigisTof() const { return fNDigisTof; }
  /** @brief Get the number of digis in this Ts */
  uint64_t GetNDigisTrd1D() const { return fNDigisTrd1D; }
  /** @brief Get the number of digis in this Ts */
  uint64_t GetNDigisTrd2D() const { return fNDigisTrd2D; }
  /** @brief Get the number of digis in this Ts */
  uint64_t GetNDigisBmon() const { return fNDigisBmon; }

  /** Get the Start time of the this Timeslice linked to this event header*/
  uint64_t GetTsIndex() const { return fTsIndex; }
  /** Get the Start time of the this Timeslice linked to this event header*/
  uint64_t GetTsStartTime() const { return fTsStartTime; }

  /** @brief Resets counters and variables for a new Timeslice, e.g. the NDigis counter are set back to 0*/
  void Reset();

  /** @brief Set the number of digis in this Ts */
  void SetNDigisMuch(uint64_t value) { fNDigisMuch = value; }
  /** @brief Set the number of digis in this Ts */
  void SetNDigisPsd(uint64_t value) { fNDigisPsd = value; }
  /** @brief Set the number of digis in this Ts */
  void SetNDigisFsd(uint64_t value) { fNDigisFsd = value; }
  /** @brief Set the number of digis in this Ts */
  void SetNDigisRich(uint64_t value) { fNDigisRich = value; }
  /** @brief Set the number of digis in this Ts */
  void SetNDigisSts(uint64_t value) { fNDigisSts = value; }
  /** @brief Set the number of digis in this Ts */
  void SetNDigisTof(uint64_t value) { fNDigisTof = value; }
  /** @brief Set the number of digis in this Ts */
  void SetNDigisTrd1D(uint64_t value) { fNDigisTrd1D = value; }
  /** @brief Set the number of digis in this Ts */
  void SetNDigisTrd2D(uint64_t value) { fNDigisTrd2D = value; }
  /** @brief Set the number of digis in this Ts */
  void SetNDigisBmon(uint64_t value) { fNDigisBmon = value; }

  /** @brief Set the Ts Start Time @param value Start time of the TS */
  void SetTsIndex(uint64_t value) { fTsIndex = value; }
  /** @brief Set the Ts Start Time @param value Start time of the TS */
  void SetTsStartTime(uint64_t value) { fTsStartTime = value; }


  CbmTsEventHeader(const CbmTsEventHeader&) = default;
  CbmTsEventHeader& operator=(const CbmTsEventHeader&) = default;

protected:
  /** Timeslice index */
  uint64_t fTsIndex = 0;
  /** Timeslice start time */
  uint64_t fTsStartTime = 0;

  /** @brief nDigis in "this" timeslice measured by the MUCH */
  uint64_t fNDigisMuch = 0;
  /** @brief nDigis in "this" timeslice measured by the PSD */
  uint64_t fNDigisPsd = 0;
  /** @brief nDigis in "this" timeslice measured by the FSD */
  uint64_t fNDigisFsd = 0;
  /** @brief nDigis in "this" timeslice measured by the RICH */
  uint64_t fNDigisRich = 0;
  /** @brief nDigis in "this" timeslice measured by the STS */
  uint64_t fNDigisSts = 0;
  /** @brief nDigis in "this" timeslice measured by the TOF */
  uint64_t fNDigisTof = 0;
  /** @brief nDigis in "this" timeslice measured by the TRD1D */
  uint64_t fNDigisTrd1D = 0;
  /** @brief nDigis in "this" timeslice measured by the TRD2D */
  uint64_t fNDigisTrd2D = 0;
  /** @brief nDigis in "this" timeslice measured by the BMON */
  uint64_t fNDigisBmon = 0;

  ClassDef(CbmTsEventHeader, 7)
};
#endif
