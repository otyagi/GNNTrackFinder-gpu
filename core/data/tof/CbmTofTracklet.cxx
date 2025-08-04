/* Copyright (C) 2015-2020 PI-UHd, GSI
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Norbert Herrmann [committer], Florian Uhlig */

/** @file CbmTofTracklet.cxx
 ** @author nh
 ** @date 17.05.2015
 **
 **/
#include "CbmTofTracklet.h"

#include "CbmTofHit.h"  // for CbmTofHit

#include <FairTrackParam.h>  // for FairTrackParam
#include <Logger.h>          // for Logger, LOG

#include <TObject.h>  // for TObject
#include <TString.h>  // for Form

#include <string>  // for string


using std::vector;

CbmTofTracklet::CbmTofTracklet()
  : TObject()
  , fTrackLength(0.)
  , fPidHypo(-1)
  , fDistance(0.)
  , fTime(0.)
  , fTt(0.)
  , fT0(0.)
  , fT0Err(0.)
  , fTtErr(0.)
  , fT0TtCov(0.)
  , fChiSq(0.)
  , fNDF(0)
  , fTrackPar()
  , fParamFirst()
  , fParamLast()
  , fTofHit(0, -1)
  , fTofDet()
  , fMatChi()
  , fhit()
{
}

CbmTofTracklet::CbmTofTracklet(const CbmTofTracklet& t)
  : TObject(t)
  , fTrackLength(t.fTrackLength)
  , fPidHypo(t.fPidHypo)
  , fDistance(t.fDistance)
  , fTime(t.fTime)
  , fTt(t.fTt)
  , fT0(t.fT0)
  , fT0Err(t.fT0Err)
  , fTtErr(t.fTtErr)
  , fT0TtCov(t.fT0TtCov)
  , fChiSq(t.fChiSq)
  , fNDF(t.fNDF)
  , fTrackPar(CbmTofTrackletParam(t.fTrackPar))
  , fParamFirst(FairTrackParam(t.fParamFirst))
  , fParamLast(FairTrackParam(t.fParamLast))
  , fTofHit(t.fTofHit)
  , fTofDet(t.fTofDet)
  , fMatChi(t.fMatChi)
  , fhit(t.fhit)
{
}

CbmTofTracklet::~CbmTofTracklet() {}

/*
CbmTofTracklet::CbmTofTracklet(const CbmTofTracklet &fSource) :
   fTrackLength(0.),
   fPidHypo(-1),
   fDistance(0.),
   fTime(0.),
   fTt(0.),
   fT0(0.),
   fChiSq(0.),
   fNDF(0),
   fTrackPar(),
   fParamFirst(),
   fParamLast(),
   fTofHit(0,-1),
   fTofDet(),
   fMatChi(),
   fhit()
{
}

CbmTofTracklet& CbmTofTracklet::operator=(const CbmTofTracklet &fSource){
  // do something !
   return *this;
}
*/

void CbmTofTracklet::SetParamLast(const CbmTofTrackletParam* par)
{
  fParamLast.SetX(par->GetX());
  fParamLast.SetY(par->GetY());
  fParamLast.SetZ(par->GetZ());
  fParamLast.SetTx(par->GetTx());
  fParamLast.SetTy(par->GetTy());
  fParamLast.SetQp(par->GetQp());
  for (int i = 0, k = 0; i < 3; i++)
    for (int j = 0; j <= i; j++, k++)
      fParamLast.SetCovariance(i, j, par->GetCovariance(k));
}

void CbmTofTracklet::GetFairTrackParamLast()
{
  fTrackPar.SetX(fParamLast.GetX());
  fTrackPar.SetY(fParamLast.GetY());
  fTrackPar.SetZ(fParamLast.GetZ());
  fTrackPar.SetTx(fParamLast.GetTx());
  fTrackPar.SetTy(fParamLast.GetTy());
  fTrackPar.SetQp(fParamLast.GetQp());
  for (int i = 0, k = 0; i < 3; i++)
    for (int j = 0; j <= i; j++, k++)
      fTrackPar.SetCovariance(k, fParamLast.GetCovariance(i, j));
}

