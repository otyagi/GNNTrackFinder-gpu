/* Copyright (C) 2022-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// \file   CaDefs.h
/// \brief  Compile-time constants definition for the CA tracking algorithm
/// \since  02.06.2022
/// \author S.Zharko <s.zharko@gsi.de>

#pragma once  // include this header only once per compilation unit

#include "CaSimd.h"
#include "KfFramework.h"
#include "KfTrackParam.h"

#include <limits>

namespace cbm::algo::ca
{
  using cbm::algo::kf::TrackParam;
  using cbm::algo::kf::TrackParamV;
}  // namespace cbm::algo::ca

/// Namespace contains compile-time constants definition for the CA tracking algorithm
///
namespace cbm::algo::ca::constants
{

  /// Array sizes
  namespace size
  {
    /// Order of polynomial to approximate field in the vicinity of station plane
    constexpr int MaxFieldApproxPolynomialOrder{5};

    /// Amount of coefficients in field approximations
    constexpr int MaxNFieldApproxCoefficients{(MaxFieldApproxPolynomialOrder + 1) * (MaxFieldApproxPolynomialOrder + 2)
                                              / 2};

    /// Amount of bits to code a station or triplet. This values determine the maximum number of stations and tripülets
    constexpr unsigned int StationBits = 6u;                 ///< Amount of bits to code one station
    constexpr unsigned int TripletBits = 32u - StationBits;  ///< Amount of bits to code one triplet

    constexpr int MaxNdetectors = 5;                  ///< Max number of tracking detectors
    constexpr int MaxNstations  = 1u << StationBits;  ///< Max number of stations, 2^6  = 64
    constexpr int MaxNtriplets  = 1u << TripletBits;  ///< Max number of triplets, 2^26 = 67,108,864

    constexpr uint8_t DetBits = 4u;  ///< Maximum 16 detector systems

    /// Max number of track groups
    /// NOTE: For a "track group" definition see CaParameters.h, GetSearchWindow function
    constexpr int MaxNtrackGroups = 4;

  }  // namespace size


  /// Control flags
  namespace control
  {
    /// \brief Flag: input data QA level
    ///  - 0: no checks will be done
    ///  - 1: only number of hits and strips as well as validity of hits first and last indexes will be checked
    ///  - 2: hits sorting is checked
    ///  - 3: every hit is checked for consistency
    /// \note The larger Level corresponds to more precise checks, but is followed by larger time penalty
    /// \warning other options than 0 do not work properly, more tests are needed!
    constexpr int InputDataQaLevel = 0;

  }  // namespace control

  /// Physics constants
  namespace phys
  {
    /// Particle masses etc used for the track fit, double precision
    constexpr double MuonMassD        = 0.105658375523;      ///< Muon mass     [GeV/c2]
    constexpr double PionMassD        = 0.1395703918;        ///< Pion mass     [GeV/c2]
    constexpr double KaonMassD        = 0.493677f;           ///< Kaon mass     [GeV/c2] (PDG 22.08.2023)
    constexpr double ElectronMassD    = 0.0005109989500015;  ///< Electron mass [GeV/c2]
    constexpr double ProtonMassD      = 0.93827208816;       ///< Proton mass   [GeV/c2] (PDG 11.08.2022)
    constexpr double SpeedOfLightD    = 29.9792458;          ///< Speed of light [cm/ns]
    constexpr double SpeedOfLightInvD = 1. / SpeedOfLightD;  ///< Inverse speed of light [ns/cm]

    /// Particle masses etc used for the track fit, fscal precision
    constexpr fscal MuonMass        = MuonMassD;         ///< Muon mass     [GeV/c2]
    constexpr fscal PionMass        = PionMassD;         ///< Pion mass     [GeV/c2]
    constexpr fscal KaonMass        = KaonMassD;         ///< Kaon mass     [GeV/c2] (PDG 22.08.2023)
    constexpr fscal ElectronMass    = ElectronMassD;     ///< Electron mass [GeV/c2]
    constexpr fscal ProtonMass      = ProtonMassD;       ///< Proton mass   [GeV/c2] (PDG 11.08.2022)
    constexpr fscal SpeedOfLight    = SpeedOfLightD;     ///< Speed of light [cm/ns]
    constexpr fscal SpeedOfLightInv = SpeedOfLightInvD;  ///< Inverse speed of light [ns/cm]

  }  // namespace phys

  /// Math constants
  namespace math
  {
    constexpr double Pi = 3.14159265358979323846;  ///< Value of PI, used in ROOT TMath
  }  // namespace math

  /// Miscellaneous constants
  namespace misc
  {
    constexpr int AssertionLevel      = 0;      ///< Assertion level
    constexpr int Alignment           = 16;     ///< Default alignment of data (bytes)
    constexpr fscal NegligibleFieldkG = 1.e-4;  ///< Negligible field [kG]

  }  // namespace misc

  /// GPU constants
  namespace gpu
  {
    constexpr int MaxDoubletsFromHit     = 150;    ///< Maximum number of doublets from a hit
    constexpr int MaxTripletsFromDoublet = 15;     ///< Maximum number of triplets from a doublet
    constexpr int MaxNofStations         = 20;     ///< Maximum number of stations //TODO: temporary solution
    constexpr bool GpuTracking           = false;  ///< Flag: use for CA GPU for tracking
    constexpr bool GpuTimeMonitoring     = true;   ///< Flag: use GPU for time monitoring
    constexpr bool GpuSortTriplets       = false;  ///< Flag: use GPU for sorting triplets
    constexpr bool CpuSortTriplets       = true;   ///< Flag: use CPU for sorting triplets
    constexpr bool GnnTracking           = true;   ///< Flag: use GNN for tracking
    constexpr bool GnnGpuTracking        = true;  ///< Flag: use GPU GNN for tracking
  }  // namespace gpu

  /// \brief Undefined values
  template<typename T1, typename T2 = T1>
  constexpr T2 Undef;

  template<>
  inline constexpr int Undef<int> = std::numeric_limits<int>::min();

  template<>
  inline constexpr unsigned Undef<unsigned> = std::numeric_limits<unsigned>::max();

  template<>
  inline constexpr float Undef<float> = std::numeric_limits<float>::signaling_NaN();

  template<>
  inline constexpr double Undef<double> = std::numeric_limits<double>::signaling_NaN();

  template<>
  inline constexpr fscal Undef<fvec> = std::numeric_limits<fscal>::signaling_NaN();


  /// Colors of terminal log messages
  namespace clrs
  {
    // NOTE: To be used first, because different users may have different console bg and fg colors
    constexpr char CL[]   = "\e[0m";    ///< clear
    constexpr char CLb[]  = "\e[1m";    ///< clear bold
    constexpr char CLi[]  = "\e[3m";    ///< clear italic
    constexpr char CLu[]  = "\e[4m";    ///< clear underline
    constexpr char CLr[]  = "\e[7m";    ///< clear reverse
    constexpr char CLbi[] = "\e[1;3m";  ///< clear bold-italic
    constexpr char CLbu[] = "\e[1;4m";  ///< clear bold-underline
    constexpr char CLbr[] = "\e[1;7m";  ///< clear bold-reverse

    // regular
    constexpr char BK[] = "\e[30m";  ///< normal black
    constexpr char RD[] = "\e[31m";  ///< normal red
    constexpr char GN[] = "\e[32m";  ///< normal green
    constexpr char YL[] = "\e[33m";  ///< normal yellow
    constexpr char BL[] = "\e[34m";  ///< normal blue
    constexpr char MG[] = "\e[35m";  ///< normal magenta
    constexpr char CY[] = "\e[36m";  ///< normal cyan
    constexpr char GY[] = "\e[37m";  ///< normal grey
    constexpr char WT[] = "\e[38m";  ///< normal white

    // bold
    constexpr char BKb[] = "\e[1;30m";  ///< bold black
    constexpr char RDb[] = "\e[1;31m";  ///< bold red
    constexpr char GNb[] = "\e[1;32m";  ///< bold green
    constexpr char YLb[] = "\e[1;33m";  ///< bold yellow
    constexpr char BLb[] = "\e[1;34m";  ///< bold blue
    constexpr char MGb[] = "\e[1;35m";  ///< bold magenta
    constexpr char CYb[] = "\e[1;36m";  ///< bold cyan
    constexpr char GYb[] = "\e[1;37m";  ///< bold grey
    constexpr char WTb[] = "\e[1;38m";  ///< bold white

    // italic
    constexpr char BKi[] = "\e[3;30m";  ///< italic black
    constexpr char RDi[] = "\e[3;31m";  ///< italic red
    constexpr char GNi[] = "\e[3;32m";  ///< italic green
    constexpr char YLi[] = "\e[3;33m";  ///< italic yellow
    constexpr char BLi[] = "\e[3;34m";  ///< italic blue
    constexpr char MGi[] = "\e[3;35m";  ///< italic magenta
    constexpr char CYi[] = "\e[3;36m";  ///< italic cyan
    constexpr char GYi[] = "\e[3;37m";  ///< italic grey
    constexpr char WTi[] = "\e[3;38m";  ///< italic white

    // underline
    constexpr char BKu[] = "\e[4;30m";  ///< underline black
    constexpr char RDu[] = "\e[4;31m";  ///< underline red
    constexpr char GNu[] = "\e[4;32m";  ///< underline green
    constexpr char YLu[] = "\e[4;33m";  ///< underline yellow
    constexpr char BLu[] = "\e[4;34m";  ///< underline blue
    constexpr char MGu[] = "\e[4;35m";  ///< underline magenta
    constexpr char CYu[] = "\e[4;36m";  ///< underline cyan
    constexpr char GYu[] = "\e[4;37m";  ///< underline grey
    constexpr char WTu[] = "\e[4;38m";  ///< underline white

    // reverse
    constexpr char BKr[] = "\e[7;30m";  ///< reverse black
    constexpr char RDr[] = "\e[7;31m";  ///< reverse red
    constexpr char GNr[] = "\e[7;32m";  ///< reverse green
    constexpr char YLr[] = "\e[7;33m";  ///< reverse yellow
    constexpr char BLr[] = "\e[7;34m";  ///< reverse blue
    constexpr char MGr[] = "\e[7;35m";  ///< reverse magenta
    constexpr char CYr[] = "\e[7;36m";  ///< reverse cyan
    constexpr char GYr[] = "\e[7;37m";  ///< reverse grey
    constexpr char WTr[] = "\e[7;38m";  ///< reverse white

    // bold-underline
    constexpr char BKbu[] = "\e[1;4;30m";  ///< bold-underline black
    constexpr char RDbu[] = "\e[1;4;31m";  ///< bold-underline red
    constexpr char GNbu[] = "\e[1;4;32m";  ///< bold-underline green
    constexpr char YLbu[] = "\e[1;4;33m";  ///< bold-underline yellow
    constexpr char BLbu[] = "\e[1;4;34m";  ///< bold-underline blue
    constexpr char MGbu[] = "\e[1;4;35m";  ///< bold-underline magenta
    constexpr char CYbu[] = "\e[1;4;36m";  ///< bold-underline cyan
    constexpr char GYbu[] = "\e[1;4;37m";  ///< bold-underline grey
    constexpr char WTbu[] = "\e[1;4;38m";  ///< bold-underline white

    // bold-italic
    constexpr char BKbi[] = "\e[1;3;30m";  ///< bold-italic black
    constexpr char RDbi[] = "\e[1;3;31m";  ///< bold-italic red
    constexpr char GNbi[] = "\e[1;3;32m";  ///< bold-italic green
    constexpr char YLbi[] = "\e[1;3;33m";  ///< bold-italic yellow
    constexpr char BLbi[] = "\e[1;3;34m";  ///< bold-italic blue
    constexpr char MGbi[] = "\e[1;3;35m";  ///< bold-italic magenta
    constexpr char CYbi[] = "\e[1;3;36m";  ///< bold-italic cyan
    constexpr char GYbi[] = "\e[1;3;37m";  ///< bold-italic grey
    constexpr char WTbi[] = "\e[1;3;38m";  ///< bold-italic white

    // bold-reverse
    constexpr char BKbr[] = "\e[1;7;30m";  ///< bold-reverse black
    constexpr char RDbr[] = "\e[1;7;31m";  ///< bold-reverse red
    constexpr char GNbr[] = "\e[1;7;32m";  ///< bold-reverse green
    constexpr char YLbr[] = "\e[1;7;33m";  ///< bold-reverse yellow
    constexpr char BLbr[] = "\e[1;7;34m";  ///< bold-reverse blue
    constexpr char MGbr[] = "\e[1;7;35m";  ///< bold-reverse magenta
    constexpr char CYbr[] = "\e[1;7;36m";  ///< bold-reverse cyan
    constexpr char GYbr[] = "\e[1;7;37m";  ///< bold-reverse grey
    constexpr char WTbr[] = "\e[1;7;38m";  ///< bold-reverse white
  }  // namespace clrs
}  // namespace cbm::algo::ca::constants
