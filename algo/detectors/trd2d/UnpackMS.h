/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pascal Raisig, Alexandru Bercuci, Dominik Smith [committer] */
#pragma once

#include "CbmTrdDigi.h"
#include "MicrosliceDescriptor.hpp"
#include "UnpackMSBase.h"

#include <array>
#include <memory>
#include <sstream>

#define NFASPMOD 180
#define NROBMOD 5
#define NFASPROB NFASPMOD / NROBMOD
#define NFASPCH 16
#define NFASPPAD NFASPCH / 2

#define FASP_EPOCH_LENGTH 128  // the length in clks of FASP epoch [1600ns @ 40MHz]

namespace cbm::algo::trd2d
{
  enum class eMessageLength : int
  {
    kMessCh    = 4,
    kMessType  = 1,
    kMessTlab  = 7,
    kMessData  = 14,
    kMessFasp  = 6,
    kMessEpoch = 21
  };

  enum class eMessageVersion : uint8_t
  {
    kMessLegacy = 2,  /// unpacker version for 2-board FASPRO+GETS HW
    kMess24     = 3,  /// unpacker version for 1-board FASPRO HW first used 18.06.24 (mCBM)
  };

  enum class eMessageType : int
  {
    kData  = 0,
    kEpoch = 1,
    kNone
  };

  /** @brief Clock period of FASP  12.5ns. Use 25ns follow by division by 2*/
  static constexpr uint fAsicClockPeriod = 25;

  /** @brief Data structure for unpacking the FASP word */
  // Constants
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
  struct FaspMessage {
    FaspMessage()                   = default;
    FaspMessage(const FaspMessage&) = default;
    FaspMessage(uint8_t c, uint8_t typ, uint8_t t, uint16_t d, uint8_t asic);

    /** \brief Implementation of message type descriptor according to message version
     * \param w the message word
     */
    template<std::uint8_t mess_ver>
    static eMessageType getType(uint32_t w);

    std::string print() const;

    /** \brief Read DATA WORD and store the content locally
     * \param w the message word
     */
    template<std::uint8_t mess_ver>
    void readDW(uint32_t w);

    /** \brief Read EPOCH WORD and store the content locally
     * \param w the message word
     */
    template<std::uint8_t mess_ver>
    void readEW(uint32_t w);

    uint8_t ch        = 0;                    ///< ch id in the FASP
    eMessageType type = eMessageType::kNone;  ///< message type 0 = data, 1 = epoch (not used for the moment)
    uint8_t tlab      = 0;                    ///< time of the digi inside the epoch
    uint16_t data     = 0;                    ///< ADC value
    uint32_t epoch    = 0;                    ///< epoch id (not used for the moment)
    uint8_t fasp      = 0;                    ///< FASP id in the module
  };

  /** @struct UnpackChannelPar
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 31 January 2023
   ** @brief TRD2D Unpacking parameters for one Asic channel
   **/
  struct UnpackChannelPar {
    int32_t fPadAddress;      ///< Pad address for channel
    bool fMask;               ///< Flag for channel masking
    int8_t fDaqOffset     = 0;  ///< Time calibration parameter
    uint16_t fSignalThres = 0;  ///< Signal threshold to remove ringing channels
  };

  /** @struct UnpackAsicPar
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 31 January 2023
   ** @brief TRD2D Unpacking parameters for one Asic
   **/
  struct UnpackAsicPar {
    std::vector<UnpackChannelPar> fChanParams;  ///< Parameters for different channels
  };

  /** @struct NoiseChannelPar
   ** @author Alexandru Bercuci <abercuci@niham.nipne.ro>
   ** @since 10 February 2025
   ** @brief TRD2D noise parameters for one Asic channel
   **/
  struct NoiseChannelPar {
    uint16_t tDelay       = 0;  /// time [clk] delay wrt to causing primary signal
    uint16_t tWindow      = 0;  /// time [clk] window to search for noisy signal
    uint16_t lThreshold   = 0;  ///< Signal threshold for induced - dynamic noise
    uint16_t fSignalThres = 0;  ///< Signal threshold for independent - static noise
  };

  /** @struct CalibChannelPar
   ** @author Alexandru Bercuci <abercuci@niham.nipne.ro>
   ** @since 4 February 2025
   ** @brief TRD2D Calibration parameters for one Asic channel
   **/
  struct CalibChannelPar {
    bool fMask            = false;  ///< Flag for channel masking
    int8_t fDaqOffset     = 0;      ///< Time calibration parameter
    float fBaseline       = 0.;     ///< baseline correction
    float fGainFee        = -1.;    ///< gain correction wrt to reference
    NoiseChannelPar noise = {};     ///< noise channel parametrisation
  };

  /** @struct UnpackPar
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 31 January 2023
   ** @brief Parameters required for the TRD2D unpacking (specific to one component)
   **/
  struct UnpackPar {
    int32_t fSystemTimeOffset                    = 0;     ///< Time calibration parameter
    uint16_t fModId                              = 0;     ///< Module ID of component
    uint16_t fEqAdd                              = 0;     ///< Equipment (optical fiber) address [HEX]
    uint8_t fEqId                                = 0xff;  ///< Equipment (optical fiber) ID of component
    float fRefSignal                             = 900;   ///< reference signal for calibration
    std::map<uint8_t, UnpackAsicPar> fAsicParams = {};    ///< Parameters for each ASIC
    std::array<CalibChannelPar, NFASPMOD* NFASPCH> fCalibParams =
      {};  ///< Parameters for each ASIC channel for each module
    int toff[NFASPMOD * NFASPPAD];

