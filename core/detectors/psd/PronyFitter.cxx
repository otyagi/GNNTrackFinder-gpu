/* Copyright (C) 2020-2021 Institute for Nuclear Research, Moscow
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Nikolay Karpushkin [committer] */

#include "PronyFitter.h"

#include "PolynomComplexRoots.h"
#include "PolynomRealRoots.h"

namespace PsdSignalFitting
{


  PronyFitter::PronyFitter(int model_order, int exponent_number, int gate_beg, int gate_end)
  {
    Initialize(model_order, exponent_number, gate_beg, gate_end);
  }

  void PronyFitter::Initialize(int model_order, int exponent_number, int gate_beg, int gate_end)
  {
    fModelOrder = model_order;
    fExpNumber  = exponent_number;
    fGateBeg    = gate_beg;
    fGateEnd    = gate_end;
    AllocData();
  }

  void PronyFitter::AllocData()
  {
    fz = new std::complex<float>[fExpNumber + 1];
    fh = new std::complex<float>[fExpNumber + 1];
    for (int i = 0; i < fExpNumber + 1; i++) {
      fz[i] = {0., 0.};
      fh[i] = {0., 0.};
    }
  }

  void PronyFitter::SetWaveform(std::vector<uint16_t>& uWfm, uint16_t uZeroLevel)
  {
    fuWfm        = uWfm;
    fuZeroLevel  = uZeroLevel;
    fSampleTotal = fuWfm.size();
    fuFitWfm.resize(fSampleTotal);
  }

  int PronyFitter::CalcSignalBegin(float front_time_beg_03, float front_time_end)
  {
    return std::ceil((3 * front_time_beg_03 - front_time_end) / 2.);
  }

  void PronyFitter::SetSignalBegin(int SignalBeg)
  {
    fSignalBegin = SignalBeg;
    if (fIsDebug) printf("\nsignal begin %i  zero level %u\n", fSignalBegin, fuZeroLevel);
  }

  void PronyFitter::CalculateFitHarmonics()
  {
    float rho_f = 999;
    float rho_b = 999;
    std::vector<float> a_f;
    std::vector<float> a_b;

    CovarianceDirect(rho_f, a_f, rho_b, a_b);
    //CovarianceQRmod(rho_f, a_f, rho_b, a_b);

    if (fIsDebug) {
      printf("LSerr %e, SLE roots forward  ", rho_f);
      for (uint8_t i = 0; i < a_f.size(); i++)
        printf(" %e ", a_f.at(i));
      printf("\n");
      printf("LSerr %e, SLE roots backward ", rho_b);
      for (uint8_t i = 0; i < a_b.size(); i++)
        printf(" %e ", a_b.at(i));
      printf("\n");
    }

    float* a_arr = new float[fModelOrder + 1];
    for (int i = 0; i < fModelOrder + 1; i++)
      a_arr[i] = 0.;

    //for(uint8_t i = 0; i < a_f.size(); i++) a_arr[i+1] = 0.5*(a_f.at(i) + a_b.at(i));
    for (uint8_t i = 0; i < a_f.size(); i++)
      a_arr[i + 1] = a_f.at(i);
    a_arr[0] = 1.;

    float* zr = new float[fModelOrder];
    float* zi = new float[fModelOrder];
    for (int i = 0; i < fModelOrder; i++) {
      zr[i] = 0.;
      zi[i] = 0.;
    }

    int total_roots;
    //polynomRealRoots(zr, fModelOrder, a_arr, total_roots);
    polynomComplexRoots(zr, zi, fModelOrder, a_arr, total_roots);
    if (fIsDebug) {
      printf("forward polinom roots ");
      for (int i = 0; i < fModelOrder; i++)
        printf("%.5f%+.5fi   ", zr[i], zi[i]);
      printf("\n");

      printf("forward freqs ");
      for (int i = 0; i < fModelOrder; i++)
        printf("%.5f ", log(zr[i]));
      printf("\n");
    }

    std::complex<float>* z_arr = new std::complex<float>[fExpNumber + 1];
    for (int i = 0; i < fExpNumber; i++) {
      if (std::isfinite(zr[i])) z_arr[i + 1] = {zr[i], zi[i]};
      else
        z_arr[i + 1] = 0.;
    }
    z_arr[0] = {1., 0.};
    SetHarmonics(z_arr);
    fTotalPolRoots = total_roots;

    delete[] a_arr;
    delete[] zr;
    delete[] zi;
    delete[] z_arr;
  }

