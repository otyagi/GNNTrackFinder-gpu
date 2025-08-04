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

namespace cbm::algo::tof
{

  /**
   * @brief Readout setup / Hardware cabling for TOF
   * Used to create the hardware mapping for the TOF unpacker.
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
    std::vector<i32> elink2Asic;
    std::vector<i32> elink2AsicInner;
    std::vector<i32> asic2PadI;

    CBM_YAML_PROPERTIES(yaml::Property(&ReadoutSetup::timeOffset, "timeOffset", "Time offset for TOF"),
                        yaml::Property(&ReadoutSetup::nFebsPerComponent, "nFebsPerComponent", "Number of FEBs per component"),
                        yaml::Property(&ReadoutSetup::nAsicsPerFeb, "nAsicsPerFeb", "Number of ASICs per FEB"),
                        yaml::Property(&ReadoutSetup::nChannelsPerAsic, "nChannelsPerAsic", "Number of channels per ASIC"),
                        yaml::Property(&ReadoutSetup::nCrobPerComponent, "nCrobPerComponent", "Number of CROBs per component"),
                        yaml::Property(&ReadoutSetup::crobs, "crobs", "Crobs", {}, YAML::Flow),
                        yaml::Property(&ReadoutSetup::eqIds, "eqIds", "Array to hold the unique IDs (equipment ID) for all TOF DPBs" , YAML::Flow, YAML::Hex),
                        yaml::Property(&ReadoutSetup::elink2Asic, "elink2Asic", "Mapping to eLink to ASIC number within CROB. Mapping is the same for each CROB. (size: nElinksPerCrob)" , YAML::Flow),
                        yaml::Property(&ReadoutSetup::elink2AsicInner, "elink2AsicInner", "add explanation" , YAML::Flow),
                        yaml::Property(&ReadoutSetup::asic2PadI, "asic2PadI", "Mapping in Readout chain PCBs; Map from GET4 channel to PADI channel (size: nChansPerFeb)" , YAML::Flow));

    size_t NComponents() const { return eqIds.size(); }
    size_t NCrobs() const { return crobs.size(); }
    i32 NFebsPerCrob() const { return nFebsPerComponent / nCrobPerComponent; }
    i32 NChansPerFeb() const { return nAsicsPerFeb * nChannelsPerAsic; }
    i32 NChansPerComponent() const { return nChannelsPerAsic * NElinksPerComponent(); }
    i32 NElinksPerCrob() const { return nAsicsPerFeb * NFebsPerCrob(); }
    i32 NElinksPerComponent() const { return NElinksPerCrob() * nCrobPerComponent; }
    i32 NCrobsPerComponent() const { return nCrobPerComponent; }

    bool CheckBmonComp(uint32_t uCompId) const { return ((uCompId & 0xFFF0) == 0xABF0); }
    //bool CheckInnerComp(uint32_t uCompId) const { return ((uCompId & 0xFFF0) == 0xBBC0); }
    bool CheckInnerComp(uint32_t uCompId) const { return ((uCompId & 0xFF00) == 0xBB00); }
  };


  class ReadoutConfig {

    using CROB = ReadoutSetup::CROB;

   public:
    /** @brief Constructor **/
    ReadoutConfig(const ReadoutSetup& pars);

    /** @brief Destructor **/
    ~ReadoutConfig();

    /** @brief System time offset for TOF
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

    // --- TOF elink time offsets
    // --- Map index: (equipment, elink), map value: (time offset)
    std::map<uint16_t, std::vector<uint64_t>> fTimeOffsetMap = {};

    // --- TOF readout map
    // --- Map index: (equipment, elink, channel), map value: (TOF address)
    std::map<uint16_t, std::vector<std::vector<uint32_t>>> fReadoutMap = {};

    /** @brief Initialisation of readout map **/
    void Init(const ReadoutSetup& pars);

    /// Mapping to eLink to ASIC number within DPB. Mapping is the same for each DPB.
    int32_t ElinkIdxToGet4Idx(uint32_t elink, const ReadoutSetup& pars);
    /// Mapping to eLink to ASIC number within DPB for 2024 inner TOF FEBs. Mapping is the same for each DPB.
    int32_t ElinkIdxToGet4IdxInner(uint32_t elink, const ReadoutSetup& pars);

    int32_t Get4ChanToPadiChan(uint32_t channelInFee, const ReadoutSetup& pars);

    std::vector<int32_t> fviRpcChUId = {};  // UID/address for each channel, build from type, side and module

    void BuildChannelsUidMap(const ReadoutSetup& pars);
    void BuildChannelsUidMapCbm(uint32_t& uCh, const CROB& crob);
    void BuildChannelsUidMapStar(uint32_t& uCh, const CROB& crob);
    void BuildChannelsUidMapCern(uint32_t& uCh, const CROB& crob);
    void BuildChannelsUidMapCera(uint32_t& uCh, const CROB& crob);
    void BuildChannelsUidMapStar2(uint32_t& uCh, const CROB& crob);
    void BuildChannelsUidMapStar2Inner(uint32_t& uCh, const CROB& crob);
    void BuildChannelsUidMapBuc(uint32_t& uCh, const CROB& crob);
  };

}  // namespace cbm::algo::tof

CBM_YAML_EXTERN_DECL(cbm::algo::tof::ReadoutSetup);
