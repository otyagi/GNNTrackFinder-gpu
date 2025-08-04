/* Copyright (C) 2018-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer] */

#ifndef CBMTRDPARSETASIC_H
#define CBMTRDPARSETASIC_H

#include "CbmTrdParSet.h"  // for CbmTrdParSet

#include <Rtypes.h>      // for THashConsistencyHolder, ClassDef
#include <RtypesCore.h>  // for Int_t, Bool_t, Option_t, UChar_t

#include <map>     // for map
#include <vector>  // for vector

class CbmTrdParAsic;
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
class CbmTrdParSetAsic : public CbmTrdParSet {
public:
  /** Standard constructor **/
  CbmTrdParSetAsic(const char* name = "CbmTrdParSetAsic", const char* title = "TRD ASIC parameters",
                   const char* context = "Default");

  /** \brief Destructor **/
  virtual ~CbmTrdParSetAsic() { ; }

  /** \brief Reset all parameters **/
  virtual void clear() { ; }

  /** \brief Search for the module in the setup parameters by equipement id 
   * \param[in] eqid equipment id from HW
   * \param[out] rob_id index of the ROB (CROB in legacy mode) on the module
   * \param[out] lnk_id index of optical link on the ROB (not used in legacy mode); 0 for down, 1 for up  
   * \return module id in the setup
   */
  virtual int FindModuleByEqId(uint16_t eqid, uint8_t& rob_id, uint8_t& lnk_id) const;
  /** \brief Build the ASICs par for the current module from the info stored in the param file 
   * It applies to the list of ASICs.
   * \param module ASICs par (FASP or SPADIC)
   */
  Bool_t getParams(FairParamList*);
  /** \brief Store the ASICs par info for the current module into the param file 
   * It applies to the list of ASICs.
   * \param module ASICs par (FASP or SPADIC)
   */
  void putParams(FairParamList*);
private:
  CbmTrdParSetAsic(const CbmTrdParSetAsic& ref);
  const CbmTrdParSetAsic& operator=(const CbmTrdParSetAsic& ref);

  ClassDef(CbmTrdParSetAsic,
           1);  // The set of ASIC settings for all TRD modules
};
#endif