  void PronyFitter::CovarianceDirect(float& /*rho_f*/, std::vector<float>& a_f, float& /*rho_b*/,
                                     std::vector<float>& /*a_b*/)
  {
    std::vector<int32_t> kiWfmSignal;
    //filtering constant fraction
    for (int sample_curr = fSignalBegin; sample_curr <= fGateEnd; sample_curr++)
      kiWfmSignal.push_back(fuWfm.at(sample_curr) - fuZeroLevel);
    int n = kiWfmSignal.size();

    float** Rkm_arr = new float*[fModelOrder];
    for (int i = 0; i < fModelOrder; i++) {
      Rkm_arr[i] = new float[fModelOrder];
      for (int j = 0; j < fModelOrder; j++)
        Rkm_arr[i][j] = 0.;
    }

    float* R0k_arr = new float[fModelOrder];
    for (int i = 0; i < fModelOrder; i++)
      R0k_arr[i] = 0.;

    //Regression via linear prediction forward
    for (int i = 1; i <= fModelOrder; i++)
      for (int j = 1; j <= fModelOrder; j++)
        for (int sample_curr = fModelOrder; sample_curr < n; sample_curr++)
          Rkm_arr[i - 1][j - 1] += (float) (kiWfmSignal.at(sample_curr - i) * kiWfmSignal.at(sample_curr - j));

    for (int i = 1; i <= fModelOrder; i++)
      for (int sample_curr = fModelOrder; sample_curr < n; sample_curr++)
        R0k_arr[i - 1] -= (float) (kiWfmSignal.at(sample_curr) * kiWfmSignal.at(sample_curr - i));

    if (fIsDebug) {
      printf("system forward\n");
      for (int i = 0; i < fModelOrder; i++) {
        for (int j = 0; j < fModelOrder; j++)
          printf("%e ", Rkm_arr[i][j]);
        printf("%e\n", R0k_arr[i]);
      }
    }

    float* a = new float[fModelOrder];
    for (int i = 0; i < fModelOrder; i++)
      a[i] = 0.;

    SolveSLEGauss(a, Rkm_arr, R0k_arr, fModelOrder);
    //SolveSLECholesky(a, Rkm_arr, R0k_arr, fModelOrder);
    if (fIsDebug) {
      printf("SLE roots ");
      for (int i = 0; i < fModelOrder; i++)
        printf(" %e ", a[i]);
      printf("\n");
    }

    a_f.resize(fModelOrder);
    for (int i = 0; i < fModelOrder; i++)
      a_f.at(i) = a[i];

    for (int i = 0; i < fModelOrder; i++)
      delete[] Rkm_arr[i];
    delete[] Rkm_arr;
    delete[] R0k_arr;
    delete[] a;
  }

