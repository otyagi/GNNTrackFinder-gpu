/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer], P.-A. Loizeau */

#ifndef ALGO_QA_DIGIEVENTQA_H
#define ALGO_QA_DIGIEVENTQA_H 1

#include "CbmDefs.h"
#include "DigiData.h"
#include "HistogramContainer.h"
#include "evbuild/EventBuilderConfig.h"

#include <gsl/span>
#include <unordered_map>
#include <vector>


namespace cbm::algo::evbuild
{

  /** @struct DigiEventQaData
   ** @brief QA results for CbmDigiEvent objects
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 16 June 2023
   **/
  struct DigiEventQaData {
    qa::HistogramContainer fHistContainer;
    std::unordered_map<ECbmModuleId, qa::H1D*> fDigiTimeHistos = {};
    size_t fNumEvents                                          = 0;
  };


  /** @struct DigiEventQaDetConfig
   ** @brief Configuration data for the QA of CbmDigiEvents for a given detector
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 16 June 2023
   **/
  struct DigiEventQaDetConfig {
    uint32_t fNumBins;
    double fMinValue;
    double fMaxValue;
    std::string ToString() const
    {
      std::stringstream ss;
      ss << "nbins " << fNumBins << " min " << fMinValue << " max " << fMaxValue << "\n";
      return ss.str();
    }
  };


  /** @struct DigiEventQaConfig
   ** @brief Configuration data for the QA of CbmDigiEvents
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 16 June 2023
   **/
  struct DigiEventQaConfig {
    std::map<ECbmModuleId, DigiEventQaDetConfig> fData;
    std::string ToString() const
    {
      std::stringstream ss;
      for (const auto& entry : fData)
        ss << "\n   Subsystem " << ::ToString(entry.first) << "  " << entry.second.ToString();
      return ss.str();
    }
    DigiEventQaConfig() = default;
    DigiEventQaConfig(const EventBuilderConfig& evbuildConfig, double borderSize, uint32_t numBins)
    {
      for (const auto& entry : evbuildConfig.fWindows) {
        auto detector   = entry.first;
        double tmin     = entry.second.first - borderSize;
        double tmax     = entry.second.second + borderSize;
        fData[detector] = {numBins, tmin, tmax};
      }
    }
    static std::string GetDigiTimeHistoName(const ECbmModuleId& subsystem)
    {
      return "digi_time_" + ::ToString(subsystem);
    }
    std::vector<std::pair<std::string, std::string>> GetHistosConfigs() const
    {
      std::vector<std::pair<std::string, std::string>> cfg{};
      for (const auto& entry : fData) {
        cfg.push_back(std::pair<std::string, std::string>(GetDigiTimeHistoName(entry.first), "DigiEvtQa"));
      }
      return cfg;
    }
    std::vector<std::pair<std::string, std::string>> GetCanvasConfigs() const
    {
      /// => Format is "CanvasName;Canvas Title;NbPadX(U);NbPadY(U);ConfigPad1(s);....;ConfigPadXY(s)"
      /// => Format of Pad config is
      ///    "GrixX(b),GridY(b),LogX(b),LogY(b),LogZ(b),(HistoName1,DrawOptions1),...,(HistoNameZ,DrawOptionsZ)"
      /// => See core/base/utils/fles/CbmFlesCanvasTools for the full code, especially GenerateCanvasConfigString

      // --- Canvas of all Time in event per system
      std::pair<std::string, std::string> cfgDigiTimeAll{"digiEvtTimeQaCanv", "digiEvtTimeQaCanv;"};
      cfgDigiTimeAll.second += "Digi time in Events per subsystem;";
      cfgDigiTimeAll.second += std::to_string(fData.size() / 2 + fData.size() % 2) + ";2;";
      for (const auto& entry : fData) {
        cfgDigiTimeAll.second += std::string("1,1,0,1,0,(") + GetDigiTimeHistoName(entry.first) + ",hist);";
      }
      if (fData.size() % 2) {  // Empty pad if odd number of systems
        cfgDigiTimeAll.second += std::string("1,1,0,1,0,;");
      }
      return std::vector<std::pair<std::string, std::string>>{cfgDigiTimeAll};
    }
  };


  /** @class DigiEventQa
   ** @brief QA for CbmDigiEvent objects
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 16 June 2023
   **/
  class DigiEventQa {
   public:
    /** @brief Constructor **/
    DigiEventQa(const DigiEventQaConfig& config) : fConfig(config){};

    /** @brief Destructor **/
    virtual ~DigiEventQa() = default;

    /** @brief Execution
     ** @param events Vector of DigiEvents to analyse
     ** @return QA data object
     **/
    DigiEventQaData operator()(const std::vector<DigiEvent>& events) const;

    /** @brief Info to string **/
    std::string ToString() const;

    /** @brief Const access to Qa config **/
    const DigiEventQaConfig& GetConfig() const { return fConfig; }


   private:  // methods
    /** @brief Fill histogram with digi time within event
     ** @param digis  Vector with digi objects
     ** @param eventTime  Time of event
     ** @param histo  Histogram to be filled
     **
     ** The templated class is required to implement the method double GetTime().
     **/
    template<class Digi>
    void FillDeltaT(gsl::span<const Digi> digis, double eventTime, qa::H1D* histo) const
    {
      for (const Digi& digi : digis)
        histo->Fill(digi.GetTime() - eventTime);
    }

    /** @brief Fill histogram with digi time within event
     ** @param digis  Vector with digi objects
     ** @param eventTime  Time of event
     ** @param histo  Histogram to be filled
     **/
    void QaDigiTimeInEvent(const DigiEvent& event, ECbmModuleId system, qa::H1D* histo) const;


   private:  // members
    DigiEventQaConfig fConfig;
  };


}  // namespace cbm::algo::evbuild

#endif /* ALGO_QA_DIGIEVENTQA_H */
