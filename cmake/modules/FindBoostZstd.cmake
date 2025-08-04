# Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
# SPDX-License-Identifier: GPL-3.0-only
# Authors: Felix Weiglhofer [committer]

set(CMAKE_REQUIRED_LIBRARIES Boost::iostreams)
check_cxx_source_compiles("
#include <boost/iostreams/filter/zstd.hpp>
#include <boost/iostreams/filtering_stream.hpp>

int main(int argc, char *argv[])
{
  std::unique_ptr<boost::iostreams::filtering_istream> in_;
  in_ = std::make_unique<boost::iostreams::filtering_istream>();
  in_->push(boost::iostreams::zstd_decompressor());

  return 0;
}" BOOST_IOS_HAS_ZSTD)
unset(CMAKE_REQUIRED_LIBRARIES)

if(BOOST_IOS_HAS_ZSTD)
  message(STATUS "Boost::iostream with ZSTD filter found.")
else()
  message(STATUS "Boost::iostream does not support ZSTD filter.")
endif()
