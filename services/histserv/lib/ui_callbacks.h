/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#ifndef CBM_SERVICES_HISTSERV_LIB_UI_CALLBACKS_H
#define CBM_SERVICES_HISTSERV_LIB_UI_CALLBACKS_H 1

#include "TNamed.h"

namespace cbm::services::histserv
{

  class UiCmdActor : public TNamed {
  public:
    UiCmdActor() : TNamed("UiCmdActor", "UiCmdActor") {};

    void SetResetHistos(bool flag = true) { bHistoServerResetHistos = flag; }
    void SetSaveHistos(bool flag = true) { bHistoServerSaveHistos = flag; }
    void SetServerStop(bool flag = true) { bHistoServerStop = flag; }

    bool GetResetHistos() { return bHistoServerResetHistos; }
    bool GetSaveHistos() { return bHistoServerSaveHistos; }
    bool GetServerStop() { return bHistoServerStop; }

  private:
    bool bHistoServerResetHistos = false;
    bool bHistoServerSaveHistos  = false;
    bool bHistoServerStop        = false;

    ClassDef(UiCmdActor, 1);
  };
}  // namespace cbm::services::histserv
#endif /* CBM_SERVICES_HISTSERV_LIB_UI_CALLBACKS_H */