  void PronyFitter::CovarianceQRmod(float& rho_f, std::vector<float>& a_f, float& rho_b, std::vector<float>& a_b)
  {

    /*
% Copyright (c) 2019 by S. Lawrence Marple Jr.
%
%
p
--
order of linear prediction/autoregressive filter
%
x
--
vector of data samples
%
rho_f
--
least squares estimate of forward linear prediction variance
%
a_f
--
vector of forward linear prediction/autoregressive
parameters
%
rho_b
--
least squares estimate of backward linear prediction
variance
%
a_b
--
vector of backward linear prediction/autoregressive
parameters

*/


    //******** Initialization ********

    std::vector<int32_t> kiWfmSignal;
    //filtering constant fraction
    for (int sample_curr = fSignalBegin; sample_curr <= fGateEnd; sample_curr++)
      kiWfmSignal.push_back(fuWfm.at(sample_curr) - fuZeroLevel);
    int n = kiWfmSignal.size();
    if (2 * fModelOrder + 1 > n) {
      if (fIsDebug) printf("ERROR: Order too high; will make solution singular\n");
      return;
    }

    float r1 = 0.;
    for (int k = 1; k <= n - 2; k++)
      r1 += std::pow(kiWfmSignal.at(k), 2);

    float r2 = std::pow(kiWfmSignal.front(), 2);
    float r3 = std::pow(kiWfmSignal.back(), 2);

    rho_f = r1 + r3;
    rho_b = r1 + r2;
    r1    = 1. / (rho_b + r3);

    std::vector<float> c, d;
    c.push_back(kiWfmSignal.back() * r1);
    d.push_back(kiWfmSignal.front() * r1);

    float gam = 0.;
    float del = 0.;
    std::vector<float> ef, eb, ec, ed;
    std::vector<float> coeffs;
    coeffs.resize(6);

    for (int v_iter = 0; v_iter < n; v_iter++) {
      ef.push_back(kiWfmSignal.at(v_iter));
      eb.push_back(kiWfmSignal.at(v_iter));
      ec.push_back(c.at(0) * kiWfmSignal.at(v_iter));
      ed.push_back(d.at(0) * kiWfmSignal.at(v_iter));
    }

    //******** Main Recursion ********

    for (int k = 1; k <= fModelOrder; k++) {
      if (rho_f <= 0 || rho_b <= 0) {
        if (fIsDebug)
          printf("PsdPronyFitter::ERROR: prediction squared error was less "
                 "than or equal to zero\n");
        return;
      }

      gam = 1. - ec.at(n - k);
      del = 1. - ed.front();
      if (gam <= 0 || gam > 1 || del <= 0 || del > 1) {
        if (fIsDebug)
          printf("PsdPronyFitter::ERROR: GAM or DEL gain factor not in "
                 "expected range 0 to 1\n");
        return;
      }

      // computation for k-th order reflection coefficients
      std::vector<float> eff, ebb;
      eff.resize(n);
      ebb.resize(n);
      float delta = 0.;
      for (int v_iter = 0; v_iter < n - 1; v_iter++) {
        eff.at(v_iter) = ef.at(v_iter + 1);
        ebb.at(v_iter) = eb.at(v_iter);
        delta += eff.at(v_iter) * ebb.at(v_iter);
      }

      float k_f = -delta / rho_b;
      float k_b = -delta / rho_f;

      //% order updates for squared prediction errors rho_f and rho_b
      rho_f = rho_f * (1 - k_f * k_b);
      rho_b = rho_b * (1 - k_f * k_b);

      //% order updates for linear prediction parameter arrays a_f and a_b
      std::vector<float> temp_af = a_f;
      std::reverse(std::begin(a_b), std::end(a_b));
      for (uint8_t i = 0; i < a_f.size(); i++)
        a_f.at(i) += k_f * a_b.at(i);
      a_f.push_back(k_f);

      std::reverse(std::begin(a_b), std::end(a_b));
      std::reverse(std::begin(temp_af), std::end(temp_af));
      for (uint8_t i = 0; i < a_b.size(); i++)
        a_b.at(i) += k_b * temp_af.at(i);
      a_b.push_back(k_b);

      //% check if maximum order has been reached
      if (k == fModelOrder) {
        rho_f = rho_f / (n - fModelOrder);
        rho_b = rho_b / (n - fModelOrder);
        return;
      }

      //% order updates for prediction error arrays ef and eb
      for (int v_iter = 0; v_iter < n; v_iter++) {
        eb.at(v_iter) = ebb.at(v_iter) + k_b * eff.at(v_iter);
        ef.at(v_iter) = eff.at(v_iter) + k_f * ebb.at(v_iter);
      }

      //% coefficients for next set of updates
      coeffs.at(0) = ec.front();
      coeffs.at(1) = coeffs.at(0) / del;
      coeffs.at(2) = coeffs.at(0) / gam;

      //% time updates for gain arrays c' and d"
      std::vector<float> temp_c = c;
      for (uint8_t v_iter = 0; v_iter < c.size(); v_iter++) {
        c.at(v_iter) += coeffs.at(1) * d.at(v_iter);
        d.at(v_iter) += coeffs.at(2) * temp_c.at(v_iter);
      }

      //% time updates for ec' and ed"
      std::vector<float> temp_ec = ec;
      for (int v_iter = 0; v_iter < n; v_iter++) {
        ec.at(v_iter) += coeffs.at(1) * ed.at(v_iter);
        ed.at(v_iter) += coeffs.at(2) * temp_ec.at(v_iter);
      }

      std::vector<float> ecc, edd;
      ecc.resize(n);
      edd.resize(n);
      for (int v_iter = 0; v_iter < n - 1; v_iter++) {
        ecc.at(v_iter) = ec.at(v_iter + 1);
        edd.at(v_iter) = ed.at(v_iter);
      }

      if (rho_f <= 0 || rho_b <= 0) {
        if (fIsDebug)
          printf("PsdPronyFitter::ERROR2: prediction squared error was less "
                 "than or equal to zero\n");
        return;
      }
      gam = 1 - ecc.at(n - k - 1);  //n-k?
      del = 1 - edd.front();
      if (gam <= 0 || gam > 1 || del <= 0 || del > 1) {
        if (fIsDebug)
          printf("PsdPronyFitter::ERROR2: GAM or DEL gain factor not in "
                 "expected range 0 to 1\n");
        return;
      }


      //% coefficients for next set of updates
      coeffs.at(0) = ef.front();
      coeffs.at(1) = eb.at(n - k - 1);  //n-k?
      coeffs.at(2) = coeffs.at(1) / rho_b;
      coeffs.at(3) = coeffs.at(0) / rho_f;
      coeffs.at(4) = coeffs.at(0) / del;
      coeffs.at(5) = coeffs.at(1) / gam;

      //% order updates for c and d; time updates for a_f' and a_b"
      std::vector<float> temp_ab = a_b;
      std::reverse(std::begin(temp_ab), std::end(temp_ab));
      std::reverse(std::begin(c), std::end(c));

      for (uint8_t i = 0; i < a_b.size(); i++)
        a_b.at(i) += coeffs.at(5) * c.at(i);
      std::reverse(std::begin(c), std::end(c));

      for (uint8_t i = 0; i < c.size(); i++)
        c.at(i) += coeffs.at(2) * temp_ab.at(i);
      c.push_back(coeffs.at(2));

      std::vector<float> temp_af2 = a_f;
      for (uint8_t i = 0; i < a_f.size(); i++)
        a_f.at(i) += coeffs.at(4) * d.at(i);

      for (uint8_t i = 0; i < d.size(); i++)
        d.at(i) += coeffs.at(3) * temp_af2.at(i);
      d.insert(d.begin(), coeffs.at(3));

      //% time updates for rho_f' and rho_b"
      rho_f = rho_f - coeffs.at(4) * coeffs.at(0);
      rho_b = rho_b - coeffs.at(5) * coeffs.at(1);

      if (rho_f <= 0 || rho_b <= 0) {
        if (fIsDebug)
          printf("PsdPronyFitter::ERROR3: prediction squared error was less "
                 "than or equal to zero\n");
        return;
      }

      //% order updates for ec and ed; time updates for ef' and eb"
      for (int v_iter = 0; v_iter < n; v_iter++) {
        ec.at(v_iter) = ecc.at(v_iter) + coeffs.at(2) * eb.at(v_iter);
        eb.at(v_iter) = eb.at(v_iter) + coeffs.at(5) * ecc.at(v_iter);
        ed.at(v_iter) = edd.at(v_iter) + coeffs.at(3) * ef.at(v_iter);
        ef.at(v_iter) = ef.at(v_iter) + coeffs.at(4) * edd.at(v_iter);
      }
    }
  }

