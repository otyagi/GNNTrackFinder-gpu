/* Copyright (C) 2025 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Oddharak Tyagi [committer] */
   #ifndef CBM_ALGO_GNN_GPU_DEVICEIMAGE_H
   #define CBM_ALGO_GNN_GPU_DEVICEIMAGE_H
   #include <xpu/device.h>
   
   namespace cbm::algo::ca
   {
     struct GPUReco_Gnn : xpu::device_image {
     };
   }  // namespace cbm::algo::ca
   
   #endif
   