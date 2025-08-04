/* Copyright (C) 2020-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pascal Raisig, Florian Uhlig [committer], Alexandru Bercuci */

/*
 * Purpose: Reference class for global definitions used within the CbmTrd project
 * -----
 */

#ifndef CBMTRDDEFS_H
#define CBMTRDDEFS_H

#include "Rtypes.h"

namespace cbm::trd
{
  enum class eAsic : int
  {
    kSpadic = 0  ///< SPADIC type definition
      ,
    kFasp = 1  ///< FASP ASIC definition
      ,
    kNotSet  ///< ASIC not set / recognized
  };
  enum class ePadPlane : int
  {
    k1d = 0  ///< rectangular 1D case
      ,
    k2d = 1  ///< triangular 2D case
      ,
    kNotSet  ///< pad=plane not set / recognized
  };
  enum class eWindow : int
  {
    kThin = 0  ///< 1D case (Al+mylar)
      ,
    kThick  ///< 2D case (Kapton + C + HC)
      ,
    kNotSet  ///< window not set / recognized
  };
  enum class eGas : int
  {
    kAr = 0  ///< ArCO2 active gas
      ,
    kXe  ///< XeCO2 active gas
      ,
    kNotSet  ///< active gas not set / recognized
  };
  /** 16 bits bit map for module configuration
   * [0] - chamber's pad-plane type; 0 rectangular pads, 1 triangular pads, \see SetRO
   * [1] - chamber's FEE type; 0 SPADIC, 1 FASP, \see SetFEE
   * [2] -  
   * [3] -  
   * [4] -  
   * [5] -  
   * [6] -  
   * [7] -  
   */
  enum eModuleConfig
  {
    kPPtyp = 0  ///< toggle pad-plane type of the chamber
      ,
    kFEEtyp = 1  ///< toggle FEE type for the module
  };

  enum class eModuleTypes1D : int
  {
    kHighChDensitySmallR = 1,
    kLowChDensitySmallR  = 3,
    kHighChDensityLargeR = 5,
    kLowChDensityLargeR  = 7,
    kMcbmModule =
      8  // FIXME moduleType 8 has multiple definitions, check if non mCbm definitions are really needed. - PR 03/25/2020
      ,
    kNmoduleTypes = 5  // REMARK this number has to be updated by hand!
  };                   ///< Enum for moduleTypes of the rectangular TrdModules as used in geometry files.
  struct FEB {
    int nasic     = 0;  ///< no of asics / feb
    int nchannels = 0;  ///< no of channels / asic
    int nmax      = 0;  ///< max no of febs on the module
  };
  struct READOUT {
    int nsec       = 0;     ///< no of sectors on the pad-plane
    int ncol       = 0;     ///< no of (rectangular - pads) column
    int nrow       = 0;     ///< no of pad rows
    int nasic      = 0;     ///< no of ASICs to read-out the pad-plane
    int ndaq       = 0;     ///< no of concentrators (e.g. CROB) to read-out the pad-plane
    float sizex    = 0.;    ///< size of active area along wires
    float sizey[3] = {0.};  ///< size of active area across wires
  };
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
  // array of pad geometries in the TRD1D
  static READOUT mod1D[9] = {
    // module type 0 dummy
    {0, 0, 0, 0, 0, 0., {0., 0., 0.}},
    // module type 1
    // number of pads:  80 x 32 = 2560
    // pad size sector 1:  0.68 cm x  1.75 cm =  1.18 cm2
    // pad size sector 0:  0.68 cm x  1.50 cm =  1.01 cm2
    {3, 80, 32, 80, 5, 54., {6.0, 42.0, 6.0}},
    // module type 2
    // number of pads:  80 x 16 = 1280
    // pad size sector 1:  0.68 cm x  3.50 cm =  2.36 cm2
    // pad size sector 0:  0.68 cm x  3.25 cm =  2.19 cm2
    {3, 80, 16, 80, 5, 54., {13.0, 28.0, 13.0}},
    // module type 3
    // number of pads:  80 x  8 =  640
    // number of asic:  20
    // pad size sector 1:  0.68 cm x  6.75 cm =  4.56 cm2
    // pad size sector 0:  0.68 cm x  6.75 cm =  4.56 cm2
    {1, 80, 8, 20, 1, 54., {54., 0., 0.}},
    // module type 4
    // number of pads:  80 x  8 =  640
    // pad size sector 1:  0.68 cm x  6.75 cm =  4.56 cm2
    // pad size sector 0:  0.68 cm x  6.75 cm =  4.56 cm2
    {1, 80, 8, 20, 1, 54., {54., 0., 0.}},
    // module type 5
    // number of pads: 144 x 24 = 3456
    // number of asic: 36
    // pad size sector 1:  0.67 cm x  4.00 cm =  2.67 cm2
    // pad size sector 0:  0.67 cm x  4.00 cm =  2.67 cm2
    {1, 144, 24, 108, 2, 96., {96.0, 0., 0.}},
    // module type 6
    // number of pads: 144 x 16 = 2304
    // pad size sector 1:  0.67 cm x  6.00 cm =  4.00 cm2
    // pad size sector 0:  0.67 cm x  6.00 cm =  4.00 cm2
    {1, 144, 16, 72, 2, 96., {96.0, 0., 0.}},
    // module type 7
    // number of pads: 144 x  8 = 1152
    // number of asic: 36
    // pad size sector 1:  0.67 cm x 12.00 cm =  8.00 cm2
    // pad size sector 0:  0.67 cm x 12.00 cm =  8.00 cm2
    {1, 144, 8, 36, 2, 96., {96.0, 0., 0.}},
    // module type 8
    // number of pads: 144 x  4 =  576
    // pad size sector 1:  0.67 cm x 24.00 cm = 16.00 cm2
    // pad size sector 0:  0.67 cm x 24.00 cm = 16.00 cm2
    {1, 144, 4, 18, 1, 96., {96.0, 0., 0.}}};