  void PronyFitter::SetHarmonics(std::complex<float>* z)
  {
    std::memcpy(fz, z, (fExpNumber + 1) * sizeof(std::complex<float>));
  }

  void PronyFitter::SetExternalHarmonics(std::complex<float> z1, std::complex<float> z2)
  {
    std::complex<float>* z_arr = new std::complex<float>[fExpNumber + 1];
    for (int i = 0; i <= fExpNumber; i++)
      z_arr[i] = {0., 0.};
    z_arr[0] = {1., 0.};
    z_arr[1] = z1;
    z_arr[2] = z2;
    SetHarmonics(z_arr);
    delete[] z_arr;
  }

  std::complex<float>* PronyFitter::GetHarmonics() { return fz; }

  int PronyFitter::GetNumberPolRoots() { return fTotalPolRoots; }

  void PronyFitter::MakePileUpRejection(int /*time_max*/) {}

  void PronyFitter::CalculateFitAmplitudes()
  {
    std::complex<float>** Zik_arr = new std::complex<float>*[fExpNumber + 1];
    for (int i = 0; i < fExpNumber + 1; i++) {
      Zik_arr[i] = new std::complex<float>[fExpNumber + 1];
      for (int j = 0; j < fExpNumber + 1; j++)
        Zik_arr[i][j] = {0., 0.};
    }

    std::complex<float>* Zyk_arr = new std::complex<float>[fExpNumber + 1];
    for (int i = 0; i < fExpNumber + 1; i++)
      Zyk_arr[i] = {0., 0.};

    int samples_in_gate            = fGateEnd - fSignalBegin + 1;
    const std::complex<float> unit = {1., 0.};

    for (int i = 0; i <= fExpNumber; i++) {
      for (int j = 0; j <= fExpNumber; j++) {
        std::complex<float> temp = std::conj(fz[i]) * fz[j];
        if (std::abs(temp - unit) > 1e-3) {
          Zik_arr[i][j] = static_cast<std::complex<float>>((std::pow(temp, static_cast<float>(samples_in_gate)) - unit)
                                                           / (temp - unit));
        }
        else {
          Zik_arr[i][j] = static_cast<std::complex<float>>(samples_in_gate);
        }
      }
    }

    std::complex<float>* z_power = new std::complex<float>[fExpNumber + 1];
    for (int i = 0; i < fExpNumber + 1; i++)
      z_power[i] = unit;

    for (int i = 0; i <= fExpNumber; i++) {
      for (int sample_curr = fSignalBegin; sample_curr <= fGateEnd; sample_curr++) {
        Zyk_arr[i] += (std::complex<float>) (std::conj(z_power[i]) * (float) fuWfm.at(sample_curr));
        z_power[i] *= fz[i];
      }
    }

    if (fIsDebug) {
      printf("\nampl calculation\n");
      for (int i = 0; i <= fExpNumber; i++) {
        for (int j = 0; j <= fExpNumber; j++)
          printf("%e%+ei   ", std::real(Zik_arr[i][j]), std::imag(Zik_arr[i][j]));
        printf("        %e%+ei\n", std::real(Zyk_arr[i]), std::imag(Zyk_arr[i]));
      }
    }

    SolveSLEGauss(fh, Zik_arr, Zyk_arr, fExpNumber + 1);

    if (fIsDebug) {
      printf("amplitudes\n%.0f%+.0fi ", std::real(fh[0]), std::imag(fh[0]));
      for (int i = 1; i < fExpNumber + 1; i++)
        printf("%e%+ei ", std::real(fh[i]), std::imag(fh[i]));
      printf("\n\n");
    }

    for (int i = 0; i < fExpNumber + 1; i++)
      z_power[i] = unit;

    std::complex<float> fit_ampl_in_sample = {0., 0.};
    fuFitZeroLevel                         = (uint16_t) std::real(fh[0]);
    for (int sample_curr = 0; sample_curr < fSampleTotal; sample_curr++) {
      fit_ampl_in_sample = {0., 0.};
      if ((sample_curr >= fSignalBegin)) {  //&& (sample_curr <= fGateEnd)) {
        for (int i = 0; i < fExpNumber + 1; i++) {
          fit_ampl_in_sample += fh[i] * z_power[i];
          z_power[i] *= fz[i];
        }
        fuFitWfm.at(sample_curr) = (uint16_t) std::real(fit_ampl_in_sample);
      }
      else
        fuFitWfm.at(sample_curr) = fuFitZeroLevel;
    }

    if (fIsDebug) {
      printf("waveform:\n");
      for (uint8_t i = 0; i < fuWfm.size(); i++)
        printf("%u ", fuWfm.at(i));

      printf("\nfit waveform:\n");
      for (uint8_t i = 0; i < fuFitWfm.size(); i++)
        printf("%u ", fuFitWfm.at(i));

      printf("\nzero level %u\n\n", fuZeroLevel);
    }

    for (int i = 0; i < fExpNumber + 1; i++)
      delete[] Zik_arr[i];
    delete[] Zik_arr;
    delete[] Zyk_arr;
    delete[] z_power;
  }