    /** \brief Write to the debug stream the content of the current param object*/
    void dump() const;

    /** \brief Calculate the module wise FASP id from the FASP id provided at the level
     * of equipment Id (optical fibre in TRD2D case).
     * \param fasp_id index of fasp as written on the message
     * \return fasp id on the module as it is used in the parameter file 
     */
    template<uint8_t ver>
    uint8_t mapFaspId2Mod(uint8_t fasp_id) const;
  };


  /** @struct UnpackMoni
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 31 January 2023
   ** @brief Monitoring data for TRD2D unpacking
   **/
  struct UnpackMonitorData {
    uint32_t fNumSelfTriggeredData = 0;  ///< word fulfills data & 0x2000
    uint32_t fNumIncompleteDigis   = 0;  ///< incomplete digis left in pads after finalization
    uint32_t fNumErrEndBitSet      = 0;  ///< Corrupted data with end bit set

    bool HasErrors()
    {
      uint32_t numErrors = fNumErrEndBitSet;
      return (numErrors > 0 ? true : false);
    }
    std::string print()
    {
      std::stringstream ss;
      ss << "stats " << fNumSelfTriggeredData << " | " << fNumIncompleteDigis << " | errors " << fNumErrEndBitSet
         << " | ";
      return ss.str();
    }
  };

  /** @struct UnpackAux
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 24 May 2024
   ** @brief Auxiliary data for unpacking
   **/
  struct UnpackAuxData {
    ///// TO BE FILLED
  };

  /** @class UnpackMS
   ** @author Dominik Smith <d.smith@gsi.de>, Alex Bercuci <abercuci@niham.nipne.ro>
   ** @since 31 January 2023
   ** @brief Unpack algorithm for TRD2D
   **/
  template<std::uint8_t sys_ver>
  class UnpackMS : public UnpackMSBase<CbmTrdDigi, UnpackMonitorData, UnpackAuxData> {

   public:
    /** @brief Construct from parameters **/
    UnpackMS(const UnpackPar& pars) : fParams(pars) {}

    /** @brief Destructor **/
    ~UnpackMS() override = default;

    /** @brief Algorithm execution
     ** @param  msContent  Microslice payload
     ** @param  msDescr    Microslice descriptor
     ** @param  tTimeslice Unix start time of timeslice [ns]
     ** @return TRD digi data
     **/
    Result_t operator()(const uint8_t* msContent, const fles::MicrosliceDescriptor& msDescr,
                        const uint64_t tTimeslice) const override;

    /** @brief Set the parameter container
     ** @param params Pointer to parameter container
     **/
    void SetParams(std::unique_ptr<UnpackPar> params) { fParams = *(std::move(params)); }

   private:  // Types
    struct MsContext {
      UnpackMonitorData fMonitor;  ///< Container for monitoring data

      std::array<std::vector<CbmTrdDigi>, NFASPMOD* NFASPCH> fDigiBuffer = {
        {}};  ///> Buffered digi for each pad in CROB component
    };

   private:                  // members
    UnpackPar fParams = {};  ///< Parameter container

    bool pushDigis(std::vector<FaspMessage> messages, const uint64_t time, MsContext& ctx) const;

    /** @brief Finalize component (e.g. copy from temp buffers)  */
    std::vector<CbmTrdDigi> FinalizeComponent(MsContext& ctx) const;
  };


  /** @class UnpackMS<kMess24>
   ** @author Alex Bercuci <abercuci@niham.nipne.ro>
   ** @since 15 January 2025
   ** @brief Unpack algorithm specialization for TRD2D based on one-layered FEB design introduced in
   * 06.2024 (https://indico.gsi.de/event/20885/attachments/49263/72236/TRD2D-CDR-FEB.pdf)
   * - using different message format (\see \enum eMessageVersion) starting with kMess24
   * - remove time buffering of digi per channel as a new 32 channels ADC is used in the FEE
   **/
  template<>
  class UnpackMS<uint8_t(eMessageVersion::kMess24)> :
    public UnpackMSBase<CbmTrdDigi, UnpackMonitorData, UnpackAuxData> {

   public:
    /** @brief Construct from parameters **/
    UnpackMS(const UnpackPar& pars) : fParams(pars) {}

    /** @brief Destructor **/
    ~UnpackMS() override = default;


    /** @brief Algorithm execution
     ** @param  msContent  Microslice payload
     ** @param  msDescr    Microslice descriptor
     ** @param  tTimeslice Unix start time of timeslice [ns]
     ** @return TRD digi data
     **/
    Result_t operator()(const uint8_t* msContent, const fles::MicrosliceDescriptor& msDescr,
                        const uint64_t tTimeslice) const override;

    /** @brief Set the parameter container
     ** @param params Pointer to parameter container
     **/
    void SetParams(std::unique_ptr<UnpackPar> params) { fParams = *(std::move(params)); }

   private:  // Types
    struct MsContext {
      UnpackMonitorData fMonitor;  ///< Container for monitoring data
      std::array<std::vector<CbmTrdDigi>, NFASPMOD* NFASPPAD> fRobDigi = {
        {}};              ///> Buffered digi for each pad in one Epoch-ROB component
      FaspMessage fMess;  ///< encapsulation of the FASP message.
    };

   private:  // members
    bool pushDigis(std::vector<FaspMessage> messages, const uint64_t time, MsContext& ctx) const;

    UnpackPar fParams = {};  ///< Parameter container
  };

}  // namespace cbm::algo::trd2d
