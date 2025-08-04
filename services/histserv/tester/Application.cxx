/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#include "Application.h"

#include "CanvasConfig.h"
#include "CbmFlesCanvasTools.h"
#include "Histogram.h"
#include "PadConfig.h"
#include "QaData.h"
#include "ui_callbacks.h"

#include <Logger.h>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>

#include <thread>

std::mutex mtx;

namespace b_io = boost::iostreams;
namespace b_ar = boost::archive;
using sctp     = std::chrono::time_point<std::chrono::system_clock>;
using scsc     = std::chrono::system_clock;

namespace cbm::services::histserv_tester
{

  // -----   Constructor   ---------------------------------------------------------------------------------------------
  Application::Application(ProgramOptions const& opt) : fOpt(opt)
  {
    /// Read options from executable
    LOG(info) << "Options for Application.";
    LOG(info) << " Output ZMQ channel: " << fOpt.ComChan();
    LOG(info) << " Run time duration:  " << fOpt.Runtime() << " s";
    fpSender = std::make_shared<cbm::algo::HistogramSender>(fOpt.ComChan());

    /// FIXME: SOMETHING_To_Replace_FairMQ!!!!!!!!!!!!!
    /// FIXME: Initialize communication channels of SOMETHING_To_Replace_FairMQ
    /// FIXME: Link channel to method in order to process received messages
    // fZmqSocket.set(zmq::sockopt::rcvhwm, int(hwm));  // FIXME: need for HWM? (NOTE: SZh 29.02.2024: if needed, move it
    // in the HistogramSender)
  }
  // -------------------------------------------------------------------------------------------------------------------


  // -----   Main Loop   -----------------------------------------------------------------------------------------------
  void Application::Exec()
  {
    using cbm::algo::qa::CanvasConfig;
    using cbm::algo::qa::Data;
    using cbm::algo::qa::H1D;
    using cbm::algo::qa::PadConfig;

    const std::chrono::milliseconds interval {250};
    const std::chrono::seconds runtime(fOpt.Runtime());
    const std::chrono::seconds pubint(fOpt.PubInterval());
    sctp startTime   = scsc::now();
    sctp stopTime    = startTime + runtime;
    sctp lastPubTime = startTime;

    // Init QA data helper
    Data qaData("Test");
    auto* pHistTest  = qaData.MakeObj<H1D>("testHist", "Tester source; Runtime [s]; Entries to histo [iterations]",
                                          fOpt.Runtime() + 2, -1.0, fOpt.Runtime() + 1.0);
    auto* pHistTrans = qaData.MakeObj<H1D>(
      "transHist", "Tester histos transmission time; Trans. time [ms]; Messages []", 1001, -0.025, 50.025);

    /// Initial emission, including generation and serialization of configs
    /// => Try to evaluate "time cost" of the histo transmission, including serialization
    sctp transStartTime = scsc::now();

    {
      auto canv = CanvasConfig("TestCanvas", "TestCanvas");
      auto pad1 = PadConfig();
      pad1.SetGrid(false, false);
      pad1.SetLog(false, false, false);
      pad1.RegisterHistogram(pHistTest, "hist");
      canv.AddPadConfig(pad1);
      auto pad2 = PadConfig();
      pad2.SetGrid(true, true);
      pad2.SetLog(false, false, false);
      pad2.RegisterHistogram(pHistTest, "hist");
      canv.AddPadConfig(pad2);
      auto pad3 = PadConfig();
      pad3.SetGrid(true, true);
      pad3.SetLog(true, false, false);
      pad3.RegisterHistogram(pHistTrans, "hist");
      canv.AddPadConfig(pad3);
      auto pad4 = PadConfig();
      pad4.SetGrid(true, true);
      pad4.SetLog(true, false, false);
      pad4.RegisterHistogram(pHistTrans, "hist");
      pad4.RegisterHistogram(pHistTest, "hist same");
      canv.AddPadConfig(pad4);
      qaData.AddCanvasConfig(canv);
    }
    qaData.Init(fpSender);  // Init the QA data

    lastPubTime = scsc::now();

    /// No general references as member/variable bec. simple example, use directly hardcoded vector "array access"
    pHistTrans->Fill(std::chrono::duration_cast<std::chrono::microseconds>(lastPubTime - transStartTime).count() / 1e3);

    while (scsc::now() < stopTime) {  //
      /// No general references as member/variable bec. simple example, use directly hardcoded vector "array access"
      pHistTest->Fill(std::chrono::duration_cast<std::chrono::milliseconds>(scsc::now() - startTime).count() / 1e3);

      if (pubint < scsc::now() - lastPubTime) {
        LOG(info) << "Pub-hist: " << scsc::now().time_since_epoch().count() << " " << pHistTest->GetEntries();
        /// Try to evaluate "time cost" of the histo transmission, including serialization
        transStartTime = scsc::now();

        /// => Histograms serialization and emission
        qaData.Send(fpSender);

        lastPubTime = scsc::now();

        /// No general references as member/variable bec. simple example, use directly hardcoded vector "array access"
        pHistTrans->Fill(std::chrono::duration_cast<std::chrono::microseconds>(lastPubTime - transStartTime).count()
                         / 1e3);
      }

      std::this_thread::sleep_for(interval);
      LOG(info) << "test: " << (scsc::now() < stopTime) << " (" << scsc::now().time_since_epoch().count() << " vs "
                << stopTime.time_since_epoch().count() << ") => "
                << (std::chrono::duration_cast<std::chrono::milliseconds>(scsc::now() - startTime).count() / 1e3);
    }

    /// Final publications
    LOG(info) << "Pub-hist: " << scsc::now().time_since_epoch().count() << " " << pHistTest->GetEntries();
    /// => Histograms serialization and emission
    qaData.Send(fpSender);
  }
}  // namespace cbm::services::histserv_tester