  std::complex<float>* PronyFitter::GetAmplitudes() { return fh; }

  float PronyFitter::GetIntegral(int gate_beg, int gate_end)
  {
    float integral = 0.;
    for (int sample_curr = gate_beg; sample_curr <= gate_end; sample_curr++)
      integral += (float) fuFitWfm.at(sample_curr) - fuFitZeroLevel;

    if (std::isfinite(integral)) return integral;
    return 0;
  }

  uint16_t PronyFitter::GetFitValue(int sample_number)
  {
    if (std::isfinite(fuFitWfm.at(sample_number))) return fuFitWfm.at(sample_number);
    return 0;
  }

  float PronyFitter::GetFitValue(float x)
  {
    std::complex<float> amplitude = {0., 0.};
    if (x < GetSignalBeginFromPhase()) return std::real(fh[0]);
    amplitude += fh[0];
    for (int i = 1; i < fExpNumber + 1; i++)
      amplitude += fh[i] * std::pow(fz[i], x - fSignalBegin);

    if (std::isfinite(std::real(amplitude))) return std::real(amplitude);
    return 0;
  }

  float PronyFitter::GetZeroLevel() { return (float) fuFitZeroLevel; }

  float PronyFitter::GetSignalMaxTime()
  {
    return fSignalBegin
           + std::real((std::log(-fh[2] * std::log(fz[2])) - std::log(fh[1] * log(fz[1])))
                       / (std::log(fz[1]) - std::log(fz[2])));
  }

  float PronyFitter::GetSignalBeginFromPhase()
  {
    if (std::real(fh[2] / fh[1]) < 0)
      return fSignalBegin + std::real(std::log(-fh[2] / fh[1]) / std::log(fz[1] / fz[2]));
    return -999.;
  }

