/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#ifndef CBM_ALGO_BASE_HISTOGRAM_SENDER_H
#define CBM_ALGO_BASE_HISTOGRAM_SENDER_H

#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#ifdef BOOST_IOS_HAS_ZSTD
#include <boost/iostreams/filter/zstd.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#endif
#include <boost/iostreams/stream.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>

#include <string>
#include <string_view>
#include <zmq.hpp>

namespace cbm::algo
{

  class HistogramSender {
   public:
    HistogramSender(std::string_view address, int32_t hwm = 1, bool compression = false)
      : fHistComChan(address)
      , fHistHighWaterMark(hwm)
      , fbCompression(compression)
      , fZmqContext(1)
      , fZmqSocket(fZmqContext, zmq::socket_type::push)
    {
      fZmqSocket.set(zmq::sockopt::sndhwm, fHistHighWaterMark);  // High-Water Mark, max nb updates kept in out buffer
      fZmqSocket.connect(fHistComChan);  // This side "connects" to socket => Other side should have "bind"!!!!
    }

    /** @brief Serialize object and send it to the histogram server
       ** @param obj: object to be serialized in the message, e.g. config pairs of strings or QaData
       ** @param flags: or'ed values from zmq::send_flags, typ. zmq::send_flags::sndmore to indicate multi-parts message
       **/
    template<typename Object>
    void PrepareAndSendMsg(const Object& obj, zmq::send_flags flags)
    {
      /// Needed ressources (serializd string, boost inserter, boost stream, boost binary output archive)
      namespace b_io = boost::iostreams;
      namespace b_ar = boost::archive;

      std::string serial_str;
      b_io::back_insert_device<std::string> inserter(serial_str);
      b_io::stream<b_io::back_insert_device<std::string>> bstream(inserter);

      serial_str.clear();

      if (fbCompression) {
#ifdef BOOST_IOS_HAS_ZSTD
        std::unique_ptr<b_io::filtering_ostream> out_ = std::make_unique<b_io::filtering_ostream>();
        out_->push(b_io::zstd_compressor(b_io::zstd::best_speed));
        out_->push(bstream);
        std::unique_ptr<b_ar::binary_oarchive> oarchive_ =
          std::make_unique<b_ar::binary_oarchive>(*out_, b_ar::no_header);
        *oarchive_ << obj;
#else
        throw std::runtime_error("Unsupported ZSTD compression (boost) for histograms emissions channel");
#endif
      }
      else {
        b_ar::binary_oarchive oa(bstream);
        oa << obj;
      }
      bstream.flush();

      zmq::message_t msg(serial_str.size());
      std::copy_n(static_cast<const char*>(serial_str.data()), msg.size(), static_cast<char*>(msg.data()));
      /// FIXME: read return value to catch EAGAIN indicating a failed emission, use it outside to skip histo reset
      fZmqSocket.send(msg, flags | zmq::send_flags::dontwait);
    }

   private:
    std::string fHistComChan   = "tcp://127.0.0.1:56800";
    int32_t fHistHighWaterMark = 1;
    bool fbCompression         = false;
    zmq::context_t fZmqContext;  ///< ZMQ context FIXME: should be only one context per binary!
    zmq::socket_t fZmqSocket;    ///< ZMQ socket to histogram server
  };

}  // namespace cbm::algo

#endif
