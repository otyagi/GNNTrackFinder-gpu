/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer] */

#pragma once

#include "Definitions.h"
#include "yaml/Property.h"
#include "yaml/Yaml.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace cbm::algo::bmon
{

  /**
   * @brief Readout setup / Hardware cabling for BMon
   * Used to create the hardware mapping for the BMon unpacker.
   */
  struct ReadoutSetup {

    struct CROB {
      i32 moduleId;
      i32 rpcType;
      i32 rpcSide;
      i32 nRPC;
      i32 timeOffset;

      CBM_YAML_PROPERTIES(
        yaml::Property(&CROB::moduleId, "moduleId", "Unique ID of module the CROB resides in"),
        yaml::Property(&CROB::rpcType, "rpcType", "add explanation."),
        yaml::Property(&CROB::rpcSide, "rpcSide", "add explanation."),
        yaml::Property(&CROB::nRPC, "nRPC", "number of RPCs."),
        yaml::Property(&CROB::timeOffset, "timeOffset", "time offset for CROB"));
    };

    i32 timeOffset;
    i32 nFebsPerComponent;
    i32 nAsicsPerFeb;
    i32 nChannelsPerAsic;
    i32 nCrobPerComponent;
    std::vector<CROB> crobs;
    std::vector<u16> eqIds;

    CBM_YAML_PROPERTIES(yaml::Property(&ReadoutSetup::timeOffset, "timeOffset", "Time offset for BMon"),
                        yaml::Property(&ReadoutSetup::nFebsPerComponent, "nFebsPerComponent", "Number of FEBs per component"),
                        yaml::Property(&ReadoutSetup::nAsicsPerFeb, "nAsicsPerFeb", "Number of ASICs per FEB"),
                        yaml::Property(&ReadoutSetup::nChannelsPerAsic, "nChannelsPerAsic", "Number of channels per ASIC"),
                        yaml::Property(&ReadoutSetup::nCrobPerComponent, "nCrobPerComponent", "Number of CROBs per component"),
                        yaml::Property(&ReadoutSetup::crobs, "crobs", "Crobs", {}, YAML::Flow),
                        yaml::Property(&ReadoutSetup::eqIds, "eqIds", "Array to hold the unique IDs (equipment ID) for all BMON DPBs" , YAML::Flow, YAML::Hex));

    size_t NComponents() const { return eqIds.size(); }
    size_t NCrobs() const { return crobs.size(); }
    i32 NFebsPerCrob() const { return nFebsPerComponent / nCrobPerComponent; }
    i32 NChansPerFeb() const { return nAsicsPerFeb * nChannelsPerAsic; }
    i32 NChansPerComponent() const { return nChannelsPerAsic * NElinksPerComponent(); }
    i32 NElinksPerCrob() const { return nAsicsPerFeb * NFebsPerCrob(); }
    i32 NElinksPerComponent() const { return NElinksPerCrob() * nCrobPerComponent; }
    i32 NCrobsPerComponent() const { return nCrobPerComponent; }

    bool CheckBmonComp(uint32_t uCompId) const { return ((uCompId & 0xFFF0) == 0xABF0); }
    bool CheckInnerComp(uint32_t uCompId) const { return ((uCompId & 0xFFF0) == 0xBBC0); }
  };


  class ReadoutConfig {

   public:
    /** @brief Constructor **/
    ReadoutConfig(const ReadoutSetup& pars);

    /** @brief Destructor **/
    virtual ~ReadoutConfig();

    /** @brief System time offset for BMon
     ** @return Value of system time offset
     **/
    i32 GetSystemTimeOffset() const { return fTimeOffset; }

    /** @brief Equipment in the configuration
     ** @return Vector of equipment IDs
     **/
    std::vector<uint16_t> GetEquipmentIds();

    /** @brief Number of elinks of a component
     ** @param Equipment ID
     ** @return Number of elinks
     **/
    size_t GetNumElinks(uint16_t equipmentId);

    /** @brief API: Mapping from component and elink to addresses per channel
     ** @param equipId     Equipment identifier (component)
     ** @param elink       Elink number within component
     ** @return Vector of TOF addresses, indexed via channel number
     */
    std::vector<uint32_t> Map(uint16_t equipId, uint16_t elink);

    /** @brief API: Mapping from component and elink to time offset
     ** @param equipId     Equipment identifier (component)
     ** @param elink       Elink number within component
     ** @return Time Offset
     */
    int32_t GetElinkTimeOffset(uint16_t equipId, uint16_t elink);

   private:
    // --- BMon system time offset
    int32_t fTimeOffset = 0;

    // --- Bmon elink time offsets
    // --- Map index: (equipment, elink), map value: (time offset)
    std::map<uint16_t, std::vector<int32_t>> fTimeOffsetMap = {};

    // --- Bmon readout map
    // --- Map index: (equipment, elink, channel), map value: (TOF address)
    std::map<uint16_t, std::vector<std::vector<uint32_t>>> fReadoutMap = {};

    /** @brief Initialisation of readout map **/
    void Init(const ReadoutSetup& pars);

    std::vector<int32_t> fviRpcChUId = {};  ///< UID/address for each channel, build from type, side and module

    void BuildChannelsUidMap(const ReadoutSetup& pars);
    void BuildChannelsUidMapBmon(uint32_t& uCh, uint32_t uGbtx, const ReadoutSetup& pars);
    void BuildChannelsUidMapBmon_2022(uint32_t& uCh, uint32_t uGbtx, const ReadoutSetup& pars);
  };

}  // namespace cbm::algo::bmon