double CbmTofTracklet::GetMatChi2(int32_t iAddr)
{
  //LOG(debug1) << "array sizes: " << fTofHit.size() << ", " << fTofDet.size() << ", " << fMatChi.size();
  for (uint32_t iHit = 0; iHit < fTofHit.size(); iHit++) {
    //LOG(debug1) << Form(" -v- ind %d, tofhit %d, 0x%08x, chi %f ",iHit,fTofHit[iHit],fTofDet[iHit],fMatChi[iHit]);
    //if(0==fTofDet[iHit]) LOG(fatal) << " CbmTofTracklet::GetMatChi2 Invalid Detector Type! ";
    if (iAddr == fTofDet[iHit]) return fMatChi[iHit];
  }
  return -1.;
}

int32_t CbmTofTracklet::GetFirstInd(int32_t iAddr)
{
  for (uint32_t iHit = 0; iHit < fTofHit.size(); iHit++) {
    //    LOG(info) << "     GFI "<< iSm <<", "<<iHit<<", "<< fTofDet[iHit];
    if (iAddr != fTofDet[iHit]) return fTofHit[iHit];
  }
  LOG(fatal) << " CbmTofTracklet::GetFirstInd, did only find " << iAddr;
  return -1;
}

double CbmTofTracklet::GetZ0x()
{
  double dZ0 = 0.;
  if (fTrackPar.GetTx() != 0.) dZ0 = -fTrackPar.GetX() / fTrackPar.GetTx() + fTrackPar.GetZ();
  return dZ0;
}

double CbmTofTracklet::GetZ0y()
{
  double dZ0 = 0.;
  if (fTrackPar.GetTy() != 0.) dZ0 = -fTrackPar.GetY() / fTrackPar.GetTy() + fTrackPar.GetZ();
  return dZ0;
}

double CbmTofTracklet::GetR0()
{
  double dR0 = sqrt(pow(GetFitX(0.), 2) + pow(GetFitY(0.), 2));
  return dR0;
}

double CbmTofTracklet::GetTex(CbmTofHit* pHit)
{
  /*
  double dR2=0.;
  dR2 += pow(fTrackPar.GetX()-pHit->GetX(),2);
  dR2 += pow(fTrackPar.GetY()-pHit->GetY(),2);
  dR2 += pow(fTrackPar.GetZ()-pHit->GetZ(),2);
  double dR = sqrt(dR2);
  */
  /*
  double dR = pHit->GetR();
  LOG(debug) <<Form(" CbmTofTracklet::GetTex Bmon %7.1f dR %7.1f, Tt %7.4f => Tex %7.3f, ",
		    fT0,dR,fTt,fT0 + dR*fTt)
	     <<fTrackPar.ToString()
	     ;
  return   fT0 + dR*fTt;
  */
  double dZ    = pHit->GetZ();
  double dSign = 1.;
  if (pHit->GetZ() < fhit[0].GetZ()) dSign = -1;
  double dTex = fhit[0].GetTime() + fTt * dSign * Dist3D(pHit, &fhit[0]);
  LOG(debug) << Form("GetTex Bmon %7.3f, Z %7.1f, DZ %5.1f, Sign %2.0f, Tt %7.4f => Tex %7.3f, ", fhit[0].GetTime(), dZ,
                     dZ - fhit[0].GetZ(), dSign, fTt, dTex)
             << fTrackPar.ToString();
  return dTex;
}

