/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#ifndef CBM_ALGO_CA_GPU_DEVICEIMAGE_H
#define CBM_ALGO_CA_GPU_DEVICEIMAGE_H
#include <xpu/device.h>

namespace cbm::algo::ca
{
  struct GPUReco : xpu::device_image {
  };
}  // namespace cbm::algo::ca

#endif