  float PronyFitter::GetMaxAmplitude() { return GetFitValue(GetSignalMaxTime()); }

  float PronyFitter::GetX(float level, int first_sample, int last_sample)
  {
    int step = 0;
    if (first_sample < last_sample) step = 1;
    else
      step = -1;
    float result_sample  = 0.;
    int sample_to_check  = first_sample;
    float amplitude      = 0.;
    float amplitude_prev = GetFitValue(sample_to_check - step);
    while ((first_sample - sample_to_check) * (last_sample - sample_to_check) <= 0) {
      amplitude = GetFitValue(sample_to_check);
      if ((level - amplitude) * (level - amplitude_prev) <= 0) {
        result_sample = LevelBy2Points(sample_to_check, amplitude, sample_to_check - step, amplitude_prev, level);
        return result_sample;
      }
      amplitude_prev = amplitude;
      sample_to_check += step;
    }

    return 0;
  }

  float PronyFitter::GetX(float level, int first_sample, int last_sample, float step)
  {
    float result_sample                = 0.;
    float sample_to_check              = (float) first_sample;
    std::complex<float> amplitude      = {0., 0.};
    std::complex<float> amplitude_prev = GetFitValue(sample_to_check - step);
    while ((first_sample - sample_to_check) * (last_sample - sample_to_check) <= 0) {
      amplitude = GetFitValue(sample_to_check);
      if ((level - std::real(amplitude)) * (level - std::real(amplitude_prev)) <= 0) {
        if (amplitude != amplitude_prev)
          result_sample = LevelBy2Points(sample_to_check, std::real(amplitude), sample_to_check - step,
                                         std::real(amplitude_prev), level);
        return result_sample;
      }
      amplitude_prev = amplitude;
      sample_to_check += step;
    }

    return 0;
  }

  float PronyFitter::LevelBy2Points(float X1, float Y1, float X2, float Y2, float Y0)
  {
    return (X1 * Y0 - X1 * Y2 - X2 * Y0 + X2 * Y1) / (Y1 - Y2);
  }

  float PronyFitter::GetRSquare(int gate_beg, int gate_end)
  {
    float R2          = 0.;
    float RSS         = 0.;
    float TSS         = 0.;
    int m             = gate_end - gate_beg + 1;
    int params_number = 1 + 2 * fModelOrder;
    if (m <= params_number) return 999;
    float average = 0.;
    for (int sample_curr = gate_beg; sample_curr <= gate_end; sample_curr++)
      average += fuWfm.at(sample_curr);
    average /= m;

    for (int sample_curr = gate_beg; sample_curr <= gate_end; sample_curr++) {
      RSS += std::pow(fuFitWfm.at(sample_curr) - fuWfm.at(sample_curr), 2);
      TSS += std::pow(fuWfm.at(sample_curr) - average, 2);
    }
    if (TSS == 0) return 999;
    R2 = RSS / TSS;  // correct definition is R2=1.-RSS/TSS, but R2=RSS/TSS is more convenient

    float R2_adj = R2 * (m - 1) / (m - params_number);
    return R2_adj;
  }

  float PronyFitter::GetRSquareSignal() { return GetRSquare(fSignalBegin, fGateEnd); }

  float PronyFitter::GetChiSquare(int gate_beg, int gate_end, int time_max)
  {
    float chi2          = 0.;
    int freedom_counter = 0;
    int regions_number  = 10;
    float amplitude_max = std::abs(fuWfm.at(time_max) - fuZeroLevel);
    if (amplitude_max == 0) return 999;

    int* probability_exp     = new int[regions_number];
    int* probability_theor   = new int[regions_number];
    float* amplitude_regions = new float[regions_number + 1];
    amplitude_regions[0]     = 0.;
    for (int i = 0; i < regions_number; i++) {
      probability_exp[i]       = 0;
      probability_theor[i]     = 0;
      amplitude_regions[i + 1] = (i + 1) * amplitude_max / regions_number;
    }

    for (int sample_curr = gate_beg; sample_curr <= gate_end; sample_curr++) {
      for (int i = 0; i < regions_number; i++) {
        if ((std::abs(fuWfm.at(sample_curr) - fuZeroLevel) > amplitude_regions[i])
            && (std::abs(fuWfm.at(sample_curr) - fuZeroLevel) <= amplitude_regions[i + 1]))
          probability_exp[i]++;
        if ((std::abs(fuFitWfm.at(sample_curr) - fuFitZeroLevel) > amplitude_regions[i])
            && (std::abs(fuFitWfm.at(sample_curr) - fuFitZeroLevel) <= amplitude_regions[i + 1]))
          probability_theor[i]++;
      }
    }

    for (int i = 0; i < regions_number; i++) {
      if (probability_exp[i] > 0) {
        chi2 += std::pow(probability_exp[i] - probability_theor[i], 2.) / (probability_exp[i]);
        freedom_counter++;
      }
    }

    if (freedom_counter > 0) chi2 /= freedom_counter;
    delete[] probability_exp;
    delete[] probability_theor;
    delete[] amplitude_regions;

    return chi2;
  }