double CbmTofTracklet::UpdateT0()
{  //returns estimated time at R=0
   //  double dBmon=0.;
  int32_t nValidHits = 0;
  int32_t iHit1;
  double dDist1;

  double aR[fTofHit.size()];
  double at[fTofHit.size()];
  double ae[fTofHit.size()];
  for (uint32_t iHit = 0; iHit < fTofHit.size(); iHit++) {
    if (fTofDet[iHit] >= 0) {  // exclude faked hits
      if (nValidHits == 0) {
        iHit1 = iHit;
        //dDist1=fhit[iHit1].GetR();
        dDist1 =
          fhit[iHit1].GetZ() * sqrt(1. + fTrackPar.GetTx() * fTrackPar.GetTx() + fTrackPar.GetTy() * fTrackPar.GetTy());
      }
      //aR[nValidHits]=fhit[iHit].GetR();
      double dSign = 1.;
      if (fhit[iHit].GetZ() < fhit[iHit1].GetZ()) dSign = -1.;
      aR[nValidHits] = dDist1 + dSign * Dist3D(&fhit[iHit], &fhit[iHit1]);
      at[nValidHits] = fhit[iHit].GetTime();
      ae[nValidHits] = 0.1;  // const timing error, FIXME (?)
      //ae[nValidHits]=fhit[iHit].GetTimeError();
      nValidHits++;
    }
  }

  /*
  //
  // follow tutorial solveLinear.C to solve the linear equation t=t0+tt*R
  //
  TVectorD R; R.Use(nValidHits,aR);
  TVectorD t; t.Use(nValidHits,at);
  TVectorD e; e.Use(nValidHits,ae);

  const int32_t nrVar  = 2;
  TMatrixD A(nValidHits,nrVar);
  TMatrixDColumn(A,0) = 1.0;
  TMatrixDColumn(A,1) = R;

  // first bring the weights in place
  TMatrixD Aw = A;
  TVectorD yw = t;
  for (int32_t irow = 0; irow < A.GetNrows(); irow++) {
    TMatrixDRow(Aw,irow) *= 1/e(irow);
    yw(irow) /= e(irow);
  }

  TDecompSVD svd(Aw);
  bool ok;
  const TVectorD c_svd = svd.Solve(yw,ok);

  // c_svd.Print();

  fT0=c_svd[0];
  fTt=c_svd[1];
  */

  //
  // Using analyctical Solution of Chi2-Fit to solve the linear equation t=t0+tt*R
  // Converted into Matrix Form, Matrices calcualted and only resulting formula are implemented
  // J.Brandt
  //
  double RRsum      = 0;           //  Values will follow this procedure:
  double Rsum       = 0;           //  $Rsum=\sum_{i}^{nValidHits}\frac{R_i}{e_i^2}$
  double tsum       = 0;           //  where e_i will always be the error on the t measurement
  double esum       = 0;           //  RR=R^2 in numerator, e denotes 1 in numerator , Rt= R*t in numerator
  double Rtsum      = 0;           //
  double sig_weight = 0;           //  ae^2
  double yoffset    = at[0] - 10;  //  Bmon time offset to scale times to ns regime and not 10^10ns
  for (int32_t i = 0; i < nValidHits; i++) {
    at[i] -= yoffset;  //  substract offset
    sig_weight = (ae[i] * ae[i]);
    Rsum += (aR[i] / sig_weight);
    tsum += (at[i] / sig_weight);
    RRsum += (aR[i] * aR[i] / sig_weight);
    Rtsum += (aR[i] * at[i] / sig_weight);
    esum += (1 / sig_weight);
  }
  double det_cov_mat =
    esum * RRsum
    - Rsum * Rsum;  // Determinant of inverted Covariance Matrix -> 1/det is common Faktor of Cavariance Matrix
  fT0      = (RRsum * tsum - Rsum * Rtsum) / det_cov_mat;  // Best Guess for time at origin
  fTt      = (-Rsum * tsum + esum * Rtsum) / det_cov_mat;  // Best guess for inverted velocity
  fT0Err   = sqrt(RRsum / det_cov_mat);                    // sqrt of (0,0) in Covariance matrix -> error on fT0
  fTtErr   = sqrt(esum / det_cov_mat);                     // sqrt of (1,1) in Covariance Matrix -> error on fTt
  fT0TtCov = -Rsum / det_cov_mat;                          // (0,1)=(1,0) in Covariance Matrix -> cov(fT0,fTt)

  fT0 += yoffset;  // Adding yoffset again

  //if (iHit0>-1) fhit[iHit0].SetTime(fT0);

  LOG(debug) << Form("Trkl size %u,  validHits %d, Tt = %6.4f TtErr = %2.4f Bmon "
                     "= %6.2f BmonErr = %2.2f BmonTtCov = %6.4f",
                     (uint32_t) fTofHit.size(), nValidHits, fTt, fTtErr, fT0, fT0Err, fT0TtCov);

  return fT0;
}

