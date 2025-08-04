/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   check_ca_par.C
/// \brief  Prints the contents of the CA tracking parameter file
/// \since  07.05.2024
/// \author Sergei Zharko <s.zharko@gsi.de>

/// \brief Macro execution function
/// \param name  Path to the parameter file
void check_ca_par(const std::string& name)
{
  cbm::algo::ca::InitManager im;
  im.ReadParametersObject(name);
  std::cout << im.PrintParameters() << '\n';
}
