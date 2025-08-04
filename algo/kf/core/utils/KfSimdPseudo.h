/* Copyright (C) 2010-2024 Frankfurt Institute for Advanced Studies, Goethe-Universität Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Igor Kulakov [committer] */

#pragma once  // include this header only once per compilation unit

#include <boost/serialization/access.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace cbm::algo::kf
{

  typedef float fscal;

  /**********************************
   *
   *   Pseudo SIMD vector
   *
   **********************************/

  fscal min(fscal x, fscal y);
  fscal max(fscal x, fscal y);
  fscal asgnb(fscal x, fscal y);
  fscal sgn(fscal x);


  constexpr auto VcMemAlign = 16;

  class fmask {

   public:
    static constexpr int Size{4};

    static constexpr size_t size() { return Size; }

    bool v[Size];

    bool& operator[](size_t i) { return v[i]; }
    bool operator[](size_t i) const { return v[i]; }

    fmask() : fmask(0.f) {}

    fmask(const fmask& a)
    {
      for (size_t i = 0; i < size(); i++) {
        v[i] = a.v[i];
      }
    }

    fmask(bool a)
    {
      for (size_t i = 0; i < size(); i++) {
        v[i] = a;
      }
    }

    static fmask One() { return fmask(true); }

    static fmask Zero() { return fmask(false); }

#define _op(A, B, F)                                                                                                   \
  fmask z;                                                                                                             \
  for (size_t i = 0; i < size(); i++) {                                                                                \
    z[i] = (A[i] F B[i]);                                                                                              \
  }                                                                                                                    \
  return z;

    /* Logic */
    friend fmask operator&&(const fmask& a, const fmask& b) { _op(a, b, &&) }
    friend fmask operator||(const fmask& a, const fmask& b) { _op(a, b, ||) }
    friend fmask operator&(const fmask& a, const fmask& b) { _op(a, b, &) }
    friend fmask operator|(const fmask& a, const fmask& b) { _op(a, b, |) }
    friend fmask operator^(const fmask& a, const fmask& b) { _op(a, b, ^) }

#undef _op

    friend fmask operator!(const fmask& a)
    {
      fmask z;
      for (size_t i = 0; i < size(); i++) {
        z[i] = !a[i];
      }
      return z;
    }

    bool isEmpty()
    {
      bool ret = true;
      for (size_t i = 0; i < size(); i++) {
        ret = ret && (!v[i]);
      }
      return ret;
    }

    bool isNotEmpty() { return !isEmpty(); }


    friend std::ostream& operator<<(std::ostream& strm, const fmask& a)
    {
      strm << '[';
      for (size_t i = 0; i < size(); i++) {
        strm << std::setw(12) << std::setfill(' ') << a[i] << ' ';
      }
      return strm;
    }

    friend std::istream& operator>>(std::istream& strm, fmask& a)
    {
      for (size_t i = 0; i < size(); i++) {
        strm >> a[i];
      }
      return strm;
    }
  };


  class fvec {

   public:
    static constexpr size_t size() { return fmask::size(); }

    fscal v[fmask::Size];

    fvec() : fvec(0.) {}

    fvec(const fvec& a)
    {
      for (size_t i = 0; i < size(); i++) {
        v[i] = a.v[i];
      }
    }

    fvec(fscal a)
    {
      for (size_t i = 0; i < size(); i++) {
        v[i] = a;
      }
    }

    /// Serialization block
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int)
    {
      for (size_t i = 0; i < size(); ++i) {
        ar& v[i];
      }
    }

    static fvec One() { return fvec(1.); }

    static fvec Zero() { return fvec(0.); }

    fscal& operator[](size_t i) { return v[i]; }

    fscal operator[](size_t i) const { return v[i]; }

    void setZero(fmask m)
    {
      for (size_t i = 0; i < size(); i++) {
        if (m[i]) {
          v[i] = 0.;
        }
      }
    }

#define _f1(A, F)                                                                                                      \
  fvec z;                                                                                                              \
  for (size_t i = 0; i < size(); i++) {                                                                                \
    z[i] = F(A[i]);                                                                                                    \
  }                                                                                                                    \
  return z;

#define _f2(A, B, F)                                                                                                   \
  fvec z;                                                                                                              \
  for (size_t i = 0; i < size(); i++) {                                                                                \
    z[i] = F(A[i], B[i]);                                                                                              \
  }                                                                                                                    \
  return z;

#define _op(A, B, F)                                                                                                   \
  fvec z;                                                                                                              \
  for (size_t i = 0; i < size(); i++) {                                                                                \
    z[i] = (A[i] F B[i]);                                                                                              \
  }                                                                                                                    \
  return z;

#define _opComp(A, B, F)                                                                                               \
  fmask z;                                                                                                             \
  for (size_t i = 0; i < size(); i++) {                                                                                \
    z[i] = (A[i] F B[i]);                                                                                              \
  }                                                                                                                    \
  return z;


    /* Arithmetic Operators */
    friend fvec operator+(const fvec& a, const fvec& b) { _op(a, b, +) }
    friend fvec operator-(const fvec& a, const fvec& b) { _op(a, b, -) }
    friend fvec operator*(const fvec& a, const fvec& b) { _op(a, b, *) }
    friend fvec operator/(const fvec& a, const fvec& b) { _op(a, b, /) }

    /* Comparison */
    friend fmask operator<(const fvec& a, const fvec& b) { _opComp(a, b, <) }
    friend fmask operator<=(const fvec& a, const fvec& b) { _opComp(a, b, <=) }
    friend fmask operator>(const fvec& a, const fvec& b) { _opComp(a, b, >) }
    friend fmask operator>=(const fvec& a, const fvec& b) { _opComp(a, b, >=) }
    friend fmask operator==(const fvec& a, const fvec& b) { _opComp(a, b, ==) }

    friend fmask isnan(const fvec& a)
    {
      fmask m;
      for (size_t i = 0; i < size(); i++) {
        m[i] = std::isnan(a[i]);
      }
      return m;
    }

    friend fvec iif(fmask a, fvec b, fvec c)
    {
      fvec z;
      for (size_t i = 0; i < size(); i++) {
        z[i] = a[i] ? b[i] : c[i];
      }
      return z;
    }

    /* Functions */
    friend fscal min(fscal x, fscal y) { return x < y ? x : y; }
    friend fscal max(fscal x, fscal y) { return x < y ? y : x; }
    friend fscal asgnb(fscal x, fscal y) { return y >= 0.f ? fabs(x) : -fabs(x); }
    friend fscal sgn(fscal x) { return x >= 0.f ? 1.f : -1.f; }

    friend fvec min(const fvec& a, const fvec& b) { _f2(a, b, min) }
    friend fvec max(const fvec& a, const fvec& b) { _f2(a, b, max) }
    friend fvec asgnb(const fvec& a, const fvec& b) { _f2(a, b, asgnb) }
    friend fvec sqrt(const fvec& a) { _f1(a, sqrt) }
    // friend fvec abs(const fvec& a) { _f1(a, fabs) } // disabled to avoid confusions, use fabs() instead
    friend fvec sgn(const fvec& a) { _f1(a, sgn) }
    friend fvec exp(const fvec& a) { _f1(a, exp) }
    friend fvec log(const fvec& a) { _f1(a, log) }
    friend fvec sin(const fvec& a) { _f1(a, sin) }
    friend fvec cos(const fvec& a) { _f1(a, cos) }
#undef _f1
#undef _f2
#undef _op
#undef _opComp

    /* Define all operators for consistensy */

    friend fvec operator-(const fvec& a) { return fvec(0) - a; }
    friend fvec operator+(const fvec& a) { return a; }

    friend void operator+=(fvec& a, const fvec& b) { a = a + b; }
    friend void operator-=(fvec& a, const fvec& b) { a = a - b; }
    friend void operator*=(fvec& a, const fvec& b) { a = a * b; }
    friend void operator/=(fvec& a, const fvec& b) { a = a / b; }

    friend std::ostream& operator<<(std::ostream& strm, const fvec& a)
    {
      //strm << "[" << a[0] << " " << a[1] << " " << a[2] << " " << a[3] << "]";
      strm << '[';
      for (size_t i = 0; i < size(); i++) {
        strm << std::setw(12) << std::setfill(' ') << a[i] << ' ';
      }
      return strm;
    }

    friend std::istream& operator>>(std::istream& strm, fvec& a)
    {
      for (size_t i = 0; i < size(); i++) {
        strm >> a[i];
      }
      return strm;
    }

  } __attribute__((aligned(16)));

#define _fvecalignment __attribute__((aligned(fvec::size() * sizeof(fscal))))

}  // namespace cbm::algo::kf
