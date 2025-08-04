/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Alexandru Bercuci */

#pragma once

#include "DigiRec.h"
#include "compat/RTypes.h"

#include <cstdint>
#include <vector>  // for vector

class CbmTrdDigi;

namespace cbm::algo::trd
{
  /**
 * \class Cluster2D
 * \author Dominik Smith <d.smith@gsi.de>
 * \brief Data Container for TRD clusters.
 */
  class Cluster2D {

   public:
    enum eClusterDef
    {
      kFasp = 5  ///< set type of FEE digis contained
        ,
      kStart  ///< only for triangular if no T in first col
        ,
      kStop  ///< only for triangular if no R in last col
    };

    /**
   * \brief Default constructor.
   */
    Cluster2D() = delete;
    Cluster2D(const Cluster2D& ref);
    /**
   * \brief Constructor starting from first digit (FASP specific).
   * \param[in] address global module address
   * \param[in] idx global digi index in the TClonesArray
   * \param[in] digi pointer to digi
   * \param[in] chT RO channel address within the module for tilt pairing
   * \param[in] chR RO channel address within the module for rect pairing
   * \param[in] r module row for the RO channel
   * \param[in] time relative buffer DAQ time
   */
    Cluster2D(int32_t address, int32_t idx, const CbmTrdDigi* digi, uint16_t chT, uint16_t chR, int32_t r,
              int32_t time);
    /**
   * \brief Destructor.
   */
    virtual ~Cluster2D(){};

    /**
	 * \brief Add digi to cluster.
	 * \param[in] index Digi index in TClonesArray.
	 * \param[in] digi pointer to digi
	 */
    void AddDigiIdxPair(int32_t index, const CbmTrdDigi* digi)
    {
      fDigiIndices.push_back(index);
      fDigis.push_back(digi);
    }

    /**
	 * \brief Add array of digi to cluster.
	 * \param[in] indices Array of digi indices in TClonesArray.
	 */
    void AddDigiIdxPairs(const std::vector<int32_t>& indices, const std::vector<const CbmTrdDigi*> digis)
    {
      fDigiIndices.insert(fDigiIndices.end(), indices.begin(), indices.end());
      fDigis.insert(fDigis.end(), digis.begin(), digis.end());
    }

    /**
	 * \brief Number of digis in cluster.
	 * \return Number of digis in cluster.
	 */
    int32_t GetNofDigis() const { return fDigiIndices.size(); }

    /**
	 * \brief Get digi at position index.
	 * \param[in] index Position of digi in array.
	 * \return Digi index in TClonesArray.
	 */
    int32_t GetDigi(int32_t index) const { return fDigiIndices[index]; }

    /**
	 * \brief Get array of digi indices.
	 * \return Array of digi indices in TClonesArray.
	 */
    const std::vector<int32_t>& GetDigiIndices() const { return fDigiIndices; }

    /**
	 * \brief Get array of digi pointers.
	 * \return Array of digi pointers
	 */
    std::vector<const CbmTrdDigi*>& GetDigis() { return fDigis; }

    /**
	 * \brief Get array of calibrated digis
	 * \return Array of calibrated digis
	 */
    const std::vector<DigiRec>& GetRecDigis() const { return fRecDigis; }

    /**
	 * \brief Remove all digis.
	 */
    void ClearDigis()
    {
      fDigiIndices.clear();
      fDigis.clear();
      fRecDigis.clear();
    }

    /** Accessors **/
    int32_t GetAddress() const { return fAddress; }

    /** \brief Append digi to cluster
   * \param[in] idx index of digi in TClonesArray
   * \param[in] digi pointer to digi
   * \param[in] chT RO channel for digi (tilt pairing for FASP) default 0xffff (SPADIC)
   * \param[in] chR RO channel for rect pairing (only for FASP)
   * \param[in] dt update start time of cluster if current digi is prompt
   * \return true if successful
   */
    bool AddDigi(int32_t idx, const CbmTrdDigi* digi, uint16_t chT = 0xffff, uint16_t chR = 0, int32_t dt = 0);

    /** Accessors **/
    uint16_t GetNCols() const { return fNCols; }
    uint16_t GetNRows() const { return fNRows & 0x1f; }
    uint16_t GetNRowsRaw() const { return fNRows; }
    uint16_t GetEndCh() const { return fStartCh + fNCols - 1; }
    uint16_t GetRow() const { return GetNRows(); }
    uint16_t GetSize() const { return GetNCols(); }
    uint16_t GetStartCh() const { return fStartCh; }
    uint32_t GetStartTime() const { return fStartTime; }
    bool HasFaspDigis() const { return true; }
    bool HasStart() const { return TESTBIT(fNRows, kStart); }
    bool HasStop() const { return TESTBIT(fNRows, kStop); }

    /** \brief Query on RO channels list
   * \param[in] chT tilted RO channel for digi
   * \param[in] chR rectangular RO channel for digi
   * \return -1 before range; 0 in range; 1 after range; -2 cluster empty of digits
   */
    int32_t IsChannelInRange(uint16_t chT, uint16_t chR) const;

    /** \brief Merge current cluster with info from second
   * \param[in] second cluster to be added
   * \param[in] typ the type of pad-plane of the source chamber; true if Trd2d
   * \return success or fail
   */
    bool Merge(Cluster2D* second);

    /** \brief Fill array of calibrated digis
   * \param[in] number of columns
   * \return success or fail
   */
    bool Finalize(const size_t numCols);

    /** Setters **/
    void SetNCols(uint16_t ncols) { fNCols = ncols; }
    void SetNRows(uint16_t nrows)
    {
      fNRows &= (7 << 5);
      fNRows |= (nrows & 0x1f);
    }
    void SetStart(bool set = true) { set ? SETBIT(fNRows, kStart) : CLRBIT(fNRows, kStart); }
    void SetStop(bool set = true) { set ? SETBIT(fNRows, kStop) : CLRBIT(fNRows, kStop); }

   private:
    std::vector<DigiRec> fRecDigis;         ///< Array of calibrated digis
    std::vector<const CbmTrdDigi*> fDigis;  ///< Array of digi pointers
    std::vector<int32_t> fDigiIndices;      ///< Array of digi indices

    int32_t fAddress    = 0;     ///< Unique detector ID
    uint8_t fNCols      = 0;     //< number of columns with charge above threshold
    uint8_t fNRows      = 0x1f;  //< cluster row info plus extra meta data. Use dedicated getters for the correct value
    uint16_t fStartCh   = 0xffff;      //< channel address of first channel
    uint32_t fStartTime = 0xffffffff;  //! start time of cluster in clk units wrt buffer start
  };
}  // namespace cbm::algo::trd
