/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

/** @file CbmMCInput.h
 ** @author Volker Friese <v.friese@gsi.de>
 ** @date 20.06.2023
 **
 ** Definitions in namespace cbm::sim
 **/

#ifndef SIM_RESPONSE_BASE_DEFS_H
#define SIM_RESPONSE_BASE_DEFS_H 1


namespace cbm::sim
{

  /** @enum Simulation mode **/
  enum class Mode
  {
    Timebased,
    EventByEvent,
    Undefined
  };

  /** @enum Time distribution model **/
  enum class TimeDist
  {
    Poisson,
    Uniform,
    Undefined
  };


}  // namespace cbm::sim


#endif /* SIM_RESPONSE_BASE_DEFS_H */