  float PronyFitter::GetDeltaInSample(int sample) { return fuFitWfm.at(sample) - fuWfm.at(sample); }

  /*
void PronyFitter::DrawFit(TObjArray *check_fit_arr, TString hist_title)
{
   float *sample_arr = new float[fSampleTotal];
   for(int i = 0; i < fSampleTotal; i++)
       sample_arr[i] = (float) i;

   TGraph* tgr_ptr = new TGraph( fSampleTotal, sample_arr, fuWfm);
   TGraph* tgr_ptr_fit = new TGraph( fSampleTotal, sample_arr, fuFitWfm);
   TCanvas *canv_ptr = new TCanvas(hist_title.Data());
   tgr_ptr->SetTitle(hist_title.Data());
   tgr_ptr->Draw();

   tgr_ptr_fit->SetLineColor(kRed);
   tgr_ptr_fit->SetLineWidth(2);
   tgr_ptr_fit->Draw("same");

   check_fit_arr->Add(canv_ptr);

   delete[] sample_arr;
}
*/

  int PronyFitter::ChooseBestSignalBeginHarmonics(int first_sample, int last_sample)
  {
    float best_R2         = 0.;
    int best_signal_begin = 0;
    bool IsReasonableRoot;
    bool IsGoodFit       = false;
    int good_fit_counter = 0;

    for (int signal_begin = first_sample; signal_begin <= last_sample; signal_begin++) {
      SetSignalBegin(signal_begin);
      CalculateFitHarmonics();
      IsReasonableRoot = true;
      for (int j = 0; j < fExpNumber; j++)
        IsReasonableRoot = IsReasonableRoot && (std::abs(fz[j + 1]) > 1e-6) && (std::abs(fz[j + 1]) < 1e1);
      IsGoodFit = (fTotalPolRoots > 0) && (IsReasonableRoot);

      if (IsGoodFit) {
        if (fIsDebug) printf("good fit candidate at signal begin %i\n", signal_begin);
        good_fit_counter++;
        CalculateFitAmplitudes();
        float R2 = GetRSquare(fGateBeg, fGateEnd);
        if (good_fit_counter == 1) {
          best_R2           = R2;
          best_signal_begin = signal_begin;
        }
        if (R2 < best_R2) {
          best_R2           = R2;
          best_signal_begin = signal_begin;
        }
      }
    }

    return best_signal_begin;
  }

  int PronyFitter::ChooseBestSignalBegin(int first_sample, int last_sample)
  {
    float best_R2         = 0.;
    int best_signal_begin = first_sample;

    for (int signal_begin = first_sample; signal_begin <= last_sample; signal_begin++) {
      SetSignalBegin(signal_begin);
      CalculateFitAmplitudes();
      float R2 = GetRSquare(fGateBeg, fGateEnd);
      if (signal_begin == first_sample) best_R2 = R2;
      if (R2 < best_R2) {
        best_R2           = R2;
        best_signal_begin = signal_begin;
      }
    }

    return best_signal_begin;
  }

