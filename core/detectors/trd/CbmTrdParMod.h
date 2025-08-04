/* Copyright (C) 2018-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer], Alexandru Bercuci */

#ifndef CBMTRDPARMOD_H
#define CBMTRDPARMOD_H

#include <Rtypes.h>      // for THashConsistencyHolder, ClassDef
#include <TNamed.h>      // for TNamed

/** \brief Definition of generic parameters for one TRD module **/
class CbmTrdParMod : public TNamed {
public:
  CbmTrdParMod(const char* name = "CbmTrdParMod", const char* title = "TRD generic module definition");
  virtual ~CbmTrdParMod();

  virtual uint16_t GetConfig() const { return fConfig; }
  virtual int GetModuleId() const { return fModuleId; }
  virtual uint8_t GetVersion() const { return fVersion; }

  virtual void SetConfigId(uint16_t c) { fConfig = c; }
  virtual void SetModuleId(int m) { fModuleId = m; }
  virtual void SetVersion(uint8_t v) { fVersion = v; }

 protected:
  uint8_t fVersion = 0;  ///< version of the parameter
  uint16_t fConfig = 0;  ///< configuration setup of the module
  int fModuleId    = 0;  ///< module id
 private:
  ClassDef(CbmTrdParMod,
           2)  // Definition of generic parameters for one TRD module
};

#endif
