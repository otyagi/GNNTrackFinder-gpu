/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Alexandru Bercuci */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

class CbmTrdDigi;

namespace cbm::algo::trd
{
  /**
 * \class Cluster
 * \author Dominik Smith <d.smith@gsi.de>
 * \brief Data container for TRD clusters.
 */
  class Cluster {

   public:
    /**
   * \brief Default constructor.
   */
    Cluster() = delete;
    Cluster(const std::vector<int32_t>& indices, const std::vector<const CbmTrdDigi*>& digis, int32_t address,
            uint16_t ncols, uint16_t nrows);

    /**
   * \brief Destructor.
   */
    virtual ~Cluster(){};

    /**
	 * \brief Number of digis in cluster.
	 * \return Number of digis in cluster.
	 */
    size_t GetNofDigis() const { return fDigiInd.size(); }

    /**
	 * \brief Get digi at position index.
	 * \param[in] index Position of digi in array.
	 * \return Digi index in TClonesArray.
	 */
    int32_t GetDigi(int32_t index) const { return fDigiInd[index]; }

    /**
	 * \brief Get array of digi pointers.
	 * \return Array of digi pointers.
	 */
    const std::vector<const CbmTrdDigi*>& GetDigis() const { return fDigis; }

    /**
	 * \brief Get array of digi indices.
	 * \return Array of digi indices in TClonesArray.
	 */
    const std::vector<int32_t>& GetDigiIndices() const { return fDigiInd; }

    /** Accessors **/
    int32_t GetAddress() const { return fAddress; }
    uint16_t GetStartCh() const { return fStartCh; }
    uint16_t GetNCols() const { return fNCols; }
    uint16_t GetNRows() const { return fNRows & 0x1f; }
    uint16_t GetNRowsRaw() const { return fNRows; }
    uint32_t GetStartTime() const { return fStartTime; }
    bool HasFaspDigis() const { return false; }

   private:
    std::vector<int32_t> fDigiInd;          ///< Array of digi indices
    std::vector<const CbmTrdDigi*> fDigis;  ///< Array of digi pointers

    int32_t fAddress    = 0;     ///< Unique detector ID
    uint8_t fNCols      = 0;     //< number of columns with charge above threshold
    uint8_t fNRows      = 0x1f;  //< cluster row info plus extra meta data. Use dedicated getters for the correct value
    uint16_t fStartCh   = 0xffff;      //< channel address of first channel
    uint32_t fStartTime = 0xffffffff;  //! start time of cluster in clk units wrt buffer start

    void SetNRows(uint16_t nrows)
    {
      fNRows &= (7 << 5);
      fNRows |= (nrows & 0x1f);
    }
  };
}  // namespace cbm::algo::trd
