/* Copyright (C) 2023-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, Dominik Smith [committer], Alex Bercuci */

#pragma once

#include "UnpackMS.h"
#include "yaml/Property.h"
#include "yaml/Yaml.h"

#include <map>
#include <sstream>
#include <utility>
#include <vector>

namespace cbm::algo::trd2d
{


  /** @class ReadoutConfig
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 3 March 2022
   ** @brief Provides the hardware-to-software address mapping for the CBM-TRD2D
   **
   ** The hardware address as provided in the raw data stream is specified in terms of the
   ** equipment identifier (specific to one FLES component) and the elink number with in
   ** component. This is to be translated into the module address and the ASIC number within
   ** the module.
   ** The mapping of the two address spaces is hard-coded in this class.
   **/

  class ReadoutSetup {

   public:
    struct CompMapping {
      uint16_t moduleId;
      uint8_t fiberId;

      CBM_YAML_FORMAT(YAML::Flow);
      CBM_YAML_PROPERTIES(yaml::Property(&CompMapping::moduleId, "moduleId", "Module ID"),
                     yaml::Property(&CompMapping::fiberId, "fiberId", "Optical Fibre ID"));
    };

    struct ChanMapping {
      int32_t padAddress;   /// map pad and pairing to FASP channel
      bool maskFlag;        /// HW mask flag for channel
      int8_t tOffset;       /// time correction in clk
      uint16_t lThreshold;  /// SW threshold for ringing channels

      CBM_YAML_FORMAT(YAML::Flow);
      CBM_YAML_PROPERTIES(
        yaml::Property(&ChanMapping::padAddress, "address", "Pad address"),
        yaml::Property(&ChanMapping::maskFlag, "mask", "Channel masking flag"),
        yaml::Property(&ChanMapping::tOffset, "toff", "Channel wise time offset"),
        yaml::Property(&ChanMapping::lThreshold, "thres", "SW masking by threshold"));
    };

    /** @brief Constructor **/
    ReadoutSetup();


    /** @brief Destructor **/
    ~ReadoutSetup();


    /** @brief Equipment in the configuration
     ** @return Vector of equipment IDs
     **/
    std::vector<uint16_t> GetEquipmentIds();


    /** @brief Number of ASICS of a component
     ** @param Equipment ID
     ** @return Number of ASICS
     **/
    size_t GetNumAsics(uint16_t equipmentId);


    /** @brief Number of ASICS of a component
     ** @param Equipment ID
     ** @return List of ASICS linked to the curent ROB
     **/
    std::vector<uint8_t> GetAsicList(uint16_t equipmentId);


    /** @brief Number of channels of a component - ASIC pair
     ** @param Equipment ID
     ** @param ASIC ID
     ** @return Number of channels
     **/
    size_t GetNumChans(uint16_t equipmentId, uint16_t asicId);

    //// TO DO: Check uint sizes (for asic Id etc).

    /** @brief API: Mapping from component to pair (module id, crob id)
     ** @param equipId     Equipment identifier (component)
     ** @return pair (module id, crob id)
     */
    CompMapping CompMap(uint16_t equipId);


    /** @brief API: Mapping from component, asic and channel to tuple (pad address, R pairing flag, daq offset)
     ** @param equipId     Equipment identifier (component)
     ** @param asic        ASIC number within component
     ** @param channel     Channel number within CROB
     ** @return tuple (pad address, R pairing flag, daq offset)
     */
    ChanMapping ChanMap(uint16_t equipId, uint16_t asic, uint16_t chan);


    /** @brief Debug output of readout map **/
    std::string PrintReadoutMap();

    /** @brief Initialisation of readout map **/
    void InitComponentMap(const std::map<uint32_t, std::vector<uint16_t>>& map);

    /** @brief Initialisation of channel map **/
    void InitChannelMap(
      const std::map<size_t, std::map<size_t, std::map<size_t, std::tuple<int32_t, bool, int16_t, uint16_t>>>>&
        channelMap);

    /** @brief Get system time offset **/
    int64_t GetSystemTimeOffset() { return fSystemTimeOffset; };

    /** @brief Get system time offset **/
    void SetSystemTimeOffset(int64_t offsetNs) { fSystemTimeOffset = offsetNs; };

   private:
    // --- System time offset
    int64_t fSystemTimeOffset = 0;

    // --- TRD2D readout map
    // --- Map index: (equipment), map value: (module id, crob id)
    std::map<uint16_t, CompMapping> fReadoutMap = {};  //!

