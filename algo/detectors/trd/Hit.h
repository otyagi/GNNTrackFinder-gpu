/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Matus Kalisky, Florian Uhlig, Andrey Lebedev */

/**
 * \file Hit.h
 * \author Dominik Smith <d.smith@gsi.de>
 * \date 2024
 * \brief A light-weight TRD hit class for online reconstruction, based on CbmTrdHit. .
 **/

#pragma once

#include "CbmTrdAddress.h"  // for CbmTrdAddress
#include "Math/Rotation3D.h"
#include "Math/Vector3Dfwd.h"

#include <Rtypes.h>      // for CLRBIT, SETBIT, TESTBIT, ClassDef
#include <RtypesCore.h>  // for Double32_t

#include <boost/serialization/access.hpp>

#include <cstdint>
#include <string>  // for string

namespace cbm::algo::trd
{

  /** \class Hit
 * \brief A light-weight TRD hit class for online reconstruction, based on CbmTrdHit. .
 */

  class Hit {
   public:
    enum HitDef
    {
      kType = 0  ///< set type of pad layout
        ,
      kMaxType  ///< set type of pad on which the maximum charge is found
        ,
      kRowCross  ///< mark hit defined by 2 clusters
        ,
      kOvfl  ///< mark over-flow in the data
    };


    /**
	 * \brief Default constructor.
	 */
    Hit();

    /**
	 * \brief Standard constructor.
	  *\param address Unique detector ID
	  *\param pos Position in global c.s. [cm]
         *\param dpos Errors of position in global c.s. [cm]
	  *\param dxy XY correlation of the hit
	  *\param refId Index of digi or cluster
	  *\param eLoss TR + dEdx
	  **/
    Hit(int32_t address, const ROOT::Math::XYZVector& pos, const ROOT::Math::XYZVector& dpos, double dxy, int32_t refId,
        double eLoss, double time = 0., double timeError = 0.);

    /**
	 * \brief Standard constructor.
	 * \param[in] address Detector unique identifier.
	 * \param[in] x X position of the hit [cm].
	 * \param[in] y Y position of the hit [cm].
	 * \param[in] z Z position of the hit [cm].
	 * \param[in] dx X position error of the hit [cm].
	 * \param[in] dy Y position error of the hit [cm].
	 * \param[in] dz Z position error of the hit [cm].
	 * \param[in] dxy X-Y covariance of the hit [cm**2].
	 * \param[in] refId Some reference ID.
         * \param[in] time Hit time [ns].
         * \param[in] timeError Error of hit time [ns].
 	 **/
    Hit(int32_t address, double x, double y, double z, double dx, double dy, double dz, double dxy, int32_t refId,
        double time = -1., double timeError = -1.);


    /** \brief Destructor. */
    ~Hit(){};

    /* Accessors */
    int32_t GetPlaneId() const { return CbmTrdAddress::GetLayerId(Address()); }
    double GetELoss() const { return fELoss; }
    bool GetClassType() const { return TESTBIT(fDefine, kType); }
    bool GetMaxType() const { return TESTBIT(fDefine, kMaxType); }
    bool HasOverFlow() const { return TESTBIT(fDefine, kOvfl); }
    bool IsRowCross() const { return TESTBIT(fDefine, kRowCross); }
    bool IsUsed() const { return (GetRefId() < 0); }
    double X() const { return fX; }
    double Y() const { return fY; }
    double Z() const { return fZ; }
    double Dx() const { return fDx; }
    double Dy() const { return fDy; }
    double Dz() const { return fDz; }
    double Dxy() const { return fDxy; }
    int32_t GetRefId() const { return fRefId; }
    int32_t Address() const { return fAddress; }
    double Time() const { return fTime; }
    double TimeError() const { return fTimeError; }

    /**
	 * \brief Copies hit position to pos.
	 * \param pos Output hit position.
	 */
    void Position(ROOT::Math::XYZVector& pos) const;

    /**
	 * \brief Copies hit position error to pos.
	 * \param pos Output hit position error.
	 */
    void PositionError(ROOT::Math::XYZVector& dpos) const;

    /* Setters */
    void SetELoss(double loss) { fELoss = loss; }
    /** \brief Mark overflow in one or more digits which define the hit.*/
    void SetOverFlow(bool set = true) { set ? SETBIT(fDefine, kOvfl) : CLRBIT(fDefine, kOvfl); }
    /** \brief Mark hit reconstructed between pad rows.*/
    void SetRowCross(bool set = true) { set ? SETBIT(fDefine, kRowCross) : CLRBIT(fDefine, kRowCross); }
    /** \brief Type of pad layout used in reconstruction R[0], T[1]*/
    void SetClassType(bool set = true) { set ? SETBIT(fDefine, kType) : CLRBIT(fDefine, kType); }
    /** \brief Extra bool definition for the hit (e.g. the type of maximum for triangular pads).*/
    void SetMaxType(bool set = true) { set ? SETBIT(fDefine, kMaxType) : CLRBIT(fDefine, kMaxType); }
    void SetX(double x) { fX = x; }
    void SetY(double y) { fY = y; }
    void SetZ(double z) { fZ = z; }
    void SetDx(double dx) { fDx = dx; }
    void SetDy(double dy) { fDy = dy; }
    void SetDz(double dz) { fDz = dz; }
    void SetDxy(double dxy) { fDxy = dxy; }
    void SetRefId(int32_t refId) { fRefId = refId; }
    void SetAddress(int32_t address) { fAddress = address; }
    void SetTime(double time) { fTime = time; }
    void SetTime(double time, double error)
    {
      fTime      = time;
      fTimeError = error;
    }
    void SetTimeError(double error) { fTimeError = error; }

    /**
	 * \brief Sets position of the hit.
	 * \param pos new hit position.
	 **/
    void SetPosition(const ROOT::Math::XYZVector& pos);

    /**
	 * \breif Sets position error of the hit.
	 * \param dpos new hit position error
	 **/
    void SetPositionError(const ROOT::Math::XYZVector& dpos);


   private:
    double fX, fY, fZ;     ///< X, Y, Z positions of hit [cm]
    double fDx, fDy, fDz;  ///< X, Y, Z errors [cm]
    double fDxy;           ///< X-Y covariance of the hit [cm**2]
    int32_t fRefId;        ///< some reference id (usually to cluster, digi or MC point)
    int32_t fAddress;      ///< detector unique identifier
    double fTime;          ///< Hit time [ns]
    double fTimeError;     ///< Error of hit time [ns]

    uint8_t fDefine;      // hit extra info
    int32_t fNeighborId;  // refId in case of row cross clusters
    Double32_t fELoss;    // Energy deposit due to TR + dEdx

   private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, unsigned int /*version*/)
    {
      ar& fX;
      ar& fY;
      ar& fZ;
      ar& fDx;
      ar& fDy;
      ar& fDz;
      ar& fDxy;
      ar& fRefId;
      ar& fAddress;
      ar& fTime;
      ar& fTimeError;
      ar& fDefine;
      ar& fNeighborId;
      ar& fELoss;
    }
  };


}  // namespace cbm::algo::trd