  void PronyFitter::SolveSLEGauss(float* x, float** r, float* b, int n)
  {
    bool solvable = true;
    int maxRow;
    float maxEl, tmp, c;
    float** a = new float*[n];
    for (int i = 0; i < n; i++) {
      a[i] = new float[n + 1];
      for (int j = 0; j < n + 1; j++)
        a[i][j] = 0.;
    }

    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++)
        a[i][j] = r[i][j];
      a[i][n] = b[i];
    }

    for (int i = 0; i < n; i++) {
      maxEl  = std::abs(a[i][i]);
      maxRow = i;
      for (int k = i + 1; k < n; k++)
        if (abs(a[k][i]) > maxEl) {
          maxEl  = std::abs(a[k][i]);
          maxRow = k;
        }

      if (maxEl == 0) {
        solvable = false;
        if (fIsDebug) printf("SLE has not solution\n");
      }

      for (int k = i; k < n + 1; k++) {
        tmp          = a[maxRow][k];
        a[maxRow][k] = a[i][k];
        a[i][k]      = tmp;
      }

      for (int k = i + 1; k < n; k++) {
        c = -a[k][i] / a[i][i];
        for (int j = i; j < n + 1; j++) {
          if (i == j) a[k][j] = 0.;
          else
            a[k][j] += c * a[i][j];
        }
      }
    }

    for (int i = n - 1; i >= 0; i--) {
      x[i] = a[i][n] / a[i][i];
      for (int k = i - 1; k >= 0; k--)
        a[k][n] -= a[k][i] * x[i];
    }

    if (!solvable) {
      for (int i = n - 1; i >= 0; i--)
        x[i] = 0.;
    }

    for (int i = 0; i < n; i++)
      delete[] a[i];
    delete[] a;
  }

  void PronyFitter::SolveSLEGauss(std::complex<float>* x, std::complex<float>** r, std::complex<float>* b, int n)
  {
    bool solvable = true;
    int maxRow;
    float maxEl;
    std::complex<float> tmp, c;
    std::complex<float>** a = new std::complex<float>*[n];
    for (int i = 0; i < n; i++) {
      a[i] = new std::complex<float>[n + 1];
      for (int j = 0; j < n + 1; j++)
        a[i][j] = {0., 0.};
    }

    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++)
        a[i][j] = r[i][j];
      a[i][n] = b[i];
    }

    for (int i = 0; i < n; i++) {
      maxEl  = std::abs(a[i][i]);
      maxRow = i;
      for (int k = i + 1; k < n; k++)
        if (std::abs(a[k][i]) > maxEl) {
          maxEl  = std::abs(a[k][i]);
          maxRow = k;
        }

      if (maxEl == 0) {
        solvable = false;
        if (fIsDebug) printf("PsdPronyFitter:: SLE has not solution\n");
      }

      for (int k = i; k < n + 1; k++) {
        tmp          = a[maxRow][k];
        a[maxRow][k] = a[i][k];
        a[i][k]      = tmp;
      }

      for (int k = i + 1; k < n; k++) {
        c = -a[k][i] / a[i][i];
        for (int j = i; j < n + 1; j++) {
          if (i == j) a[k][j] = 0.;
          else
            a[k][j] += c * a[i][j];
        }
      }
    }

    for (int i = n - 1; i >= 0; i--) {
      x[i] = a[i][n] / a[i][i];
      for (int k = i - 1; k >= 0; k--)
        a[k][n] -= a[k][i] * x[i];
    }

    if (!solvable) {
      for (int i = n - 1; i >= 0; i--)
        x[i] = {0., 0.};
    }

    for (int i = 0; i < n; i++)
      delete[] a[i];
    delete[] a;
  }

  void PronyFitter::SolveSLECholesky(float* x, float** a, float* b, int n)
  {
    float temp;
    float** u = new float*[n];
    for (int i = 0; i < n; i++) {
      u[i] = new float[n];
      for (int j = 0; j < n; j++)
        u[i][j] = 0.;
    }

    float* y = new float[n];
    for (int i = 0; i < n; i++)
      y[i] = 0.;

    for (int i = 0; i < n; i++) {
      temp = 0.;
      for (int k = 0; k < i; k++)
        temp = temp + u[k][i] * u[k][i];
      u[i][i] = std::sqrt(a[i][i] - temp);
      for (int j = i; j < n; j++) {
        temp = 0.;
        for (int k = 0; k < i; k++)
          temp = temp + u[k][i] * u[k][j];
        u[i][j] = (a[i][j] - temp) / u[i][i];
      }
    }

    for (int i = 0; i < n; i++) {
      temp = 0.;
      for (int k = 0; k < i; k++)
        temp = temp + u[k][i] * y[k];
      y[i] = (b[i] - temp) / u[i][i];
    }

    for (int i = n - 1; i >= 0; i--) {
      temp = 0.;
      for (int k = i + 1; k < n; k++)
        temp = temp + u[i][k] * x[k];
      x[i] = (y[i] - temp) / u[i][i];
    }

    for (int i = 0; i < n; i++)
      delete[] u[i];
    delete[] u;
    delete[] y;
  }

  void PronyFitter::DeleteData()
  {
    delete[] fz;
    delete[] fh;
  }

  void PronyFitter::Clear()
  {
    fModelOrder    = 0;
    fExpNumber     = 0;
    fGateBeg       = 0;
    fGateEnd       = 0;
    fSampleTotal   = 0;
    fuZeroLevel    = 0.;
    fSignalBegin   = 0;
    fTotalPolRoots = 0;
    fuFitWfm.clear();
    DeleteData();
  }


}  // namespace PsdSignalFitting