double CbmTofTracklet::UpdateTt()
{
  double dTt  = 0.;
  int32_t iNt = 0;
  for (uint32_t iHL = 0; iHL < fhit.size() - 1; iHL++) {
    //if( fTofDet[iHL]>0 )                         // exclude faked hits
    for (uint32_t iHH = iHL + 1; iHH < fhit.size(); iHH++) {
      //if( fTofDet[iHH]>0)   // exclude faked hits
      {
        dTt += (fhit[iHH].GetTime() - fhit[iHL].GetTime())
               / (fhit[iHH].GetR() - fhit[iHL].GetR());  // only valid for tracks from nominal vertex (0,0,0)
        iNt++;
      }
    }
  }
  if (iNt == 0) {
    LOG(warning) << "No valid hit pair ";
    return fTt;
  }
  fTt = dTt / (double) iNt;
  return fTt;
}

double CbmTofTracklet::GetXdif(int32_t iDetId, CbmTofHit* pHit)
{
  double dXref  = 0.;
  int32_t iNref = 0;
  double dTx    = 0;

  if (1) {
    for (uint32_t iHL = 0; iHL < fhit.size() - 1; iHL++) {
      if (iDetId == fTofDet[iHL] || 0 == fTofDet[iHL]) continue;  // exclude faked hits
      for (uint32_t iHH = iHL + 1; iHH < fhit.size(); iHH++) {
        if (iDetId == fTofDet[iHH] || 0 == fTofDet[iHH]) continue;  // exclude faked hits
        //dTt+=(fhit[iHH].GetTime()-fhit[iHL].GetTime())/(fhit[iHH].GetR()-fhit[iHL].GetR()); // for projective geometries only !!!
        dTx += (fhit[iHH].GetX() - fhit[iHL].GetX()) / (fhit[iHH].GetZ() - fhit[iHL].GetZ());
        iNref++;
      }
    }
    dTx /= iNref;
  }
  else {
    dTx = fTrackPar.GetTx();
  }

  iNref = 0;
  for (uint32_t iHit = 0; iHit < fTofHit.size(); iHit++) {
    if (iDetId == fTofDet[iHit] || 0 == fTofDet[iHit]) continue;

    double dDZ = pHit->GetZ() - fhit[iHit].GetZ();
    dXref += fhit[iHit].GetX() + dTx * dDZ;
    iNref++;
  }

  if (iNref == 0) {
    LOG(error) << "DetId " << iDetId << ", Nref " << iNref << " sizes " << fTofHit.size() << ", " << fhit.size();
    return 1.E20;
  }

  dXref /= iNref;

  return pHit->GetX() - dXref;
}

double CbmTofTracklet::GetYdif(int32_t iDetId, CbmTofHit* pHit)
{
  double dYref  = 0.;
  int32_t iNref = 0;
  double dTy    = 0;

  if (1) {
    for (uint32_t iHL = 0; iHL < fhit.size() - 1; iHL++) {
      if (iDetId == fTofDet[iHL] || 0 == fTofDet[iHL]) continue;  // exclude faked hits
      for (uint32_t iHH = iHL + 1; iHH < fhit.size(); iHH++) {
        if (iDetId == fTofDet[iHH] || 0 == fTofDet[iHH]) continue;  // exclude faked hits
        dTy += (fhit[iHH].GetY() - fhit[iHL].GetY()) / (fhit[iHH].GetZ() - fhit[iHL].GetZ());
        iNref++;
      }
    }
    dTy /= iNref;
  }
  else {
    dTy = fTrackPar.GetTy();
  }

  iNref = 0;
  for (uint32_t iHit = 0; iHit < fTofHit.size(); iHit++) {
    if (iDetId == fTofDet[iHit] || 0 == fTofDet[iHit]) continue;

    double dDZ = pHit->GetZ() - fhit[iHit].GetZ();
    dYref += fhit[iHit].GetY() + dTy * dDZ;
    iNref++;
  }

  if (iNref == 0) {
    LOG(error) << "DetId " << iDetId << ", Nref " << iNref << " sizes " << fTofHit.size() << ", " << fhit.size();
    return 1.E20;
  }

  dYref /= iNref;

  return pHit->GetY() - dYref;
}

