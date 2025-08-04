/* Copyright (C) 2020-2021 Institute for Nuclear Research, Moscow
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Nikolay Karpushkin [committer] */

/** @file   PronyFitter.h
    @class  PronyFitter
    @author Nikolay Karpushkin (nkarpushkin@mail.ru)
    @brief  Class to fit waveform using Prony least squares method
*/

#ifndef PronyFitter_H
#define PronyFitter_H

#include <algorithm>  // for reverse
#include <complex>    // for complex numbers
#include <cstring>    // for memcpy
#include <iostream>
#include <vector>  // for std::vector

#include <stdint.h>  // for uint16_t
#include <stdio.h>   // for printf

namespace PsdSignalFitting
{
  class PronyFitter {

  public:
    /**         Default constructor         **/
    PronyFitter() {};
    PronyFitter(int model_order, int exponent_number, int gate_beg, int gate_end);

    /**         Default destructor         **/
    ~PronyFitter() { Clear(); };

    int CalcSignalBegin(float front_time_beg_03, float front_time_end);
    int ChooseBestSignalBeginHarmonics(int first_sample, int last_sample);
    int ChooseBestSignalBegin(int first_sample, int last_sample);
    void MakePileUpRejection(int time_max);
    void CalculateFitHarmonics();
    void CalculateFitAmplitudes();
    void SolveSLEGauss(float* x, float** r, float* b, int n);
    void SolveSLEGauss(std::complex<float>* x, std::complex<float>** r, std::complex<float>* b, int n);
    void SolveSLECholesky(float* x, float** a, float* b, int n);
    void CovarianceQRmod(float& rho_f, std::vector<float>& a_f, float& rho_b, std::vector<float>& a_b);
    void CovarianceDirect(float& rho_f, std::vector<float>& a_f, float& rho_b, std::vector<float>& a_b);
    float LevelBy2Points(float X1, float Y1, float X2, float Y2, float Y0);
    //
    //                           Setters
    //
    void SetDebugMode(bool IsDebug) { fIsDebug = IsDebug; };
    void SetWaveform(std::vector<uint16_t>& uWfm, uint16_t uZeroLevel);
    void SetSignalBegin(int SignalBeg);
    void SetHarmonics(std::complex<float>* z);
    void SetExternalHarmonics(std::complex<float> z1, std::complex<float> z2);
    //
    //                           Getters
    //
    std::complex<float>* GetHarmonics();
    std::complex<float>* GetAmplitudes();
    float GetIntegral(int gate_beg, int gate_end);
    uint16_t GetFitValue(int sample_number);
    float GetFitValue(float x);
    float GetZeroLevel();
    float GetX(float level, int first_sample, int last_sample);
    float GetX(float level, int first_sample, int last_sample, float step);
    float GetRSquare(int gate_beg, int gate_end);
    float GetRSquareSignal();
    float GetChiSquare(int gate_beg, int gate_end, int time_max);
    float GetDeltaInSample(int sample);
    float GetSignalBeginFromPhase();
    float GetSignalMaxTime();
    float GetMaxAmplitude();
    int GetNumberPolRoots();
    std::vector<uint16_t> GetFitWfm() { return fuFitWfm; }

  private:
    void Initialize(int model_order, int exponent_number, int gate_beg, int gate_end);
    void AllocData();
    void DeleteData();
    void Clear();

    bool fIsDebug = false;
    int fModelOrder;
    int fExpNumber;
    int fGateBeg;
    int fGateEnd;
    int fSampleTotal;

    int fSignalBegin;
    int fTotalPolRoots;

    std::vector<uint16_t> fuWfm;
    uint16_t fuZeroLevel;
    std::complex<float>* fz;  //!
    std::complex<float>* fh;  //!
    std::vector<uint16_t> fuFitWfm;
    uint16_t fuFitZeroLevel;
  };
}  // namespace PsdSignalFitting

#endif
