/* Copyright (C) 2021 Goethe-University Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pascal Raisig [committer], Alexandru Bercuci */

/**
 * @file CbmTrdUnpackFaspAlgo.h
 * @author Alexandru Bercuci
 * @author Pascal Raisig (praisig@ikf.uni-frankfurt.de)
 * @brief Trd FASP unpacking algorithm
 * @version 0.1
 * @date 2021-04-21
 *
 * @copyright Copyright (c) 2021
 *
 * This is the base class for the algorithmic part of the tsa data unpacking
 * processes of the CbmTrd.
 * The actual translation from tsa to digi happens in the derived classes.
 *
 *
*/

#ifndef CbmTrdUnpackFaspAlgo_H
#define CbmTrdUnpackFaspAlgo_H

#include "CbmRecoUnpackAlgo.tmpl"
#include "CbmTrdDigi.h"
#include "CbmTrdParFasp.h"
#include "CbmTrdParSetAsic.h"
#include "CbmTrdUnpackFaspMonitor.h"
#include "Timeslice.hpp"  // timeslice

#include <Rtypes.h>  // for types
#include <RtypesCore.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

// TODO to be defined in correlation with time offsets (AB 20.05.22)
#define NCRI 40  // no of CRI in the system (5/TRD-2D_FASP module)
#define NCOLS 8  // no of cols / FASP

class CbmTrdParSetDigi;
class CbmTrdUnpackFaspAlgo : public CbmRecoUnpackAlgo<CbmTrdDigi> {
 public:
  /** @brief Bytes per FASP frame stored in the microslices (32 bits words)
   * - DATA WORD - for legacy version
   * ffff.ffdd dddd.dddd dddd.tttt ttta.cccc
   * f - FASP id
   * d - ADC signal
   * t - time label inside epoch
   * a - word type (1)
   * c - channel id
   * - EPOCH WORD -
   * ffff.fftt tttt.tttt tttt.tttt ttta.cccc
   * f - FASP id
   * t - epoch index
   * a - word type (0)
   * c - channel id
   * =====================================================
   * - DATA WORD - for 06.2024 version
   * afff.fffc ccct.tttt ttdd.dddd dddd.dddd
   * f - FASP id
   * d - ADC signal
   * t - time label inside epoch
   * a - word type (1)
   * c - channel id
   * - EPOCH WORD -
   * afff.fffc ccct.tttt tttt tttt tttt.tttt
   * a - word type (0)
   * f - FASP id
   * t - epoch index
   * c - channel id
   */
  enum class eMessageLength : int
  {
    kMessCh    = 4,
    kMessType  = 1,
    kMessTlab  = 7,
    kMessData  = 14,
    kMessFasp  = 6,
    kMessEpoch = 21
  };
  enum eMessageType
  {
    kData  = 0,
    kEpoch = 1,
    kNone
  };
  enum class eMessageVersion : int
  {
    kMessLegacy = 2,  /// unpacker version for 2-board FASPRO+GETS HW
    kMess24     = 3,  /// unpacker version for 1-board FASPRO HW first used 18.06.24 (mCBM)
    kMessNoDef        /// default unpacker version
  };
  /** @brief Create the Cbm Trd Unpack AlgoBase object */
  CbmTrdUnpackFaspAlgo();

  /** @brief Destroy the Cbm Trd Unpack Task object */
  virtual ~CbmTrdUnpackFaspAlgo();

  /** @brief Copy constructor - not implemented **/
  CbmTrdUnpackFaspAlgo(const CbmTrdUnpackFaspAlgo&) = delete;

  /** @brief Assignment operator - not implemented **/
  CbmTrdUnpackFaspAlgo& operator=(const CbmTrdUnpackFaspAlgo&) = delete;


  /** @brief Data structure for unpacking the FASP word */
  class CbmTrdFaspMessage {
   public:
    CbmTrdFaspMessage()                         = default;
    CbmTrdFaspMessage(const CbmTrdFaspMessage&) = default;
    virtual ~CbmTrdFaspMessage()                = default;
    CbmTrdFaspMessage(uint8_t rob, uint8_t asic, uint8_t c, uint8_t typ, uint8_t t, uint16_t d, uint8_t l);
    virtual int getFaspIdMod() const { return fasp + rob * NFASPCROB; }
    virtual eMessageType getType(uint32_t w) const;
    virtual void print() const;
    virtual void readDW(uint32_t w);
    virtual void readEW(uint32_t w);

