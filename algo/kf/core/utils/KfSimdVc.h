/* Copyright (C) 2010-2024 Frankfurt Institute for Advanced Studies, Goethe-Universität Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Igor Kulakov [committer], Maksym Zyzak, Sergei Zharko */

#pragma once  // include this header only once per compilation unit

#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/split_free.hpp>

#include <Vc/Vc>

namespace cbm::algo::kf
{
  using fvec = Vc::float_v;
  //using fvec = Vc::double_v;
  //using fvec = Vc::Vector<float, Vc::VectorAbi::Scalar>;
  //using fvec = Vc::SimdArray<float, 4>;

  using fscal = fvec::EntryType;
  using fmask = fvec::MaskType;

  constexpr auto VcMemAlign = Vc::VectorAlignment;

// TODO: redefine via C++11 alignas
#define _fvecalignment __attribute__((aligned(Vc::VectorAlignment)))
}  // namespace cbm::algo::kf

/// \brief Serializer for SIMD vectors
namespace boost::serialization
{
  template<class Archive>
  void save(Archive& ar, const cbm::algo::kf::fvec& vect, unsigned int)
  {
    std::array<cbm::algo::kf::fscal, cbm::algo::kf::fvec::size()> buffer;
    for (size_t i = 0; i < cbm::algo::kf::fvec::size(); ++i) {
      buffer[i] = vect[i];
    }
    ar << buffer;
  }

  template<class Archive>
  void load(Archive& ar, cbm::algo::kf::fvec& vect, unsigned int)
  {
    std::array<cbm::algo::kf::fscal, cbm::algo::kf::fvec::size()> buffer;
    ar >> buffer;
    for (size_t i = 0; i < cbm::algo::kf::fvec::size(); ++i) {
      vect[i] = buffer[i];
    }
  }

  template<class Archive>
  void serialize(Archive& ar, cbm::algo::kf::fvec& vect, const unsigned int version)
  {
    split_free(ar, vect, version);
  }
}  // namespace boost::serialization
