/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer], Felix Weiglhofer */
#ifndef CBM_ALGO_DETECTOR_STS_READOUT_CONFIG_H
#define CBM_ALGO_DETECTOR_STS_READOUT_CONFIG_H

#include "Definitions.h"
#include "yaml/Property.h"
#include "yaml/Yaml.h"

#include <map>
#include <string>
#include <vector>

namespace cbm::algo::sts
{

  struct ChannelMaskSet;

  enum class FEBType
  {
    FEB8_1,
    FEB8_5
  };

  /**
   * @brief Readout setup / Hardware cabling for STS
   * Used to create the hardware mapping for the STS unpacker.
   */
  struct ReadoutSetup {

    struct Module {
      i32 address;
      i32 type;

      CBM_YAML_PROPERTIES(
        yaml::Property(&Module::address, "address", "HW address of module", YAML::Hex),
        yaml::Property(&Module::type, "type",
                         "Type 0 had the connector at the right side, type 1 at the left side. For type 0, the mapping "
                         "of FEB to module side as above applies, for type 1, it has to be inverted."));
    };

    struct Component {
      u16 equipmentId;
      std::vector<std::vector<i16>> feb2module;
      std::vector<std::vector<i16>> feb2moduleSide;
      std::vector<std::vector<bool>> febIsPulser;

      FEBType GetFEBType() const;
      size_t FEBsPerCrob() const { return feb2module.at(0).size(); }

      CBM_YAML_PROPERTIES(
        yaml::Property(&Component::equipmentId, "equipmentId",
                         "Equipment ID of component. Written to the data stream (MicrosliceDescriptor).", YAML::Hex),
        yaml::Property(&Component::feb2module, "feb2module",
                         "Mapping of FEB within CROB to module index (-1 = inactive)"),
        yaml::Property(&Component::feb2moduleSide, "feb2moduleSide",
                         "Mapping of FEB within CROB to module side (0 = left, 1 = right)"),
        yaml::Property(&Component::febIsPulser, "febIsPulser", "Flag if FEB is pulser (true) or not (false)"));
    };

    struct Elink {
      i16 toFeb;
      u32 toAsicFebA;
      u32 toAsicFebB;

      CBM_YAML_PROPERTIES(
        yaml::Property(&Elink::toFeb, "toFeb", "Mapping of elink to FEB within CROB (-1 = inactive)"),
        yaml::Property(&Elink::toAsicFebA, "toAsicFebA", "Mapping of eLink to ASIC for FEB Type A", YAML::Hex),
        yaml::Property(&Elink::toAsicFebB, "toAsicFebB", "Mapping of eLink to ASIC for FEB Type B", YAML::Hex));
    };

    u16 numAsicsPerFeb;
    std::vector<Module> modules;
    std::vector<Component> components;
    std::vector<Elink> elinksFeb8_1;
    std::vector<Elink> elinksFeb8_5;

    const std::vector<Elink>& GetElinks(FEBType type) const;

    CBM_YAML_PROPERTIES(yaml::Property(&ReadoutSetup::numAsicsPerFeb, "numAsicsPerFeb", "Number of ASICs per FEB"),
                        yaml::Property(&ReadoutSetup::modules, "modules", "Modules", {}, YAML::Flow),
                        yaml::Property(&ReadoutSetup::components, "components", "Components", {}, YAML::Flow),
                        yaml::Property(&ReadoutSetup::elinksFeb8_1, "elinksFeb8_1",
                                       "Elinks for FEB8_1 (1:1 elink:ASIC, 5 FEB / ROB)", {}, YAML::Flow),
                        yaml::Property(&ReadoutSetup::elinksFeb8_5, "elinksFeb8_5",
                                       "Elinks for FEB8_5 (5:1 elink:ASIC, 1 FEB / ROB)", {}, YAML::Flow));
  };

  /** @class ReadoutConfig
  ** @author Volker Friese <v.friese@gsi.de>
  ** @since 3 March 2022
  ** @brief Provides the hardware-to-software address mapping for the CBM-STS
  **
  ** The hardware address as provided in the raw data stream is specified in terms of the
  ** equipment identifier (specific to one FLES component) and the elink number with in
  ** component. This is to be translated into the module address and the ASIC number within
  ** the module.
  **/
  class ReadoutConfig {

   public:
    struct Entry {
      i32 moduleAddress;
      u16 asicNumber;
      bool pulser;
    };

    /** @brief Empty mapping **/
    ReadoutConfig() = default;

    /** @brief Constructor **/
    ReadoutConfig(const ReadoutSetup&, const ChannelMaskSet&);

    /** @brief Equipment in the configuration
   ** @return Vector of equipment IDs
      **/
    std::vector<u16> GetEquipmentIds();

    /** @brief Number of elinks of a component
   ** @param Equipment ID
      ** @return Number of elinks
      **/
    size_t GetNumElinks(u16 equipmentId);

    /** @brief Total number of elinks for STS
     ** @return Number of elinks
     **/
    size_t GetNumElinks();

    /** @brief API: Mapping from component and elink to address / ASIC number + pulser flag
      ** @param equipId     Equipment identifier (component)
      ** @param elink       Elink number within component
      ** @return (module address, ASIC number within module)
      */
    Entry Map(u16 equipId, u16 elink);

    /** @brief API: Mapping from component and elink to channel mask flags
     ** @param equipId     Equipment identifier (component)
     ** @param elink       Elink number within component
     ** @return (vector of mask flags for channels per asic)
     */
    std::vector<bool> MaskMap(uint16_t equipId, uint16_t elink);

    /** @brief API: Mapping from component and elink to minimum adc cut
     ** @param equipId     Equipment identifier (component)
     ** @param elink       Elink number within component
     ** @return adc cut
     */
    uint32_t AdcCutMap(uint16_t equipId, uint16_t elink);


    /** @brief Debug output of readout map **/
    std::string PrintReadoutMap();

   private:
    // --- STS readout map
    // --- Map index: (equipment, elink)
    std::map<u16, std::vector<Entry>> fReadoutConfig = {};  //!

    // --- STS adc cut map
    // --- Map index: (equipment, elink), map value: adc cut
    std::map<uint16_t, std::map<size_t, uint32_t>> fAdcCutMap = {};  //!

    // --- STS channel mask map
    // --- Map index: (equipment, elink), map value: (vector of mask flags for channels per asic)
    std::map<uint16_t, std::map<size_t, std::vector<bool>>> fMaskMap = {};  //!


    /** @brief Initialisation of readout map **/
    void Init(const ReadoutSetup&, const ChannelMaskSet&);
  };

}  // namespace cbm::algo::sts

CBM_YAML_EXTERN_DECL(cbm::algo::sts::ReadoutSetup);

#endif  // CBM_ALGO_DETECTOR_STS_READOUT_CONFIG_H
