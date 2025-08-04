/* Copyright (C) 2018-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer], Alexandru Bercuci*/

#ifndef CBMTRDPARFASP_H
#define CBMTRDPARFASP_H

#define NFASPMOD 180
#define NCROBMOD 5
#define NFASPCROB NFASPMOD / NCROBMOD
#define NFASPCH 16
// The size of the parameter translation is calculated as follows:
// size = detId + chMask + NFASPCH*(chAddress + pileUp + threshold + delay)
#define NFASPPARS 2 + 4 * NFASPCH

#define FASP_EPOCH_LENGTH 128

#include "CbmTrdParAsic.h"  // for CbmTrdParAsic

#include <Rtypes.h>      // for THashConsistencyHolder, ClassDef, TESTBIT
#include <RtypesCore.h>  // for Int_t, Double_t, Bool_t, Float_t, UChar_t
#include <TObject.h>     // for TObject

#include <vector>  // for vector

class FairParamList;
class TArrayI;

/** \brief Definition of FASP channel calibration container **/
class CbmTrdParFaspChannel : public TObject {
  friend class CbmTrdParFasp;

public:
  static constexpr double fgkSgmCh = 10;  ///< generic width of a pulser signal for FASP channels

  enum CbmTrdParFaspChannelDef
  {
    kPair = 0  //< pad pairing type definition see SetPairing()
      ,
    kMask = 1  //< pad masking. See SetMask()
  };
  /** \brief   Parametrization of a FASP channel based on CADENCE simulations from 12.01.2018 and 
   * parabolic parametrization of dt(signal). 
   */
  CbmTrdParFaspChannel(Int_t pup = 300, Int_t ft = 14, Int_t thr = 600, Int_t smin = 2586, Float_t dtPar = 4.181e-6);

  Int_t GetFlatTop() const { return fFlatTop; }
  Int_t GetMinDelaySignal() const { return fMinDelaySignal; }
  Float_t GetMinDelayParam() const { return fMinDelayParam; }
  Int_t GetPileUpTime() const { return fPileUpTime; }
  Int_t GetThreshold() const { return fThreshold; }

  /** \brief Query pad pairing type.*/
  Bool_t HasPairingR() const { return TESTBIT(fConfig, kPair); }
  Bool_t HasPairingT() const { return !TESTBIT(fConfig, kPair); }
  bool IsMasked() const { return TESTBIT(fConfig, kMask); }

  void Print(Option_t* opt = "") const;
  /** \brief Specify pad pairing type.
   *\param[in] rect if rect=kTRUE rectangular pairing; tilt otherwise 
   */
  void SetPairing(Bool_t rect) { rect ? SETBIT(fConfig, kPair) : CLRBIT(fConfig, kPair); }
  /** \brief Mask channel for processing.*/
  void SetMask(Bool_t set) { set ? SETBIT(fConfig, kMask) : CLRBIT(fConfig, kMask); }

protected:
  UShort_t fPileUpTime;  ///< Signal formation time in ns
  UChar_t fFlatTop;      ///< Length of Flat-Top in clocks
  UChar_t fConfig;       ///< configuration bit map

  // threshold
  UShort_t fThreshold;  ///< Threshold in ADC units

  // timming
  UShort_t fMinDelaySignal;  ///< Signal in ADC for minimum delay i.e. fPileUpTime
  Float_t fMinDelayParam;    ///< Factor of parabolic dependence dt=fdt*(s-s0)^2 to calculate trigger

  ClassDef(CbmTrdParFaspChannel,
           1)  // Definition of FASP channel calibration container
};

/** \brief Definition of FASP parameters **/
class CbmTrdParFasp : public CbmTrdParAsic {
public:
  CbmTrdParFasp(Int_t address = 0, Int_t FebGrouping = -1, Double_t x = 0, Double_t y = 0, Double_t z = 0);
  virtual ~CbmTrdParFasp() { ; }
  /** \brief Query the calibration for one FASP RO channel
   * \param pad_address the index of the pad on the module [row*ncol+col]
   * \param pair 0 for tilt and 1 for rect
   */
  const CbmTrdParFaspChannel* GetChannel(Int_t pad_address, UChar_t pair) const;
  /** \brief Query the calibration for one FASP RO channel
   * \param chId the index of the channel in the current FASP [0 - 15]
   */
  const CbmTrdParFaspChannel* GetChannel(Int_t chId) const;
  virtual Int_t GetNchannels() const { return NFASPCH; };
  /** \brief Return the global RO channel for curent FASP and channel index
   * \param chId the index of the channel in the current FASP [0 - 15]
   */
  virtual Int_t GetChannelAddress(Int_t chId) const
  {
    return ((chId < 0 || chId >= GetNchannels()) ? 0 : fChannelAddresses[chId]);
  }
  virtual uint32_t GetChannelMask() const;
  virtual bool IsChannelMasked(int ch) const;
  int GetPadAddress(Int_t ich) const { return int(0.5 * GetChannelAddress(ich)); }
  Double_t GetSizeX() const { return fgSizeX; }
  Double_t GetSizeY() const { return fgSizeY; }
  Double_t GetSizeZ() const { return fgSizeZ; }
  virtual void LoadParams(FairParamList* l);
  /** \brief Load ASIC parameters from param file. Called from e.g. CbmTrdParSetAsic*/
  void LoadParams(int* vals);
  virtual void Print(Option_t* opt = "") const;
  /** \brief Load FASP calibration parameters for a specific channel
   *\param ch Address of the channel inside FASP
   *\param par pointer to the list of parameters 
   */
  virtual Bool_t SetCalibParameters(Int_t ch, Double_t const* par);
  virtual void SetChannelMask(uint32_t mask);

private:
  static Double_t fgSizeX;  ///< FASP half size in x [cm]
  static Double_t fgSizeY;  ///< FASP half size in y [cm]
  static Double_t fgSizeZ;  ///< FASP half size in z [cm]

  CbmTrdParFaspChannel fCalib[NFASPCH];  ///< calibration map for FASP channels

  ClassDef(CbmTrdParFasp, 1)  // Definition of FASP ASIC parameters
};

#endif