double CbmTofTracklet::GetTdif(int32_t iDetId, CbmTofHit* pHit)
{
  double dTref = 0.;
  double Nref  = 0;
  double dTt   = 0.;
  int32_t iNt  = 0;

  if (1) {
    for (uint32_t iHL = 0; iHL < fhit.size() - 1; iHL++) {
      if (iDetId == fTofDet[iHL] || 0 == fTofDet[iHL]) continue;  // exclude faked hits
      for (uint32_t iHH = iHL + 1; iHH < fhit.size(); iHH++) {
        if (iDetId == fTofDet[iHH] || 0 == fTofDet[iHH]) continue;  // exclude faked hits
        //dTt+=(fhit[iHH].GetTime()-fhit[iHL].GetTime())/(fhit[iHH].GetR()-fhit[iHL].GetR()); // for projective geometries only !!!
        double dDist =
          sqrt(pow((fhit[iHH].GetX() - fhit[iHL].GetX()), 2) + pow((fhit[iHH].GetY() - fhit[iHL].GetY()), 2)
               + pow((fhit[iHH].GetZ() - fhit[iHL].GetZ()), 2));
        double dSign = 1.;
        if (fhit[iHH].GetZ() < fhit[iHL].GetZ()) dSign = -1.;
        dTt += (fhit[iHH].GetTime() - fhit[iHL].GetTime()) / dDist * dSign;
        iNt++;
      }
    }

    if (iNt == 0) {
      LOG(error) << "No valid hit pair ";
      return 1.E20;
    }
    dTt /= (double) iNt;
  }
  else {
    dTt = fTt;
  }

  for (uint32_t iHit = 0; iHit < fTofHit.size(); iHit++) {
    if (iDetId == fTofDet[iHit] || 0 == fTofDet[iHit]) continue;
    //dTref += fhit[iHit].GetTime() - dTt*(fhit[iHit].GetR()-pHit->GetR());
    double dSign = 1.;
    if (fhit[iHit].GetZ() < pHit->GetZ()) dSign = -1;
    dTref += fhit[iHit].GetTime() - dTt * dSign * Dist3D(&fhit[iHit], pHit);
    Nref++;
  }
  if (Nref == 0) {
    LOG(error) << "DetId " << iDetId << ", Nref " << Nref << " sizes " << fTofHit.size() << ", " << fhit.size();
    return 1.E20;
  }
  dTref /= (double) Nref;
  double dTdif = pHit->GetTime() - dTref;
  // LOG(debug) << "iSt "<< iSt<<" DetId "<<iDetId<<", Nref "<<Nref<<" Tdif
  // "<<dTdif;
  return dTdif;
}

bool CbmTofTracklet::ContainsAddr(int32_t iAddr)
{
  for (uint32_t iHit = 0; iHit < fTofHit.size(); iHit++) {
    int32_t iHaddr = fhit[iHit].GetAddress() & 0x003FFFFF;
    /*
    LOG(debug)<<Form(" Contain test hit %d for 0x%08x, 0x%08x = 0x%08x ?",
		     iHit,fhit[iHit].GetAddress(),iHaddr,iAddr)
	      ;
    */
    if (iHaddr == iAddr) return true;
  }
  return false;
}

int32_t CbmTofTracklet::HitIndexOfAddr(int32_t iAddr)
{
  for (uint32_t iHit = 0; iHit < fTofHit.size(); iHit++) {
    if ((fhit[iHit].GetAddress() & 0x003FFFFF) == iAddr) return iHit;
  }
  return -1;
}

CbmTofHit* CbmTofTracklet::HitPointerOfAddr(int32_t iAddr)
{
  for (uint32_t iHit = 0; iHit < fTofHit.size(); iHit++) {
    if ((fhit[iHit].GetAddress() & 0x003FFFFF) == iAddr) return &fhit[iHit];
  }
  return nullptr;
}

