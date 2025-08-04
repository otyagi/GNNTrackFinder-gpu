/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer] */

#ifndef CaUvConverter_h
#define CaUvConverter_h 1


#include "CaDefs.h"

namespace cbm::ca
{
  /// @brief A class to convert XY coordinates to UV coordinates

  class CaUvConverter {

   public:
    /// @brief construct from U,V angles
    CaUvConverter(double phiU, double phiV) { SetFromUV(phiU, phiV); }

    /// @brief construct from U angle and XY covariance matrix
    CaUvConverter(double phiU, double dx2, double dxy, double dy2) { SetFromXYCovMatrix(phiU, dx2, dxy, dy2); }

    /// @brief construct from U,V angles
    void SetFromUV(double phiU, double phiV);

    /// @brief construct from U angle and XY covariance matrix
    void SetFromXYCovMatrix(double phiU, double dx2, double dxy, double dy2);

    /// @brief Conversion function (x,y) -> (u,v)
    /// @param  x X-coordinate
    /// @param  y Y-coordinate
    /// @return pair [u, v] coordinates
    std::pair<double, double> ConvertXYtoUV(double x, double y) const
    {
      return std::make_pair(fcosU * x + fsinU * y, fcosV * x + fsinV * y);
    }

    /// @brief Conversion function (x,y) -> (u,v)
    /// @param  u U-coordinate
    /// @param  v V-coordinate
    /// @return pair [x, y] coordinates
    std::pair<double, double> ConvertUVtoXY(double u, double v) const
    {
      return std::make_pair(fcosX * u + fsinX * v, fcosY * u + fsinY * v);
    }

    /// @brief Conversion function (du2, duv, dv2) -> (dx2, dxy, dy2)
    /// @param du2  Variance of U-coordinate measurement
    /// @param duv  Covariance of U & V - coordinate measurement
    /// @param dv2  Variance of V-coordinate measurement
    /// @return tuple [dx2, dxy, dy2]
    std::tuple<double, double, double> ConvertCovMatrixUVtoXY(double du2, double duv, double dv2) const
    {
      return std::make_tuple(fcosX * fcosX * du2 + 2. * fsinX * fcosX * duv + fsinX * fsinX * dv2,
                             fcosX * fcosY * du2 + (fcosX * fsinY + fsinX * fcosY) * duv + fsinX * fsinY * dv2,
                             fcosY * fcosY * du2 + 2. * fsinY * fcosY * duv + fsinY * fsinY * dv2);
    }

    /// @brief Conversion function (dx2, dxy, dy2) -> (du2, duv, dv2)
    /// @param dx2  Variance of X-coordinate measurement
    /// @param dxy  Covariance of X & Y - coordinate measurement
    /// @param dy2  Variance of Y-coordinate measurement
    /// @return tuple [du2, duv, dv2]
    std::tuple<double, double, double> ConvertCovMatrixXYtoUV(double dx2, double dxy, double dy2) const
    {
      return std::make_tuple(fcosU * fcosU * dx2 + 2. * fsinU * fcosU * dxy + fsinU * fsinU * dy2,
                             fcosU * fcosV * dx2 + (fcosU * fsinV + fsinU * fcosV) * dxy + fsinU * fsinV * dy2,
                             fcosV * fcosV * dx2 + 2. * fsinV * fcosV * dxy + fsinV * fsinV * dy2);
    }


   private:
    double fcosU{cbm::algo::ca::constants::Undef<double>};  ///< U coordinate in XY
    double fsinU{cbm::algo::ca::constants::Undef<double>};

    double fcosV{cbm::algo::ca::constants::Undef<double>};  ///< V coordinate in XY
    double fsinV{cbm::algo::ca::constants::Undef<double>};

    double fcosX{cbm::algo::ca::constants::Undef<double>};  ///< X coordinate in UV
    double fsinX{cbm::algo::ca::constants::Undef<double>};

    double fcosY{cbm::algo::ca::constants::Undef<double>};  ///< Y coordinate in UV
    double fsinY{cbm::algo::ca::constants::Undef<double>};
  };

}  // namespace cbm::ca


#endif
