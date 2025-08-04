/* Copyright (C) 2023-2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer], Sergei Zharko */

#include "Application.h"

#include "CbmFlesCanvasTools.h"
#include "CbmQaOnlineInterface.h"
#include "HistogramContainer.h"
#include "TCanvas.h"
#include "TEnv.h"
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "THttpServer.h"
#include "TMessage.h"
#include "TObjArray.h"
#include "TProfile.h"
#include "TProfile2D.h"
#include "TRootSniffer.h"
#include "TSystem.h"

#include <Logger.h>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/iostreams/device/array.hpp>
#ifdef BOOST_IOS_HAS_ZSTD
#include <boost/iostreams/filter/zstd.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#endif
#include <boost/iostreams/stream.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>

#include <mutex>
#include <zmq_addon.hpp>

#include <fmt/format.h>

std::mutex mtx;

namespace b_io = boost::iostreams;
namespace b_ar = boost::archive;

using cbm::services::histserv::Application;

using cbm::algo::qa::H1D;
using cbm::algo::qa::H2D;
using cbm::algo::qa::HistogramMetadata;
using cbm::algo::qa::Prof1D;
using cbm::algo::qa::Prof2D;

// ---------------------------------------------------------------------------------------------------------------------
//
Application::Application(ProgramOptions const& opt, volatile sig_atomic_t* signalStatus)
  : fOpt(opt)
  , fSignalStatus(signalStatus)
{
  /// Read options from executable
  LOG(info) << "Options for Application:";
  LOG(info) << " Input ZMQ channel: " << fOpt.ComChan();
  LOG(info) << " HTTP server port:  " << fOpt.HttpPort();
  if ("" != fOpt.HistoFile()) {  //
    LOG(info) << " Output filename:   " << fOpt.HistoFile() << (fOpt.Overwrite() ? " (in overwrite mode)" : "");
  }

  /// FIXME: SOMETHING_To_Replace_FairMQ!!!!!!!!!!!!!
  /// FIXME: Initialize communication channels of SOMETHING_To_Replace_FairMQ
  /// FIXME: Link channel to method in order to process received messages
  fZmqSocket.set(zmq::sockopt::rcvhwm, fOpt.ComChanZmqRcvHwm());   // High-Water Mark, nb updates kept in buffer
  fZmqSocket.set(zmq::sockopt::rcvtimeo, fOpt.ComChanZmqRcvTo());  // Timeout in ms to avoid stuck in loop!
  fZmqSocket.bind(fOpt.ComChan().c_str());  // This side "binds" the socket => Other side should connect!!!!

  fServer = new THttpServer(Form("http:%u", fOpt.HttpPort()));
  /// To avoid the server sucking all Histos from gROOT when no output file is used
  fServer->GetSniffer()->SetScanGlobalDir(kFALSE);
  const char* jsrootsys = gSystem->Getenv("JSROOTSYS");
  if (!jsrootsys) jsrootsys = gEnv->GetValue("HttpServ.JSRootPath", jsrootsys);

  fUiCmdActor = std::make_unique<UiCmdActor>();
  fServer->Register("/", fUiCmdActor.get());
  fServer->Hide("/UiCmdActor");

  if (!fOpt.HideGuiCommands()) {
    fServer->RegisterCommand("/Reset_Hist", "/UiCmdActor/->SetResetHistos()");
    fServer->RegisterCommand("/Save_Hist", "/UiCmdActor/->SetSaveHistos()");
    fServer->RegisterCommand("/Stop_Server", "/UiCmdActor/->SetServerStop()");

    /*
    fServer->RegisterCommand("/Reset_Hist", "this->ResetHistograms()");
    fServer->RegisterCommand("/Save_Hist", "this->SaveHistograms()");
    */

    fServer->Restrict("/Reset_Hist", "allow=admin");
    fServer->Restrict("/Save_Hist", "allow=admin");
    fServer->Restrict("/Stop_Server", "allow=admin");
  }

  LOG(info) << "JSROOT location: " << jsrootsys;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void Application::Exec()
{
  fStopThread = false;
  fThread     = std::thread(&Application::UpdateHttpServer, this);
  LOG(info) << "Listening to ZMQ messages ...";
  while (!(fUiCmdActor->GetServerStop()) && *fSignalStatus == 0) {
    try {
      /// Infinite loop, this is a service which should survive until told otherwise after all
      /// FIXME: Start listening to <SOMETHING?!?> to receive histograms and configuration
      /// FIXME: handle signals from OS/console
      /* Jan suggestion with zmq_addon CPP interface */
      std::vector<zmq::message_t> vMsg;
      const auto ret = zmq::recv_multipart(fZmqSocket, std::back_inserter(vMsg));
      if (!ret) continue;

      std::lock_guard<std::mutex> lk(mtx);
      if (*ret > 3) {
        ReceiveConfigAndData(vMsg);
      }
      else if (*ret == 1) {
        ReceiveData(vMsg[0]);
      }
      else {
        LOG(error) << "Invalid number of message parts received: should be either 1 or more than 3 vs " << *ret;
      }
    }
    catch (const zmq::error_t& err) {
      if (err.num() != EINTR) {
        throw;
      }
    }
  }
  // FIXME: SZh: Are the socket and the context finished properly?
}

// ---------------------------------------------------------------------------------------------------------------------
//
Application::~Application()
{
  SaveHistograms();
  fStopThread = true;
  fThread.join();
  SaveHistograms();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void Application::UpdateHttpServer()
{
  /// This is needed to have a reactive GUI independently of histogram updates reception
  while (!fStopThread) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::lock_guard<std::mutex> lk(mtx);

    fServer->ProcessRequests();

    /// TODO: control flags communication from histo server to histograms sources?
    /// Idea: 1 req channel (per process or not, mixup?), polling every N TS and/or M s
    if (fUiCmdActor->GetResetHistos()) {
      LOG(info) << "Reset Monitor histos ";
      ResetHistograms();
      fUiCmdActor->SetResetHistos(false);
    }  // if( fUiCmdActor->GetResetHistos() )

    if (fUiCmdActor->GetSaveHistos()) {
      LOG(info) << "Save All histos & canvases";
      SaveHistograms();
      fUiCmdActor->SetSaveHistos(false);
    }  // if( fUiCmdActor->GetSaveHistos() )
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
//template<class HistoSrc>
//bool Appliaction::CollectHistograms(const std::forward_list<HistoSrc>& container)
//{
//  for (const auto& hist : container) {
//    if (!hist.GetFlag(cbm::algo::qa::EHistFlag::OmitIntegrated)) {
//      if (!ReadHistogram<TH1>(hist)) {
//        return false;
//      }
//    }
//    if (hist.GetFlag(cbm::algo::qa::EHistFlag::StoreVsTsId)) {
//      if constexpr (std::is_same_v<HistSrc, H1D> || std::is_same_v<HistSrc, Prof1D>) {
//        if (!ReadHistogramExtendedTsId(hist, timesliceId)) {
//          return false;
//        }
//      }
//      else {
//        LOG(warn) << "Histogram " << rHist.GetName() << " cannot be plotted vs. TS index. Ignoring";
//      }
//    }
//  }
//  return true;
//}

// ---------------------------------------------------------------------------------------------------------------------
//
bool Application::ReceiveData(zmq::message_t& msg)
{
  LOG(debug) << "Application::ReceiveData => Processing histograms update";

  /// FIXME: Something to replace FairMQ and extract the histograms!!!!
  /// FIXME: Need something to replace the ROOT serializer which allowed to have any of TH1x, TH2x, TH3x or TProfile
  /// FIXME: Need something to replace the TObjArray which allowed to have a mix of of TH1x, TH2x, TH3x or TProfile
  b_io::basic_array_source<char> device(static_cast<char*>(msg.data()), msg.size());
  b_io::stream<b_io::basic_array_source<char>> s(device);

  cbm::algo::qa::HistogramContainer vHist;
  if (fOpt.CompressedInput()) {
#ifdef BOOST_IOS_HAS_ZSTD
    std::unique_ptr<b_io::filtering_istream> in_ = std::make_unique<b_io::filtering_istream>();
    in_->push(b_io::zstd_decompressor());
    in_->push(s);
    std::unique_ptr<b_ar::binary_iarchive> iarchive_ = std::make_unique<b_ar::binary_iarchive>(*in_, b_ar::no_header);
    *iarchive_ >> vHist;
#else
    throw std::runtime_error("Unsupported ZSTD decompression (boost) for histograms input channel");
#endif
  }
  else {
    b_ar::binary_iarchive iarch(s);
    iarch >> vHist;
  }


  /// copied from CbmTaskDigiEventQa::ToTH1D
  /// FIXME: Should be placed in a tools/interface/whatever library with all similar functions!!
  /// FIXME: Reverse OP need to be implemented + CI unit tests for back and forth in each direction (ROOT <-> Algo)
  /// FIXME: Lead to "Warning in <TROOT::Append>: Replacing existing TH1: xxxxxx (Potential memory leak)."
  auto timesliceId = vHist.fTimesliceId;

  // Collects 1D-histograms (with possible extention vs. TS)
  auto CollectHistogram1D = [&](const auto& container) -> bool {
    for (const auto& hist : container) {
      if (!hist.GetFlag(cbm::algo::qa::EHistFlag::OmitIntegrated)) {
        if (!ReadHistogram<TH1>(hist)) {
          return false;
        }
      }
      if (hist.GetFlag(cbm::algo::qa::EHistFlag::StoreVsTsId)) {
        if (!ReadHistogramExtendedTsId(hist, timesliceId)) {
          return false;
        }
      }
    }
    return true;
  };

  // Collects 2D-histograms
  // TODO: Add a possibility to store multiple histograms vs each time-slice depending on the StoreVsTsId flag
  // NOTE: TProfile2D can be extended to TProfile3D, and TH2D can be extended to TH3D. Should we provide such a possibility,
  //       or it does not make sence? (SZh)
  auto CollectHistogram2D = [&](const auto& container) -> bool {
    for (const auto& hist : container) {
      if (!hist.GetFlag(cbm::algo::qa::EHistFlag::OmitIntegrated)) {
        if (!ReadHistogram<TH1>(hist)) {
          return false;
        }
      }
    }
    return true;
  };

  if (!CollectHistogram1D(vHist.fvH1)) return false;
  if (!CollectHistogram2D(vHist.fvH2)) return false;
  if (!CollectHistogram1D(vHist.fvP1)) return false;
  if (!CollectHistogram2D(vHist.fvP2)) return false;


  /// If new histos received, try to prepare as many canvases as possible
  /// Should be expensive on start and cheap afterward
  if (!fbAllCanvasReady) {
    LOG(debug) << "Application::ReceiveData => Checking for canvases updates";
    for (uint32_t uCanv = 0; uCanv < fvpsCanvasConfig.size(); ++uCanv) {
      /// Jump canvases already ready
      if (fvbCanvasReady[uCanv]) {  //
        continue;
      }

      /// Now come the expensive part as we unpack its config and check each histo
      fvbCanvasReady[uCanv] = PrepareCanvas(uCanv);
    }  // for( uint32_t uCanv = 0; uCanv < fvpsCanvasConfig.size(); ++uCanv )
  }    // if( !fbAllCanvasReady )

  /*
  TObject* tempObject = nullptr;
  if (TString(tempObject->ClassName()).EqualTo("TObjArray")) {
    std::lock_guard<std::mutex> lk(mtx);
    TObjArray* arrayHisto = static_cast<TObjArray*>(tempObject);
    for (Int_t i = 0; i < arrayHisto->GetEntriesFast(); i++) {
      TObject* pObj = arrayHisto->At(i);

      if (nullptr != dynamic_cast<TProfile*>(pObj)) {
        if (!ReadHistogram<TProfile>(dynamic_cast<TProfile*>(pObj))) {  //
          return false;
        }
      }  // if( nullptr != dynamic_cast< TProfile *>( pObj ) )
      else if (nullptr != dynamic_cast<TH2*>(pObj)) {
        if (!ReadHistogram<TH2>(dynamic_cast<TH2*>(pObj))) {  //
          return false;
        }
      }  // if( nullptr != dynamic_cast< TH2 *>( pObj ) )
      else if (nullptr != dynamic_cast<TH1*>(pObj)) {
        if (!ReadHistogram<TH1>(dynamic_cast<TH1*>(pObj))) {  //
          return false;
        }
      }  // if( nullptr != dynamic_cast< TH1 *>( pObj ) )
      else
        LOG(warning) << "Unsupported object type for " << pObj->GetName();
    }  // for (Int_t i = 0; i < arrayHisto->GetEntriesFast(); i++)

    LOG(debug) << "Application::ReceiveData => Deleting array";
    /// Need to use Delete instead of Clear to avoid memory leak!!!
    arrayHisto->Delete();

    /// If new histos received, try to prepare as many canvases as possible
    /// Should be expensive on start and cheap afterward
    if (!fbAllCanvasReady) {
      LOG(debug) << "Application::ReceiveData => Checking for canvases updates";
      for (uint32_t uCanv = 0; uCanv < fvpsCanvasConfig.size(); ++uCanv) {
        /// Jump canvases already ready
        if (fvbCanvasReady[uCanv]) {  //
          continue;
        }

        /// Now come the expensive part as we unpack its config and check each histo
        fvbCanvasReady[uCanv] = PrepareCanvas(uCanv);
      }  // for( uint32_t uCanv = 0; uCanv < fvpsCanvasConfig.size(); ++uCanv )
    }    // if( !fbAllCanvasReady )
  }      // if (TString(tempObject->ClassName()).EqualTo("TObjArray"))
  else {
    fStopThread         = true;
    std::string err_msg = "Application::ReceiveData => Wrong object type at input: ";
    err_msg += tempObject->ClassName();
    throw std::runtime_error(err_msg);
  }

  if (nullptr != tempObject) delete tempObject;
  */

  fNMessages += 1;

  LOG(debug) << "Application::ReceiveData => Finished processing histograms update";

  /// FIXME: make the log frequency configurable?
  if (0 == fNMessages % 100) {
    LOG(info) << "HistServ::Application::ReceiveData => Finished processing histograms update #" << fNMessages
              << ", still alive!";
  }

  return true;
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool Application::ReceiveHistoConfig(zmq::message_t& msg)
{
  using cbm::algo::qa::EHistFlag;
  using cbm::algo::qa::HistogramMetadata;
  /// FIXME: Something to replace FairMQ and extract the config!!!!
  //  BoostSerializer<std::pair<std::string, std::string>>().Deserialize(msg, tempObject);
  b_io::basic_array_source<char> device(static_cast<char*>(msg.data()), msg.size());
  b_io::stream<b_io::basic_array_source<char>> s(device);

  std::pair<std::string, std::string> tempObject("", "");
  if (fOpt.CompressedInput()) {
#ifdef BOOST_IOS_HAS_ZSTD
    std::unique_ptr<b_io::filtering_istream> in_ = std::make_unique<b_io::filtering_istream>();
    in_->push(b_io::zstd_decompressor());
    in_->push(s);
    std::unique_ptr<b_ar::binary_iarchive> iarchive_ = std::make_unique<b_ar::binary_iarchive>(*in_, b_ar::no_header);
    *iarchive_ >> tempObject;
#else
    throw std::runtime_error("Unsupported ZSTD decompression (boost) for histograms config input channel");
#endif
  }
  else {
    b_ar::binary_iarchive iarch(s);
    iarch >> tempObject;
  }

  // Parse metadata
  std::string& name = tempObject.first;
  std::string metadataMsg{};
  std::tie(name, metadataMsg) = HistogramMetadata::SeparateNameAndMetadata(name);
  auto metadata               = HistogramMetadata(metadataMsg);

  if (!metadata.GetFlag(EHistFlag::OmitIntegrated)) {
    // Main (integrated over time) histogram
    this->RegisterHistoConfig(tempObject);
  }
  if (metadata.GetFlag(EHistFlag::StoreVsTsId)) {
    // Histogram vs. TS id
    this->RegisterHistoConfig(std::make_pair(name + std::string(HistogramMetadata::ksTsIdSuffix), tempObject.second));
  }

  return true;
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool Application::ReceiveCanvasConfig(zmq::message_t& msg)
{
  /// FIXME: Something to replace FairMQ and extract the config!!!!
  //  BoostSerializer<std::pair<std::string, std::string>>().Deserialize(msg, tempObject);
  b_io::basic_array_source<char> device(static_cast<char*>(msg.data()), msg.size());
  b_io::stream<b_io::basic_array_source<char>> s(device);

  std::pair<std::string, std::string> tempObject("", "");
  if (fOpt.CompressedInput()) {
#ifdef BOOST_IOS_HAS_ZSTD
    std::unique_ptr<b_io::filtering_istream> in_ = std::make_unique<b_io::filtering_istream>();
    in_->push(b_io::zstd_decompressor());
    in_->push(s);
    std::unique_ptr<b_ar::binary_iarchive> iarchive_ = std::make_unique<b_ar::binary_iarchive>(*in_, b_ar::no_header);
    *iarchive_ >> tempObject;
#else
    throw std::runtime_error("Unsupported ZSTD decompression (boost) for canvas config input channel");
#endif
  }
  else {
    b_ar::binary_iarchive iarch(s);
    iarch >> tempObject;
  }

  LOG(debug) << " Received configuration for canvas " << tempObject.first << " : " << tempObject.second;

  /// Check if canvas name already received in previous messages
  /// Linear search should be ok as config is shared only at startup
  uint32_t uPrevCanv = 0;
  for (uPrevCanv = 0; uPrevCanv < fvpsCanvasConfig.size(); ++uPrevCanv) {
    if (fvpsCanvasConfig[uPrevCanv].first == tempObject.first) {  //
      break;
    }
  }  // for( UInt_t uPrevCanv = 0; uPrevCanv < fvpsCanvasConfig.size(); ++uPrevCanv )

  if (uPrevCanv < fvpsCanvasConfig.size()) {
    LOG(debug) << " Ignored new configuration for Canvas " << tempObject.first
               << " due to previously received one: " << tempObject.second;
    /// Not sure if we should return false here...
  }  // if( uPrevCanv < fvpsCanvasConfig.size() )
  else {
    fvpsCanvasConfig.push_back(tempObject);
    fvbCanvasReady.push_back(false);
    fbAllCanvasReady = false;

    fvCanvas.push_back(std::pair<TCanvas*, std::string>(nullptr, ""));
    fvbCanvasRegistered.push_back(false);
    fbAllCanvasRegistered = false;
    LOG(info) << " Stored configuration for canvas " << tempObject.first << " : " << tempObject.second;
  }  // else of if( uPrevCanv < fvpsCanvasConfig.size() )
  return true;
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool Application::ReceiveConfigAndData(std::vector<zmq::message_t>& vMsg)
{
  /// FIXME: Something to replace FairMQ and extract the histograms!!!!
  LOG(debug) << "Application::ReceiveConfigAndData => Received composed message with " << vMsg.size() << " parts";

  /// Header contains a pair of unsigned integers
  /// FIXME: Something to replace FairMQ and extract the header!!!!
  // BoostSerializer<std::pair<uint32_t, uint32_t>>().Deserialize(vMsg.at(0), pairHeader);
  b_io::basic_array_source<char> device_header(static_cast<char*>(vMsg.at(0).data()), vMsg.at(0).size());
  b_io::stream<b_io::basic_array_source<char>> s_header(device_header);

  std::pair<uint32_t, uint32_t> pairHeader;
  if (fOpt.CompressedInput()) {
#ifdef BOOST_IOS_HAS_ZSTD
    std::unique_ptr<b_io::filtering_istream> in_ = std::make_unique<b_io::filtering_istream>();
    in_->push(b_io::zstd_decompressor());
    in_->push(s_header);
    std::unique_ptr<b_ar::binary_iarchive> iarchive_ = std::make_unique<b_ar::binary_iarchive>(*in_, b_ar::no_header);
    *iarchive_ >> pairHeader;
#else
    throw std::runtime_error("Unsupported ZSTD decompression (boost) for Config + Histos input channel");
#endif
  }
  else {
    b_ar::binary_iarchive iarch_header(s_header);
    iarch_header >> pairHeader;
  }

  LOG(debug) << "Application::ReceiveConfigAndData => Received configuration for " << pairHeader.first << " histos and "
             << pairHeader.second << " canvases";

  uint32_t uOffsetHistoConfig = pairHeader.first;
  if (0 == pairHeader.first) {
    uOffsetHistoConfig = 1;
    if (0 < vMsg[uOffsetHistoConfig].size()) {
      fStopThread = true;
      fUiCmdActor->SetServerStop();
      std::string err_msg = "Application::ReceiveConfigAndData => No histo config expected but corresponding message";
      err_msg += " is not empty: ";
      err_msg += vMsg[uOffsetHistoConfig].size();
      throw std::runtime_error(err_msg);
    }
  }

  uint32_t uOffsetCanvasConfig = pairHeader.second;
  if (0 == pairHeader.second) {
    uOffsetCanvasConfig = 1;
    if (0 < vMsg[uOffsetHistoConfig + uOffsetCanvasConfig].size()) {
      fStopThread = true;
      fUiCmdActor->SetServerStop();
      std::string err_msg = "Application::ReceiveConfigAndData => No Canvas config expected but corresponding ";
      err_msg += " message is not empty: ";
      err_msg += vMsg[uOffsetHistoConfig + uOffsetCanvasConfig].size();
      throw std::runtime_error(err_msg);
    }
  }

  if ((1 + uOffsetHistoConfig + uOffsetCanvasConfig + 1) != vMsg.size()) {
    fStopThread = true;
    fUiCmdActor->SetServerStop();
    std::string err_msg = "Application::ReceiveConfigAndData => Nb parts in message not matching configs numbers ";
    err_msg += " declared in header";
    err_msg += vMsg.size();
    err_msg += " VS ";
    err_msg += 1 + uOffsetHistoConfig + uOffsetCanvasConfig + 1;
    throw std::runtime_error(err_msg);
  }

  /// Decode parts for histograms configuration (auto-skip empty message if 0 declared in header)
  for (uint32_t uHisto = 0; uHisto < pairHeader.first; ++uHisto) {
    ReceiveHistoConfig(vMsg[1 + uHisto]);
  }  // for (UInt_t uHisto = 0; uHisto < pairHeader.first; ++uHisto)
  LOG(debug) << "Application::ReceiveConfigAndData => Processed configuration for " << pairHeader.first << " histos";

  /// Decode parts for histograms configuration (auto-skip empty message if 0 declared in header)
  for (uint32_t uCanv = 0; uCanv < pairHeader.second; ++uCanv) {
    ReceiveCanvasConfig(vMsg[1 + uOffsetHistoConfig + uCanv]);
  }  // for (UInt_t uCanv = 0; uCanv < pairHeader.second; ++uCanv)
  LOG(debug) << "Application::ReceiveConfigAndData => Processed configuration for " << pairHeader.second << " canvases";

  /// Decode the histograms data now that the configuration is loaded
  ReceiveData(vMsg[1 + uOffsetHistoConfig + uOffsetCanvasConfig]);

  return true;
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<class HistoDst, class HistoSrc>
bool Application::ReadHistogram(const HistoSrc& rHist)
{
  HistoDst* pHist = cbm::qa::OnlineInterface::ROOTHistogram(rHist);
  int index1 = FindHistogram(pHist->GetName());
  if (-1 == index1) {
    // ----- Creating new histogram
    HistoDst* histogram_new = static_cast<HistoDst*>(pHist->Clone());
    fArrayHisto.Add(histogram_new);

    LOG(info) << "Received new histo " << pHist->GetName();

    /// If new histo received, try to register it if configuration available
    if (!fbAllHistosRegistered) {
      for (uint32_t uHist = 0; uHist < fvpsHistosFolder.size(); ++uHist) {
        /// Jump histos already ready
        if (fvbHistoRegistered[uHist]) {  //
          continue;
        }

        /// Check if name matches one in config for others
        if (fvpsHistosFolder[uHist].first == histogram_new->GetName()) {
          fvHistos[uHist] = std::pair<TNamed*, std::string>(histogram_new, fvpsHistosFolder[uHist].second);
          fServer->Register(Form("/%s", fvHistos[uHist].second.data()), fvHistos[uHist].first);
          fvbHistoRegistered[uHist] = true;

          LOG(info) << "registered histo " << fvHistos[uHist].first->GetName() << " in folder "
                    << fvHistos[uHist].second;


          /// Update flag telling whether all known histos are registered
          fbAllHistosRegistered = true;
          for (uint32_t uIdx = 0; uIdx < fvbHistoRegistered.size(); ++uIdx) {
            if (!fvbHistoRegistered[uIdx]) {
              fbAllHistosRegistered = false;
              break;
            }  // if( !fvbHistoRegistered[ uIdx ] )
          }    // for( uint32_t uIdx = 0; uIdx < fvbHistoRegistered.size(); ++uIdx )

          break;
        }  // if( fvpsHistosFolder[ uHist ].first == histogram_new->GetName() )
      }    // for( uint32_t uCanv = 0; uCanv < fvpsCanvasConfig.size(); ++uCanv )
    }      // if( !fbAllCanvasReady )
  }        // if (-1 == index1)
  else {
    // ----- Update histogram
    LOG(debug) << "Received update for: " << pHist->GetName();
    HistoDst* histogram_existing = dynamic_cast<HistoDst*>(fArrayHisto.At(index1));
    if (nullptr == histogram_existing) {
      LOG(error) << "CbmMqHistoServer::ReadHistogram => "
                 << "Incompatible type found during update for histo " << pHist->GetName();
      delete pHist;
      return false;
    }  // if( nullptr == histogram_existing )

    histogram_existing->Add(pHist);
  }  // else of if (-1 == index1)
  delete pHist;
  return true;
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<class HistoSrc>
bool Application::ReadHistogramExtendedTsId(const HistoSrc& rHist, uint64_t tsIndex)
{
  constexpr bool IsSrcH1D    = std::is_same_v<HistoSrc, H1D>;
  constexpr bool IsSrcProf1D = std::is_same_v<HistoSrc, Prof1D>;

  if constexpr (IsSrcH1D || IsSrcProf1D) {
    /* clang-format off */
    using HistoDst_t = typename std::conditional<IsSrcH1D, TH2D, TProfile2D>::type;
    /* clang-format on */

    std::string sHistoName = rHist.GetName() + std::string(HistogramMetadata::ksTsIdSuffix);
    int index1             = FindHistogram(sHistoName.c_str());
    if (-1 == index1) {
      // ----- Creating new histogram
      HistoDst_t* histogram_new = nullptr;
      {
        std::string title = rHist.GetTitle();
        title.insert(title.find_first_of(';'), " vs. TS index;TS index");
        int nBinsX  = fConfig.fNofTsToStore;
        double minX = static_cast<double>(tsIndex) - 0.5;
        double maxX = static_cast<double>(tsIndex + nBinsX) - 0.5;
        int nBinsY  = rHist.GetNbinsX();
        double minY = rHist.GetMinX();
        double maxY = rHist.GetMaxX();
        if constexpr (IsSrcH1D) {
          histogram_new = new TH2D(sHistoName.c_str(), title.c_str(), nBinsX, minX, maxX, nBinsY, minY, maxY);
        }
        else if constexpr (IsSrcProf1D) {
          double minZ = rHist.GetMinY();
          double maxZ = rHist.GetMaxY();
          histogram_new =
            new TProfile2D(sHistoName.c_str(), title.c_str(), nBinsX, minX, maxX, nBinsY, minY, maxY, minZ, maxZ);
          histogram_new->Sumw2();
        }
      }
      cbm::qa::OnlineInterface::AddSlice(rHist, double(tsIndex), histogram_new);
      fArrayHisto.Add(histogram_new);

      LOG(info) << "Received new histo " << sHistoName;

      /// If new histo received, try to register it if configuration available
      if (!fbAllHistosRegistered) {
        for (uint32_t uHist = 0; uHist < fvpsHistosFolder.size(); ++uHist) {
          /// Jump histos already ready
          if (fvbHistoRegistered[uHist]) {  //
            continue;
          }

          /// Check if name matches one in config for others
          if (fvpsHistosFolder[uHist].first == histogram_new->GetName()) {
            fvHistos[uHist] = std::pair<TNamed*, std::string>(histogram_new, fvpsHistosFolder[uHist].second);
            fServer->Register(Form("/%s", fvHistos[uHist].second.data()), fvHistos[uHist].first);
            fvbHistoRegistered[uHist] = true;

            LOG(info) << "registered histo " << fvHistos[uHist].first->GetName() << " in folder "
                      << fvHistos[uHist].second;


            /// Update flag telling whether all known histos are registered
            fbAllHistosRegistered = true;
            for (uint32_t uIdx = 0; uIdx < fvbHistoRegistered.size(); ++uIdx) {
              if (!fvbHistoRegistered[uIdx]) {
                fbAllHistosRegistered = false;
                break;
              }  // if( !fvbHistoRegistered[ uIdx ] )
            }    // for( uint32_t uIdx = 0; uIdx < fvbHistoRegistered.size(); ++uIdx )

            break;
          }  // if( fvpsHistosFolder[ uHist ].first == histogram_new->GetName() )
        }    // for( uint32_t uCanv = 0; uCanv < fvpsCanvasConfig.size(); ++uCanv )
      }      // if( !fbAllCanvasReady )
    }        // if (-1 == index1)
    else {
      // ----- Update histogram
      LOG(debug) << "Received update for: " << sHistoName;
      HistoDst_t* histogram_existing = dynamic_cast<HistoDst_t*>(fArrayHisto.At(index1));
      if (nullptr == histogram_existing) {
        LOG(error) << "CbmMqHistoServer::ReadHistogram => "
                   << "Incompatible type found during update for histo " << sHistoName;
        return false;
      }  // if( nullptr == histogram_existing )
      cbm::qa::OnlineInterface::AddSlice(rHist, double(tsIndex), histogram_existing);
    }  // else of if (-1 == index1)
  }
  else {
    LOG(warn) << "Histogram " << rHist.GetName() << " cannot be plotted vs. TS index. Ignoring";
  }

  return true;
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool Application::RegisterHistoConfig(const std::pair<std::string, std::string>& config)
{
  LOG(debug) << " Received configuration for histo " << config.first << " : " << config.second;

  /// Check if histo name already received in previous messages
  /// Linear search should be ok as config is shared only at startup
  UInt_t uPrevHist = 0;
  for (uPrevHist = 0; uPrevHist < fvpsHistosFolder.size(); ++uPrevHist) {
    if (fvpsHistosFolder[uPrevHist].first == config.first) {  //
      break;
    }
  }  // for( UInt_t uPrevHist = 0; uPrevHist < fvpsHistosFolder.size(); ++uPrevHist )

  if (uPrevHist < fvpsHistosFolder.size()) {
    LOG(debug) << " Ignored new configuration for histo " << config.first
               << " due to previously received one: " << config.second;
    /// Not sure if we should return false here...
  }  // if( uPrevHist < fvpsHistosFolder.size() )
  else {
    fvpsHistosFolder.push_back(config);
    fvHistos.push_back(std::pair<TNamed*, std::string>(nullptr, ""));
    fvbHistoRegistered.push_back(false);
    fbAllHistosRegistered = false;
    LOG(info) << " Stored configuration for histo " << config.first << " : " << config.second;
  }  // else of if( uPrevHist < fvpsHistosFolder.size() )

  return true;
}

// ---------------------------------------------------------------------------------------------------------------------
//
int Application::FindHistogram(const std::string& name)
{
  for (int iHist = 0; iHist < fArrayHisto.GetEntriesFast(); ++iHist) {
    TObject* obj = fArrayHisto.At(iHist);
    if (TString(obj->GetName()).EqualTo(name)) {  //
      return iHist;
    }
  }  // for( int iHist = 0; iHist < fArrayHisto.GetEntriesFast(); ++iHist )
  return -1;
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool Application::PrepareCanvas(uint32_t uCanvIdx)
{
  CanvasConfig conf(ExtractCanvasConfigFromString(fvpsCanvasConfig[uCanvIdx].second));
  LOG(info) << " Extracting configuration for canvas index " << uCanvIdx << "(name: " << conf.GetName().data() << ")";

  /// First check if all objects to be drawn are present
  uint32_t uNbPads(conf.GetNbPads());
  for (uint32_t uPadIdx = 0; uPadIdx < uNbPads; ++uPadIdx) {
    uint32_t uNbObj(conf.GetNbObjsInPad(uPadIdx));
    for (uint32_t uObjIdx = 0; uObjIdx < uNbObj; ++uObjIdx) {
      std::string sName(conf.GetObjName(uPadIdx, uObjIdx));
      /// Check for empty pads!
      if ("nullptr" != sName) {
        if (FindHistogram(sName) < 0) {
          LOG(warn) << "Histogram \"" << sName << "\" requested by canvas \"" << conf.GetName().data()
                    << "\" was not found";
          return false;
        }  // if( FindHistogram( conf.GetObjName( uPadIdx, uObjIdx ) ) < 0 )
      }    // if( "nullptr" != sName )
    }      // for( uint32_t uObjIdx = 0; uObjIdx < uNbObj; ++uObjIdx )
  }        // for( uint32_t uPadIdx = 0; uPadIdx < uNbPads; ++uPadIdx )

  LOG(info) << " All histos found for canvas " << conf.GetName().data() << ", now preparing it";

  // Temporary solution to save canvases into directories
  std::string sNameFull = conf.GetName();
  size_t lastSlashPos   = sNameFull.find_last_of('/');
  std::string sNamePart = lastSlashPos > sNameFull.size() ? sNameFull : sNameFull.substr(lastSlashPos + 1);
  std::string sDir      = lastSlashPos > sNameFull.size() ? "" : sNameFull.substr(0, lastSlashPos);
  std::string canvDir   = sDir.empty() ? "canvases" : fmt::format("canvases/{}", sDir);

  /// Create new canvas and pads
  TCanvas* pNewCanv = new TCanvas(sNamePart.c_str(), conf.GetTitle().data());
  pNewCanv->Divide(conf.GetNbPadsX(), conf.GetNbPadsY());

  /// Loop on pads
  for (uint32_t uPadIdx = 0; uPadIdx < uNbPads; ++uPadIdx) {
    pNewCanv->cd(1 + uPadIdx);

    /// Pad settings
    gPad->SetGrid(conf.GetGridx(uPadIdx), conf.GetGridy(uPadIdx));
    gPad->SetLogx(conf.GetLogx(uPadIdx));
    gPad->SetLogy(conf.GetLogy(uPadIdx));
    gPad->SetLogz(conf.GetLogz(uPadIdx));

    /// Add objects (we know they are there
    uint32_t uNbObj(conf.GetNbObjsInPad(uPadIdx));
    for (uint32_t uObjIdx = 0; uObjIdx < uNbObj; ++uObjIdx) {
      std::string sName(conf.GetObjName(uPadIdx, uObjIdx));
      if ("nullptr" != sName) {
        TObject* pObj = fArrayHisto[FindHistogram(sName)];

        if (auto* pHistP2 = dynamic_cast<TProfile2D*>(pObj)) {
          pHistP2->Draw(conf.GetOption(uPadIdx, uObjIdx).data());
        }  // if( nullptr != dynamic_cast< TProfile *>( pObj ) )
        else if (auto* pHistP1 = dynamic_cast<TProfile*>(pObj)) {
          pHistP1->SetLineColor(uObjIdx + 1);
          pHistP1->SetMarkerColor(uObjIdx + 1);
          pHistP1->Draw(conf.GetOption(uPadIdx, uObjIdx).data());
        }  // if( nullptr != dynamic_cast< TProfile *>( pObj ) )
        else if (auto* pHistH2 = dynamic_cast<TH2*>(pObj)) {
          pHistH2->Draw(conf.GetOption(uPadIdx, uObjIdx).data());
        }  // if( nullptr != dynamic_cast< TH2 *>( pObj ) )
        else if (auto* pHistH1 = dynamic_cast<TH1*>(pObj)) {
          pHistH1->SetLineColor(uObjIdx + 1);
          pHistH1->SetMarkerColor(uObjIdx + 1);
          pHistH1->Draw(conf.GetOption(uPadIdx, uObjIdx).data());
        }  // if( nullptr != dynamic_cast< TH1 *>( pObj ) )
        else
          LOG(warning) << "  Unsupported object type for " << sName << " when preparing canvas " << conf.GetName();

        LOG(info) << "  Configured histo " << sName << " on pad " << (1 + uPadIdx) << " for canvas "
                  << conf.GetName().data();
      }  // if( "nullptr" != sName )
    }    // for( uint32_t uObjIdx = 0; uObjIdx < uNbObj; ++uObjIdx )
  }      // for( uint32_t uPadIdx = 0; uPadIdx < uNbPads; ++uPadIdx )

  fvCanvas[uCanvIdx] = std::pair<TCanvas*, std::string>(pNewCanv, canvDir);
  fServer->Register(Form("/%s", fvCanvas[uCanvIdx].second.data()), fvCanvas[uCanvIdx].first);
  fvbCanvasRegistered[uCanvIdx] = true;

  LOG(info) << " Registered canvas " << fvCanvas[uCanvIdx].first->GetName() << " in folder "
            << fvCanvas[uCanvIdx].second;

  /// Update flag telling whether all known canvases are registered
  fbAllCanvasRegistered = true;
  for (uint32_t uIdx = 0; uIdx < fvbCanvasRegistered.size(); ++uIdx) {
    if (!fvbCanvasRegistered[uIdx]) {
      fbAllCanvasRegistered = false;
      break;
    }  // if( !fvbCanvasRegistered[ uIdx ] )
  }    // for( uint32_t uIdx = 0; uIdx < fvbCanvasRegistered.size(); ++uIdx )

  return true;
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool Application::ResetHistograms()
{
  for (int iHist = 0; iHist < fArrayHisto.GetEntriesFast(); ++iHist) {
    dynamic_cast<TH1*>(fArrayHisto.At(iHist))->Reset();
  }  // for( int iHist = 0; iHist < fArrayHisto.GetEntriesFast(); ++iHist )
  return true;
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool Application::SaveHistograms()
{
  if ("" == fOpt.HistoFile()) {  //
    LOG(error) << "Filename for saving histograms and canvases not defined. Ignoring request.";
    return false;
  }

  /// Save old global file and folder pointer to avoid messing with FairRoot
  TFile* oldFile     = gFile;
  TDirectory* oldDir = gDirectory;

  /// (Re-)Create ROOT file to store the histos
  TFile* histoFile = nullptr;

  // open separate histo file in recreate mode
  histoFile = new TFile(fOpt.HistoFile().data(), fOpt.Overwrite() ? "RECREATE" : "CREATE");

  if (nullptr == histoFile) {  //
    gFile      = oldFile;
    gDirectory = oldDir;
    LOG(error) << "Ignoring request to save histograms and canvases: could not open output file " << fOpt.HistoFile();
    return false;
  }

  LOG(info) << "Saving Histograms and canvases in file: " << fOpt.HistoFile();

  /// Register the histos in the HTTP server
  for (UInt_t uHisto = 0; uHisto < fvHistos.size(); ++uHisto) {
    /// catch case of histograms declared in config but not yet received
    if (nullptr != fvHistos[uHisto].first) {
      /// Make sure we end up in chosen folder
      TString sFolder = fvHistos[uHisto].second.data();
      if (nullptr == gDirectory->Get(sFolder)) {  //
        gDirectory->mkdir(sFolder);
      }
      gDirectory->cd(sFolder);

      /// Write plot
      fvHistos[uHisto].first->Write();
    }

    histoFile->cd();
  }  // for( UInt_t uHisto = 0; uHisto < fvHistos.size(); ++uHisto )

  for (UInt_t uCanvas = 0; uCanvas < fvCanvas.size(); ++uCanvas) {
    /// catch case of canvases declared in config but for which not all histos were yet received
    if (nullptr != fvCanvas[uCanvas].first) {
      /// Make sure we end up in chosen folder
      TString sFolder = fvCanvas[uCanvas].second.data();
      if (nullptr == gDirectory->Get(sFolder)) {  //
        gDirectory->mkdir(sFolder);
      }
      gDirectory->cd(sFolder);

      /// Write plot
      fvCanvas[uCanvas].first->Write();
    }

    histoFile->cd();
  }  // for( UInt_t uHisto = 0; uHisto < fvCanvas.size(); ++uHisto )

  /// Restore old global file and folder pointer to avoid messing with FairRoot
  gFile      = oldFile;
  gDirectory = oldDir;

  histoFile->Close();

  return true;
}
