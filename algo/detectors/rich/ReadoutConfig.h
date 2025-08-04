/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, Dominik Smith [committer] */

#ifndef ALGO_DETECTORS_RICH_READOUTCONFIG_H
#define ALGO_DETECTORS_RICH_READOUTCONFIG_H

#include <cstdint>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

namespace cbm::algo::rich
{


  /** @class ReadoutConfig
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 3 March 2022
   ** @brief Provides the hardware-to-software address mapping for the CBM-RICH
   **
   ** The hardware address as provided in the raw data stream is specified in terms of the
   ** equipment identifier (specific to one FLES component) and the trb address within
   ** component. This is to be translated to a tot shift value.
   ** The mapping of the address spaces is hard-coded in this class.
   **/

  class ReadoutConfig {

   public:
    /** @brief Constructor **/
    ReadoutConfig();


    /** @brief Destructor **/
    virtual ~ReadoutConfig();


    /** @brief Equipment in the configuration
     ** @return Vector of equipment IDs
     **/
    std::vector<uint16_t> GetEquipmentIds();


    /** @brief Number of elinks of a component
     ** @param Equipment ID
     ** @return Number of elinks
     **/
    size_t GetNumElinks(uint16_t equipmentId);


    /** @brief Total number of elinks for RICH
     ** @return Number of elinks
     **/
    size_t GetNumElinks();


    /** @brief API: Mapping from component, address and channel to tot shift
     ** @param equipId     Equipment identifier (component)
     ** @param address     trb address within component
     ** @param chan        channel within trb address
     ** @return tot shift
     */
    double Map(uint16_t equipmentId, uint32_t address, uint16_t chan);

    /** @brief API: Returns map from address and channel to tot shift for component
     ** @param equipId     Equipment identifier (component)
     ** @return map: (address, channel) - > tot shift
     */
    std::map<uint32_t, std::vector<double>> Map(uint16_t equipmentId);


    /** @brief Debug output of readout map **/
    std::string PrintReadoutMap();


   private:
    // --- RICH readout map
    // --- Map index: (equipment, trb address, channel), map value: (tot shift)
    std::map<uint16_t, std::map<uint32_t, std::vector<double>>> fReadoutMap = {};  //!


    /** @brief Initialisation of readout map **/
    void Init();
  };

}  // namespace cbm::algo::rich

#endif /* ALGO_DETECTORS_RICH_READOUTCONFIG_H_ */
