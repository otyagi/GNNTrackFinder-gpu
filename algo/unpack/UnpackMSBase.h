/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#pragma once

#include <utility>
#include <vector>

namespace fles
{
  class MicrosliceDescriptor;
}

namespace cbm::algo
{

  template<typename D, typename M, typename A>
  class UnpackMSBase {

   public:
    using Digi_t    = D;
    using Monitor_t = M;
    using Aux_t     = A;

    using Result_t = std::tuple<std::vector<Digi_t>, Monitor_t, Aux_t>;

    virtual ~UnpackMSBase() = default;

    virtual Result_t operator()(const uint8_t* msContent, const fles::MicrosliceDescriptor& msDescr,
                                const uint64_t tTimeslice) const = 0;
  };

}  // namespace cbm::algo