   public:
    eMessageVersion version = eMessageVersion::kMessLegacy;  ///< format version
    uint8_t ch              = 0;                             ///< ch id in the FASP
    uint8_t type            = 0;                             ///< message type 0 = data, 1 = epoch
    uint8_t tlab            = 0;                             ///< time of the digi inside the epoch
    uint16_t data           = 0;                             ///< ADC value
    uint32_t epoch          = 0;                             ///< epoch id (not used for the moment)
    uint32_t mod            = 0;                             ///< full module address according to CbmTrdAddress
    uint8_t rob             = 0;                             ///< Read-Out unit id in the module
    uint8_t elink           = 0;  ///< optical link for read-out unit (up or down, starting with v2024)
    uint8_t fasp            = 0;  ///< FASP id in the module
  };
  class CbmTrdFaspMessage24 : public CbmTrdFaspMessage {
   public:
    CbmTrdFaspMessage24();
    virtual eMessageType getType(uint32_t w) const;
    virtual void readDW(uint32_t w);
    virtual void readEW(uint32_t w);
  };
  /** Access the asic parameter list, read-only*/
  virtual const CbmTrdParSetAsic* GetAsicPar() const { return &fAsicSet; }
  /**
   * @brief Get the requested parameter containers.
   * Return the required parameter containers together with the paths to the ascii
   * files to.
   *  
   * @param[in] std::string geoTag as used in CbmSetup
   * @param[in] std::uint32_t runId for runwise defined parameters
   * @return fParContVec
  */
  virtual std::vector<std::pair<std::string, std::shared_ptr<FairParGenericSet>>>*
  GetParContainerRequest(std::string geoTag, std::uint32_t runId);
  /** @brief Set a predefined monitor 
   *  @param monitor predefined unpacking monitor */
  void SetMonitor(std::shared_ptr<CbmTrdUnpackFaspMonitor> monitor) { fMonitor = monitor; }

  /** @brief Check and assure there are no data left-overs */
  uint32_t ResetTimeslice();

 protected:
  /** @brief Get message type from the FASP word */
  eMessageType mess_type(uint32_t wd);
  bool pushDigis(std::vector<CbmTrdUnpackFaspAlgo::CbmTrdFaspMessage> messages, const uint16_t mod_id);
  /** @brief Time offset for digi wrt the TS start, expressed in 80 MHz clks. It contains:
   *  - relative offset of the MS wrt the TS
   *  - FASP epoch offset for current CROB
   *  - TRD2D system offset wrt to experiment time (e.g. Bmon)
   */
  ULong64_t fTime = 0;

  /** @brief Finish function for this algorithm base clase */
  void finish()
  {
    if (fMonitor) fMonitor->Finish();
  }

  /**
   * @brief Additional initialisation function for all BaseR derived algorithms.
   * 
   * @return Bool_t initOk
  */
  virtual Bool_t init() { return kTRUE; }

  // Initialise par set, the base function handles the casting to distribute the pointers to their explicit functions

  /**
   * @brief Handles the distribution of the hidden derived classes to their explicit functions.
   * 
   * @param parset
   * @return Bool_t initOk
  */
  Bool_t initParSet(FairParGenericSet* parset);

  /**
   * @brief Unpack a given microslice.
   *
   * @param ts timeslice pointer
   * @param icomp index to the component to be unpacked
   * @param imslice index of the microslice to be unpacked
   * @return true if all is fine
   *
   * @remark The content of the µslice can only be accessed via the timeslice. Hence, we need to pass the pointer to the full timeslice
  */
  bool unpack(const fles::Timeslice* ts, std::uint16_t icomp, UInt_t imslice);

  /**
   * @brief Finalize component (e.g. copy from temp buffers)
  */
  void FinalizeComponent();

  // Constants
  /** @brief Bytes per FASP frame stored in the microslices (32 bits words)*/
  static const std::uint8_t fBytesPerWord = 4;

 private:
  std::map<uint16_t, std::pair<uint16_t, uint16_t>>* fCompMap        = nullptr;  ///> Map eq_id -> (mod_id, crob_id)
  std::array<std::vector<CbmTrdDigi>, NFASPMOD* NFASPCH> fDigiBuffer = {
    {}};  ///> Buffered digi for each pad in CROB component
  /** @brief Potential (online) monitor for the unpacking process */
  std::shared_ptr<CbmTrdUnpackFaspMonitor> fMonitor = nullptr;
  std::vector<uint16_t> fModuleId = {};  ///> list of modules for which there is are calibration parameters
  CbmTrdParSetAsic fAsicSet;
  CbmTrdParSetDigi* fDigiSet = nullptr;
  CbmTrdFaspMessage* fMess   = nullptr;  ///> current message version

  ClassDef(CbmTrdUnpackFaspAlgo, 4)  // unpack FASP read-out detectors
};

#endif  // CbmTrdUnpackFaspAlgo_H