  // array of pad geometries in the TRD2D
  static const READOUT mod2D[3] = {
    // module type 1 (TRD2D @ CBM)
    // number of pads:  72 x 20 = 1440
    // pad size:  0.75 cm x  2.70 cm =  2.03 cm2
    // number of asic:  180
    // number of crob:  5
    {1, 72, 20, 180, 5, 54.0, {54.0, 0., 0.}},
    // module type 9 (TRD2012 @ mCBM)
    // number of pads:  72 x 20 = 1440
    // pad size:  0.75 cm x  2.79 cm =  2.03 cm2
    {1, 72, 20, 180, 5, 54.0, {55.8, 0., 0.}},
    // module type 10 (TRD2010)
    // number of pads:  32 x  3 =   96
    // pad size :  0.72 cm x  2.72 cm =  1.96 cm2
    {1, 32, 3, 6, 1, 23.04, {8.16, 0., 0.}}};

  static constexpr FEB faspFeb[2] = {
    {6, 16, 30},  ///< FASPRO v1
    {12, 16, 15}  ///< FASPRO v2
  };
#pragma GCC diagnostic pop

  /** \brief Inquire the FEE read-out type of the module
   * \return false for SPADIC and true for FASP
   */
  bool HasFaspFEE(uint16_t config);
  bool HasSpadicFEE(uint16_t config);
  /** \brief Inquire the pad-plane type of the chamber
   * \return false for TRD-1D and true for TRD-2D
   */
  bool HasPadPlane2D(uint16_t config);
  bool HasPadPlane1D(uint16_t config);
  /** \brief Define the read-out FEE type of the module
   * \param[in] set true for FASP and false [default] for SPADIC
   */
  void SetFEE(uint16_t config, bool fasp = true);
  /** \brief Define the pad-plane type of the chamber
   * \param[in] set true for TRD-2D and false [default] for TRD-1D
   */
  void SetPP(uint16_t config, bool twod = true);
}  // namespace cbm::trd

#endif
