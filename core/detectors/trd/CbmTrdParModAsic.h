/* Copyright (C) 2018-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Alexandru Bercuci [committer] */

#ifndef CBMTRDPARMODASIC_H
#define CBMTRDPARMODASIC_H

#include "CbmTrdDigi.h"    // for CbmTrdDigi::eCbmTrdAsicType
#include "CbmTrdParMod.h"  // for CbmTrdParSet

#include <Rtypes.h>      // for THashConsistencyHolder, ClassDef
#include <RtypesCore.h>  // for Int_t, Bool_t, Option_t, UChar_t

#include <map>     // for map
#include <vector>  // for vector

class CbmTrdParAsic;
class CbmTrdParFaspChannel;
class FairParamList;

/**
  * \brief Describe TRD module ASIC settings (electronic gain, delays, etc)
  * 
  * The following concepts are used :
  * - DAQ id : unique id of an ASIC composed of the format modId*1000+asicId. 
  * - modId : is calculated according to the CbmTrdAddress::GetModuleId(). 
  * - asicId : uniquely identify the ASICs in one module.
  * - chAddress : identify the read-out pad(s) connected to the ASIC. In the case of FASP it distinguish between R and T pairing    
  **/
class CbmTrdParModAsic : public CbmTrdParMod {
  friend class CbmTrdParSetAsic;

public:
  /** Standard constructor **/
  CbmTrdParModAsic(const char* name = "CbmTrdParModAsic", const char* title = "Generic list of ASICs for TRD modules");

  /** \brief Destructor **/
  virtual ~CbmTrdParModAsic() { clear(); }

  /** \brief Reset all parameters **/
  virtual void clear();
  /** \brief Access the calibration objects describing the two FASP channels allocated to a pad. FASP specific !
   * \param[in] pad Id of the pad according to position on the pad-plane
   * \param[in,out] tilt Parameter for FASP channel connected to the tilt paired pads. On output it can be:
   *    - nullptr : if channel is not installed
   *    - allocated : if channel installed. It is up to the user to retrieve more info, e.g. masking  
   *  \param[in,out] rect Parameter for FASP channel connected to the rectangular paired pads. The output follows the same convention as for the tilt parameters
   * \return true if pad is correctly defined
   */
  bool GetFaspChannelPar(int pad, const CbmTrdParFaspChannel*& tilt, const CbmTrdParFaspChannel*& rect) const;
  /** \brief Query the ASICs in the module for their DAQ address. 
   * It applies to the list of ASICs.
   * Returns the list of id of the ASICs within the module.   
   */
  virtual void GetAsicAddresses(std::vector<Int_t>* a) const;
  /** \brief Look for the ASIC which operates on a specific channel
   * It applies to the list of ASICs.
   *\param chAddress Pad address of the channel
   *\return id of the ASIC operating on the channel. -1 in case of failure
   */
  virtual Int_t GetAsicAddress(Int_t chAddress) const;
  /** \brief Look for the ASIC parameters of a given DAQ id
   * It applies to the list of ASICs.
   * \param address ASIC address from DAQ
   * \return A read-only pointer to the parameters 
   */
  virtual const CbmTrdParAsic* GetAsicPar(Int_t address) const;
  /** \brief Look for the ASIC parameters of a given DAQ id
   * It applies to the list of ASICs.
   * \param address ASIC address from DAQ
   * \return A read-write pointer to the parameters 
   */
  virtual CbmTrdParAsic* GetAsicPar(Int_t address);

  /** \brief Query the type of ASICs in the list*/
  virtual CbmTrdDigi::eCbmTrdAsicType GetAsicType() const;

  /** \brief Query the type of chamber*/
  virtual Int_t GetChamberType() const { return fType; }

  /** \brief Query the type of chamber*/
  virtual const int32_t* GetCrobAddresses() const { return fCrobAdd.data(); }

  /** \brief Query the existence of an equipment (CROB) by HW id on the current module params.
   * \return Returns the position of the equipment (CROB) on the current module; -1 in case of failure.  
   */
  virtual int HasEqId(uint16_t eqid, uint8_t& lnk) const;

  /** \brief Returns the number of INSTALLED ASICs for the current module
   * It applies to the list of ASICs.
   */
  virtual size_t GetNofAsicsOnModule() const { return fModPar.size(); }

  /** \brief Returns the DEFAULT number of ASICs for the current module
   */
  virtual int GetNofAsics() const;

  virtual void Print(Option_t* opt = "") const;
  /** \brief Initialize the ASIC parameters for DAQ id
   * It applies to the list of ASICs.
   */
  virtual void SetAsicPar(CbmTrdParAsic* p);
  virtual void SetChamberType(Int_t t) { fType = t; }
  /** \brief Initialize the CROB addresses as they are used in the DAQ*/
  virtual void SetCrobAddresses(int* addresses);

protected:
  CbmTrdParModAsic(const CbmTrdParModAsic& ref);
  const CbmTrdParModAsic& operator=(const CbmTrdParModAsic& ref);

  uint8_t fType;                          ///< type of chamber for current module
  std::vector<int32_t> fCrobAdd;          ///< ordered list of Crobs for current module
  std::map<int, CbmTrdParAsic*> fModPar;  ///< list of ASIC params for module
  ClassDef(CbmTrdParModAsic,
           1);  // The set of ASICs for one TRD modules
};
#endif
