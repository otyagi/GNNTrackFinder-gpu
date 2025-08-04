/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#ifndef CBM_SERVICES_HISTSERV_APP_APPLICATION_H
#define CBM_SERVICES_HISTSERV_APP_APPLICATION_H 1

#include "Histogram.h"
#include "ProgramOptions.h"
#include "THttpServer.h"
#include "TObjArray.h"
#include "ui_callbacks.h"

#include <csignal>
#include <forward_list>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <zmq.hpp>

class TCanvas;
class TNamed;

namespace cbm::services::histserv
{
  /// \struct AppConfig
  /// \brief  Application configuarion
  struct AppConfig {
    int fNofTsToStore = 400;  ///< Number of consequent timeslices to store
  };

  class Application {

   public:
    /** @brief Standard constructor, initialises the application
     ** @param opt  **/
    explicit Application(ProgramOptions const& opt, volatile sig_atomic_t* signalStatus);

    /** @brief Copy constructor forbidden **/
    Application(const Application&) = delete;

    /** @brief Assignment operator forbidden **/
    void operator=(const Application&) = delete;

    /** @brief Destructor **/
    ~Application();

    /** @brief Run the application **/
    void Exec();

    void UpdateHttpServer();

   private:
    /// \brief  Collects histograms of the same type from the histogram list
    /// \tparam HistoSrc  Histogram type
    /// \param  container  List of histograms
    //template<class HistoSrc>
    //bool CollectHistograms(const std::forward_list<HistoSrc>& container);

    //const std::string& ConfigFile() const;
    /// \param name  A name of the histogram
    int FindHistogram(const std::string& name);

    /// \brief Resets handled histograms
    bool ResetHistograms();

    /// \brief  Read a histogram
    /// \tparam HistoDst  Destination histogram type
    /// \tparam HistoSrc  Source histogram type
    /// \param  rHist  Reference to the source histogram
    template<class HistoDst, class HistoSrc>
    bool ReadHistogram(const HistoSrc& rHist);

    /// \brief  Reads a histogram slice for an extended histogram with the TS ID
    /// \tparam HistoSrc  Source histogram type
    /// \param  rHistSrc  Reference to the source histogram
    /// \param  tsIndex   Index of timeslice
    template<class HistoSrc>
    bool ReadHistogramExtendedTsId(const HistoSrc& pHistSrc, uint64_t tsIndex);

    /// \brief Find histogram index in the histogram array
    /// \brief Receives histograms
    bool ReceiveData(zmq::message_t& msg);

    /// \brief Receives histogram configuration
    bool ReceiveHistoConfig(zmq::message_t& msg);

    /// \brief Receives canvas configuration
    bool ReceiveCanvasConfig(zmq::message_t& msg);

    /// \brief Receives a list of canvases and histograms
    /// \param vMsg  Message with the histograms and canvases list
    bool ReceiveConfigAndData(std::vector<zmq::message_t>& vMsg);

    /// \brief Register a histogram  config in the histogram server
    /// \param config  A pair (histogram name, histogram directory)
    ///
    /// This function should be called after the metadata is extracted from the config.
    bool RegisterHistoConfig(const std::pair<std::string, std::string>& config);

    /// \brief Register a histogram in the histogram server
    /// \param hist A pointer to histogram
    bool RegisterHistogram(const TNamed* hist);

    /// \brief Prepares canvases using received canvas configuration
    /// \param uCanvIdx Index of canvas
    bool PrepareCanvas(uint32_t uCanvIdx);

    /// \brief Saves handled histograms
    bool SaveHistograms();

    /// \brief A handler for system signals
    /// \param signal  Signal ID
    //static void SignalHandler(int signal);

   private:
    ProgramOptions const& fOpt;      ///< Program options object
    volatile sig_atomic_t* fSignalStatus;  ///< Global signal status
    THttpServer* fServer = nullptr;  ///< ROOT Histogram server (JSroot)
    std::thread fThread;
    bool fStopThread = false;

    std::unique_ptr<UiCmdActor> fUiCmdActor;

    /// Interface
    zmq::context_t fZmqContext {1};
    zmq::socket_t fZmqSocket {fZmqContext, ZMQ_PULL};

    /// Array of histograms with unique names
    TObjArray fArrayHisto;
    /// Vector of string with ( HistoName, FolderPath ) to configure the histogram
    std::vector<std::pair<std::string, std::string>> fvpsHistosFolder = {};
    /// Vector of string pairs with ( CanvasName, CanvasConfig ) to configure the canvases and histos within
    /// Format of Can config is "Name;Title;NbPadX(U);NbPadY(U);ConfigPad2(s);....;ConfigPadXY(s)"
    /// Format of Pad config is "GrixX(b),GridY(b),LogX(b),LogY(b),LogZ(b),HistoName(s),DrawOptions(s)"
    std::vector<std::pair<std::string, std::string>> fvpsCanvasConfig = {};
    std::vector<bool> fvbCanvasReady                                  = {};
    bool fbAllCanvasReady                                             = false;
    AppConfig fConfig;

    std::vector<std::pair<TNamed*, std::string>> fvHistos  = {};  ///< Vector of Histos pointers and folder path
    std::vector<bool> fvbHistoRegistered                   = {};
    bool fbAllHistosRegistered                             = false;
    std::vector<std::pair<TCanvas*, std::string>> fvCanvas = {};  ///< Vector of Canvas pointers and folder path
    std::vector<bool> fvbCanvasRegistered                  = {};
    bool fbAllCanvasRegistered                             = false;

    /// Internal status
    uint32_t fNMessages = 0;
  };

}  // namespace cbm::services::histserv

#endif /* CBM_SERVICES_HISTSERV_APP_APPLICATION_H */
