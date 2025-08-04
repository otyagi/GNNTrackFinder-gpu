/* Copyright (C) 2010-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer] */

/**
 * \file CbmTrdCluster.h
 * \author Florian Uhlig <f.uhlig@gsi.de>
 * \brief Data Container for TRD clusters.
 **/

#ifndef CBMTRDCLUSTER_H
#define CBMTRDCLUSTER_H

#include "CbmCluster.h"  // for CbmCluster

#include <Rtypes.h>      // for CLRBIT, SETBIT, TESTBIT, ClassDefr

#include <cstdint>
#include <string>  // for string
#include <vector>  // for vector

/**
 * \class CbmTrdCluster
 * \author Florian Uhlig <f.uhlig@gsi.de>
 * \brief Data Container for TRD clusters.
 */
class CbmTrdCluster : public CbmCluster {
public:
  enum eCbmTrdClusterDef
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
  CbmTrdCluster();
  CbmTrdCluster(const CbmTrdCluster& ref);
  CbmTrdCluster(const std::vector<int32_t>& indices, int32_t address);
  /**
   * \brief Constructor starting from first digit (FASP specific).
   * \param[in] address global module address
   * \param[in] idx global digi index in the TClonesArray
   * \param[in] chT RO channel address within the module for tilt pairing
   * \param[in] chR RO channel address within the module for rect pairing
   * \param[in] r module row for the RO channel
   * \param[in] time relative buffer DAQ time
   */
  CbmTrdCluster(int32_t address, int32_t idx, uint16_t chT, uint16_t chR, int32_t r, int32_t time);
  /**
   * \brief Destructor.
   */
  virtual ~CbmTrdCluster();

  CbmTrdCluster& operator=(const CbmTrdCluster& ref);

  /** \brief Append a channel to cluster edge. The usage is to account for the masked channels.
   * The mask status is assumed to be performed in the calling function.
   * \param[in] r by default apply to the right edge. If false apply to left
   */
  bool AddChannel(bool r = true);
  /** \brief Append digi to cluster
   * \param[in] idx index of digi in TClonesArray
   * \param[in] chT RO channel for digi (tilt pairing for FASP) default 0xffff (SPADIC)
   * \param[in] chR RO channel for rect pairing (only for FASP)
   * \param[in] dt update start time of cluster if current digi is prompt
   * \return true if successful
   */
  bool AddDigi(int32_t idx, uint16_t chT = 0xffff, uint16_t chR = 0, int32_t dt = 0);
  /** \brief reset cluster data*/
  void Clear(Option_t*);
  /** Accessors **/
  uint16_t GetNCols() const { return fNCols; }
  uint16_t GetNRows() const { return fNRows & 0x1f; }
  uint16_t GetEndCh() const { return fStartCh + fNCols - 1; }
  uint16_t GetRow() const { return GetNRows(); }
  uint16_t GetSize() const { return GetNCols(); }
  uint16_t GetStartCh() const { return fStartCh; }
  uint32_t GetStartTime() const { return fStartTime; }
  bool HasFaspDigis() const { return TESTBIT(fNRows, kFasp); }
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
  bool Merge(CbmTrdCluster* second);
  /** \brief Initialize basic parameters of the cluster
   * \param[in] address global module address
   * \param[in] row cluster row in the module
   * \param[in] time cluster time in time buffer
   */
  void ReInit(int32_t address, int32_t row, int32_t time);
  /** Setters **/
  void SetNCols(uint16_t ncols) { fNCols = ncols; }
  void SetNRows(uint16_t nrows)
  {
    fNRows &= (7 << 5);
    fNRows |= (nrows & 0x1f);
  }
  void SetFaspDigis(bool set = true) { set ? SETBIT(fNRows, kFasp) : CLRBIT(fNRows, kFasp); }
  void SetStart(bool set = true) { set ? SETBIT(fNRows, kStart) : CLRBIT(fNRows, kStart); }
  void SetStop(bool set = true) { set ? SETBIT(fNRows, kStop) : CLRBIT(fNRows, kStop); }

  /** \brief Extended functionality*/
  virtual std::string ToString() const;

protected:
  uint8_t fNCols      = 0;       //< number of columns with charge above threshold
  uint8_t fNRows      = 0x1f;    //< cluster row info plus extra meta data. Use dedicated getters for the correct value
  uint16_t fStartCh   = 0xffff;  //< channel address of first channel
  uint32_t fStartTime = 0xffffffff;  //! start time of cluster in clk units wrt buffer start

  ClassDef(CbmTrdCluster, 6)  // cluster of digi for the TRD detector
};
#endif