    // --- TRD2D channel map
    // --- Map index: (equipment, asic, chan), map value: (pad address, mask flag, daq offset)
    std::map<uint16_t, std::map<uint8_t, std::vector<ChanMapping>>> fChannelMap = {};  //!

    CBM_YAML_PROPERTIES(
      yaml::Property(&ReadoutSetup::fSystemTimeOffset, "timeOffset", "System time offset for TRD2D"),
      yaml::Property(&ReadoutSetup::fReadoutMap, "readoutMap", "Maps equipment to module and Optical fibre Id", YAML::Hex),
      yaml::Property(&ReadoutSetup::fChannelMap, "channelMap",
                                    "Maps equipment, ASIC and channel to pad address, mask flag and DAQ offset",
                                    YAML::Hex));
  };  // end struct cbm::algo::trd2d::ReadoutSetup

  class ReadoutCalib {

   public:
    struct ChanNoise {
      uint16_t tDelay;   /// time [clk] delay wrt to causing primary signal
      uint16_t tWindow;  /// time [clk] window to search for noisy signal
      uint16_t lDThres;  /// Threshold [ADU] for dynamic channel noise
      uint16_t lSThres;  /// threshold [ADU] for static channel noise (happens independently)

      CBM_YAML_PROPERTIES(
        yaml::Property(&ChanNoise::tDelay, "delay", "time delay wrt the causing signal"),
        yaml::Property(&ChanNoise::tWindow, "window", "time window for noise signal search"),
        yaml::Property(&ChanNoise::lDThres, "dnoise", "Max signal value for cutting dynamic noise signals"),
        yaml::Property(&ChanNoise::lSThres, "snoise", "Max signal value for cutting static noise signals"));
    };
    struct ChanDescriptor {
      bool maskFlag;    /// HW mask flag for channel
      int8_t tOffset;   /// time [clk] correction
      float baseline;   /// Baseline correction for channel
      float gainfee;    /// ASIC gain deviation for channel
      ChanNoise noise;  /// noise descriptor [ChanNoise]

      CBM_YAML_FORMAT(YAML::Flow);
      CBM_YAML_PROPERTIES(
        yaml::Property(&ChanDescriptor::maskFlag, "mask", "Channel masking flag"),
        yaml::Property(&ChanDescriptor::tOffset, "toff", "Channel wise time offset"),
        yaml::Property(&ChanDescriptor::baseline, "bline", "Baseline correction"),
        yaml::Property(&ChanDescriptor::gainfee, "gainfee", "Gain correction for FEE channel"),
        yaml::Property(&ChanDescriptor::noise,  "noise", "Dynamic noise descriptor"));
    };

    /** @brief Constructor **/
    ReadoutCalib();


    /** @brief Destructor **/
    ~ReadoutCalib();

    /** @brief Retrieve calibration for one channel
     * \param[in] modId module id according to geometry
     * \param[in] padId paired-pad id [0-2879] according to geometry
     * \return calibration in the struct ChanDescriptor
     */
    ChanDescriptor GetChannelFeeCalib(uint16_t modId, uint16_t padId);

    /** @brief Get system reference signal for calibration **/
    float GetSystemCalibSignal() { return fSystemCalibSignal; };

    /** @brief Debug output of readout map **/
    std::string PrintCalibMap();

   private:
    float fSystemCalibSignal = 900.;  /// reference signal [ADU] to which the FEE gain is refereed

    // --- TRD2D-FEE calibration map
    // --- Map index: (module_id), map value: (array of channel calibration parameters)
    std::map<uint16_t, std::array<ChanDescriptor, NFASPMOD* NFASPCH>> fCalibMap = {};  //!

    CBM_YAML_PROPERTIES(
      yaml::Property(&ReadoutCalib::fSystemCalibSignal, "sCalib", "System wide calibrating signal for TRD2D"),
      yaml::Property(&ReadoutCalib::fCalibMap, "calibMap",
                                    "Maps of FEE calibration: mask, DAQ time offset, etc.",
                                    YAML::Hex));
  };  // end struct cbm::algo::trd2d::ReadoutCalib

}  // namespace cbm::algo::trd2d

CBM_YAML_EXTERN_DECL(cbm::algo::trd2d::ReadoutSetup);
CBM_YAML_EXTERN_DECL(cbm::algo::trd2d::ReadoutCalib);
