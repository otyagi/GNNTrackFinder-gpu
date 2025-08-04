/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#pragma once

#include "Definitions.h"
#include "LandauTable.h"
#include "yaml/Property.h"
#include "yaml/Yaml.h"

#include <xpu/defines.h>

namespace cbm::algo::sts
{
  struct HitfinderPars {
    struct Asic {
      int nAdc;
      float dynamicRange;
      float threshold;
      float timeResolution;
      float deadTime;
      float noise;
      float zeroNoiseRate;

      XPU_D float AdcToCharge(unsigned short adc) const
      {
        return threshold + dynamicRange / float(nAdc) * (float(adc) + 0.5f);
      }

      CBM_YAML_PROPERTIES(
        yaml::Property(&Asic::nAdc, "nAdc"),
        yaml::Property(&Asic::dynamicRange, "dynamicRange"),
        yaml::Property(&Asic::threshold, "threshold"),
        yaml::Property(&Asic::timeResolution, "timeResolution"),
        yaml::Property(&Asic::deadTime, "deadTime"),
        yaml::Property(&Asic::noise, "noise"),
        yaml::Property(&Asic::zeroNoiseRate, "zeroNoiseRate")
      );
    };

    struct ModuleTransform {
      // Rotation + translation matrix to transform
      // local module coordinates into global coordinate system.
      // No need for fancy math types here. These values are just copied
      // and moved to the GPU.
      // TODO: thats a lie, should use glm::mat3x4
      std::array<float, 9> rotation;  // 3x3 matrix
      std::array<float, 3> translation;

      CBM_YAML_PROPERTIES(yaml::Property(&ModuleTransform::rotation, "rotation", "Rotation matrix", YAML::Flow),
                          yaml::Property(&ModuleTransform::translation, "translation", "Translation vector",
                                         YAML::Flow));
    };

    struct Module {
      int32_t address;
      float dY;
      float pitch;
      float stereoF;
      float stereoB;
      float lorentzF;
      float lorentzB;
      ModuleTransform localToGlobal;

      CBM_YAML_PROPERTIES(yaml::Property(&Module::address, "address", "Hardware Address", YAML::Hex),
                          yaml::Property(&Module::dY, "dY"), yaml::Property(&Module::pitch, "pitch"),
                          yaml::Property(&Module::stereoF, "stereoF"), yaml::Property(&Module::stereoB, "stereoB"),
                          yaml::Property(&Module::lorentzF, "lorentzF"), yaml::Property(&Module::lorentzB, "lorentzB"),
                          yaml::Property(&Module::localToGlobal, "localToGlobal"));
    };

    Asic asic;
    int nChannels;
    std::vector<Module> modules;
    LandauTable landauTable;  // Landau table for hitfinder, read from a seperate file

    CBM_YAML_PROPERTIES(
      yaml::Property(&HitfinderPars::asic, "asic",
                     "Asic definitions. Currently assumes same parameters for all asics."),
      yaml::Property(&HitfinderPars::nChannels, "nChannels",
                     "Total number of channels per module. Hitfinder assumes nChannels / 2 channels per side."),
      yaml::Property(&HitfinderPars::modules, "modules"));
  };
}  // namespace cbm::algo::sts

CBM_YAML_EXTERN_DECL(cbm::algo::sts::HitfinderPars);