const double* CbmTofTracklet::GetPoint(int32_t n)
{  // interface to event display: CbmTracks
  fP[0] = fhit[n].GetX();
  fP[1] = fhit[n].GetY();
  fP[2] = fhit[n].GetZ();
  fP[3] = fhit[n].GetTime();
  //  LOG(info) <<Form("CbmTofTracklet::GetPoint %d, %6.2f, %6.2f, %6.2f, %6.2f ",n,fP[0],fP[1],fP[2],fP[3]);
  return fP;
}

const double* CbmTofTracklet::GetFitPoint(int32_t n)
{  // interface to event display: CbmTracks
  fP[0] = GetFitX(fhit[n].GetZ());
  fP[1] = GetFitY(fhit[n].GetZ());
  fP[2] = fhit[n].GetZ();
  fP[3] = fhit[n].GetTime();
  /*
  LOG(info) <<Form("CbmTofTracklet::GetFitPoint %d, %6.2f - %6.2f, %6.2f - %6.2f, %6.2f, %6.2f ",
	      n,fP[0],fhit[n]->GetX(),fP[1],fhit[n]->GetY(),fP[2],fP[3]);
  */
  return fP;
}

double CbmTofTracklet::GetFitX(double dZ) { return fTrackPar.GetX() + fTrackPar.GetTx() * (dZ - fTrackPar.GetZ()); }

double CbmTofTracklet::GetFitY(double dZ) { return fTrackPar.GetY() + fTrackPar.GetTy() * (dZ - fTrackPar.GetZ()); }

double CbmTofTracklet::GetFitT(double dZ)
{
  //  return   fT0 + dR*fTt;
  //return fT0 + fTt*dZ*sqrt(fTrackPar.GetTx()*fTrackPar.GetTx()+fTrackPar.GetTy()*fTrackPar.GetTy());
  return fT0
         + fTt * (dZ - fTrackPar.GetZ())
             * sqrt(1. + fTrackPar.GetTx() * fTrackPar.GetTx() + fTrackPar.GetTy() * fTrackPar.GetTy());
}

void CbmTofTracklet::Clear(Option_t* /*option*/)
{
  //  LOG(debug) << "Clear TofTracklet with option "<<*option;
  fTofHit.clear();
  fTofDet.clear();
  fMatChi.clear();
  fhit.clear();
}

void CbmTofTracklet::PrintInfo()
{
  LOG(info) << Form("TrklInfo: Nhits %d, Tt %6.3f, stations: ", GetNofHits(), GetTt());
  LOG(info);
  for (int32_t iH = 0; iH < GetNofHits(); iH++) {
    LOG(info) << Form("  Hit %2d: Ind %5d, det 0x%08x, addr 0x%08x, chi %6.3f, ae[]= %6.4f", iH, fTofHit[iH],
                      fTofDet[iH], fhit[iH].GetAddress(), fMatChi[iH], fhit[iH].GetTimeError());
  }
}

double CbmTofTracklet::Dist3D(CbmTofHit* pHit0, CbmTofHit* pHit1)
{
  double dDist = sqrt(pow((pHit0->GetX() - pHit1->GetX()), 2) + pow((pHit0->GetY() - pHit1->GetY()), 2)
                      + pow((pHit0->GetZ() - pHit1->GetZ()), 2));
  return dDist;
}

double CbmTofTracklet::GetRefVel(uint32_t iNhit)
{
  double dVel = 0.;
  double dTt  = 0.;
  int32_t iNt = 0;
  if (fhit.size() >= iNhit) {
    for (uint32_t iHL = 0; iHL < iNhit - 1; iHL++) {
      for (uint32_t iHH = iHL + 1; iHH < iNhit; iHH++) {
        dTt += (fhit[iHH].GetTime() - fhit[iHL].GetTime()) / (fhit[iHH].GetR() - fhit[iHL].GetR());
        iNt++;
      }
    }
  }
  if (iNt == 0) { LOG(debug) << " CbmTofTracklet::GetRefVel: No valid hit pair "; }
  else {
    dVel = (double) iNt / dTt;
  }
  return dVel;
}

ClassImp(CbmTofTracklet)
