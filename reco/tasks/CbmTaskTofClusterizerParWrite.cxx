/* Copyright (C) 2022 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Pierre-Alain Loizeau, Norbert Herrmann */

#include "CbmTaskTofClusterizerParWrite.h"

// TOF Classes and includes
#include "CbmTofAddress.h"          // in cbmdata/tof
#include "CbmTofCell.h"             // in tof/TofData
#include "CbmTofCreateDigiPar.h"    // in tof/TofTools
#include "CbmTofDetectorId_v12b.h"  // in cbmdata/tof
#include "CbmTofDetectorId_v14a.h"  // in cbmdata/tof
#include "CbmTofDetectorId_v21a.h"  // in cbmdata/tof
#include "CbmTofDigiBdfPar.h"       // in tof/TofParam
#include "CbmTofDigiPar.h"          // in tof/TofParam
#include "CbmTofGeoHandler.h"       // in tof/TofTools

// FAIR classes and includes
#include "FairRootFileSink.h"
#include "FairRootManager.h"
#include "FairRunAna.h"
#include "FairRuntimeDb.h"
#include "tof/CalibrateSetup.h"
#include "tof/HitfindSetup.h"
#include "yaml/Yaml.h"

#include <Logger.h>

// ROOT Classes and includes
#include "TClonesArray.h"
#include "TGeoManager.h"
#include "TGeoPhysicalNode.h"
#include "TH2.h"
#include "TProfile.h"
#include "TROOT.h"
#include "TStopwatch.h"
#include "TVector3.h"

// C++ Classes and includes
#include <iomanip>
#include <vector>


/************************************************************************************/
CbmTaskTofClusterizerParWrite::CbmTaskTofClusterizerParWrite()
  : CbmTaskTofClusterizerParWrite("TestbeamClusterizer", 0, 0)
{
}

CbmTaskTofClusterizerParWrite::CbmTaskTofClusterizerParWrite(const char* name, int32_t verbose, bool /*writeDataInOut*/)
  : FairTask(TString(name), verbose)
  , fTofId(NULL)
  , fDigiPar(NULL)
  , fDigiBdfPar(NULL)
  , fvCPTOff()
  , fvCPTotGain()
  , fvCPTotOff()
  , fvCPWalk()
  , fvCPTOffY()
  , fvCPTOffYBinWidth()
  , fvCPTOffYRange()
  , fvDeadStrips()
  , fCalMode(0)
  , fPosYMaxScal(1.)
  , fTotMax(100.)
  , fTotMin(0.)
  , fTotOff(0.)
  , fTotMean(0.)
  , fMaxTimeDist(1.)
  , fdChannelDeadtime(0.)
  , fCalParFileName("")
  , fdTOTMax(50.)
  , fdTOTMin(0.)
  , fdTTotMean(2.)
  , fdMaxTimeDist(0.)
  , fdMaxSpaceDist(0.)
  , fdModifySigvel(1.0)
  , fbSwapChannelSides(false)
{
}

CbmTaskTofClusterizerParWrite::~CbmTaskTofClusterizerParWrite() {}

/************************************************************************************/
// FairTasks inherited functions
InitStatus CbmTaskTofClusterizerParWrite::Init()
{
  LOG(info) << "CbmTaskTofClusterizerParWrite initializing... expect Digis in ns units! ";
  if (false == InitParameters()) return kFATAL;
  if (false == InitCalibParameter()) return kFATAL;
  if (false == InitAlgos()) return kFATAL;
  return kSUCCESS;
}

void CbmTaskTofClusterizerParWrite::SetParContainers()
{
  LOG(info) << "=> Get the digi parameters for tof";
  FairRunAna* ana     = FairRunAna::Instance();
  FairRuntimeDb* rtdb = ana->GetRuntimeDb();

  fDigiPar = (CbmTofDigiPar*) (rtdb->getContainer("CbmTofDigiPar"));

  LOG(info) << "found " << fDigiPar->GetNrOfModules() << " cells ";
  fDigiBdfPar = (CbmTofDigiBdfPar*) (rtdb->getContainer("CbmTofDigiBdfPar"));
}


/************************************************************************************/
bool CbmTaskTofClusterizerParWrite::InitParameters()
{

  // Initialize the TOF GeoHandler
  bool isSimulation = false;
  LOG(info) << "CbmTaskTofClusterizerParWrite::InitParameters - Geometry, Mapping, ...  ??";

  // Get Base Container
  FairRun* ana        = FairRun::Instance();
  FairRuntimeDb* rtdb = ana->GetRuntimeDb();

  CbmTofGeoHandler geoHandler;
  int32_t iGeoVersion = geoHandler.Init(isSimulation);
  if (k14a > iGeoVersion) {
    LOG(error) << "CbmTaskTofClusterizerParWrite::InitParameters => Only compatible "
                  "with geometries after v14a !!!";
    return false;
  }
  if (iGeoVersion == k14a)
    fTofId = new CbmTofDetectorId_v14a();
  else
    fTofId = new CbmTofDetectorId_v21a();

  // create digitization parameters from geometry file
  CbmTofCreateDigiPar* tofDigiPar = new CbmTofCreateDigiPar("TOF Digi Producer", "TOF task");
  LOG(info) << "Create DigiPar ";
  tofDigiPar->Init();

  fDigiPar = (CbmTofDigiPar*) (rtdb->getContainer("CbmTofDigiPar"));
  if (0 == fDigiPar) {
    LOG(error) << "CbmTaskTofClusterizerParWrite::InitParameters => Could not obtain "
                  "the CbmTofDigiPar ";
    return false;
  }

  fDigiBdfPar = (CbmTofDigiBdfPar*) (rtdb->getContainer("CbmTofDigiBdfPar"));
  if (0 == fDigiBdfPar) {
    LOG(error) << "CbmTaskTofClusterizerParWrite::InitParameters => Could not obtain "
                  "the CbmTofDigiBdfPar ";
    return false;
  }

  rtdb->initContainers(ana->GetRunId());

  LOG(info) << "CbmTaskTofClusterizerParWrite::InitParameter: currently " << fDigiPar->GetNrOfModules()
            << " digi cells ";

  fdMaxTimeDist  = fDigiBdfPar->GetMaxTimeDist();     // in ns
  fdMaxSpaceDist = fDigiBdfPar->GetMaxDistAlongCh();  // in cm

  if (fMaxTimeDist != fdMaxTimeDist) {
    fdMaxTimeDist  = fMaxTimeDist;  // modify default
    fdMaxSpaceDist = fdMaxTimeDist * fDigiBdfPar->GetSignalSpeed()
                     * 0.5;  // cut consistently on positions (with default signal velocity)
  }

  LOG(info) << " BuildCluster with MaxTimeDist " << fdMaxTimeDist << ", MaxSpaceDist " << fdMaxSpaceDist;

  fvDeadStrips.resize(fDigiBdfPar->GetNbDet());
  return true;
}
/************************************************************************************/
bool CbmTaskTofClusterizerParWrite::InitCalibParameter()
{
  // dimension and initialize calib parameter
  int32_t iNbSmTypes = fDigiBdfPar->GetNbSmTypes();

  if (fTotMean != 0.) fdTTotMean = fTotMean;  // adjust target mean for TOT

  fvCPTOff.resize(iNbSmTypes);
  fvCPTotGain.resize(iNbSmTypes);
  fvCPTotOff.resize(iNbSmTypes);
  fvCPWalk.resize(iNbSmTypes);
  fvCPTOffY.resize(iNbSmTypes);
  fvCPTOffYBinWidth.resize(iNbSmTypes);
  fvCPTOffYRange.resize(iNbSmTypes);
  for (int32_t iSmType = 0; iSmType < iNbSmTypes; iSmType++) {
    int32_t iNbSm  = fDigiBdfPar->GetNbSm(iSmType);
    int32_t iNbRpc = fDigiBdfPar->GetNbRpc(iSmType);
    fvCPTOff[iSmType].resize(iNbSm * iNbRpc);
    fvCPTotGain[iSmType].resize(iNbSm * iNbRpc);
    fvCPTotOff[iSmType].resize(iNbSm * iNbRpc);
    fvCPWalk[iSmType].resize(iNbSm * iNbRpc);
    fvCPTOffY[iSmType].resize(iNbSm * iNbRpc);
    fvCPTOffYBinWidth[iSmType].resize(iNbSm * iNbRpc);
    fvCPTOffYRange[iSmType].resize(iNbSm * iNbRpc);
    for (int32_t iSm = 0; iSm < iNbSm; iSm++) {
      for (int32_t iRpc = 0; iRpc < iNbRpc; iRpc++) {

        fvCPTOffYBinWidth[iSmType][iSm * iNbRpc + iRpc] = 0.;  // initialize
        fvCPTOffYRange[iSmType][iSm * iNbRpc + iRpc]    = 0.;  // initialize

        /*	D.Smith 23.8.23: For testing hit time calibration. Please remove when done.
        fvCPTOffYBinWidth[iSmType][iSm * iNbRpc + iRpc] = 1.;  // initialize
        fvCPTOffYRange[iSmType][iSm * iNbRpc + iRpc]    = 200.;  // initialize
        fvCPTOffY[iSmType][iSm * iNbRpc + iRpc] =  std::vector<double>(10000, 1000.);
        for( size_t i = 0; i < fvCPTOffY[iSmType][iSm * iNbRpc + iRpc].size(); i++ )
	{
		fvCPTOffY[iSmType][iSm * iNbRpc + iRpc][i] = 1000.+500.*i;
	}
*/
        int32_t iNbChan = fDigiBdfPar->GetNbChan(iSmType, iRpc);
        fvCPTOff[iSmType][iSm * iNbRpc + iRpc].resize(iNbChan);
        fvCPTotGain[iSmType][iSm * iNbRpc + iRpc].resize(iNbChan);
        fvCPTotOff[iSmType][iSm * iNbRpc + iRpc].resize(iNbChan);
        fvCPWalk[iSmType][iSm * iNbRpc + iRpc].resize(iNbChan);
        int32_t nbSide = 2 - fDigiBdfPar->GetChanType(iSmType, iRpc);
        for (int32_t iCh = 0; iCh < iNbChan; iCh++) {
          fvCPTOff[iSmType][iSm * iNbRpc + iRpc][iCh].resize(nbSide);
          fvCPTotGain[iSmType][iSm * iNbRpc + iRpc][iCh].resize(nbSide);
          fvCPTotOff[iSmType][iSm * iNbRpc + iRpc][iCh].resize(nbSide);
          fvCPWalk[iSmType][iSm * iNbRpc + iRpc][iCh].resize(nbSide);
          for (int32_t iSide = 0; iSide < nbSide; iSide++) {
            fvCPTOff[iSmType][iSm * iNbRpc + iRpc][iCh][iSide]    = 0.;  //initialize
            fvCPTotGain[iSmType][iSm * iNbRpc + iRpc][iCh][iSide] = 1.;  //initialize
            fvCPTotOff[iSmType][iSm * iNbRpc + iRpc][iCh][iSide]  = 0.;  //initialize
            fvCPWalk[iSmType][iSm * iNbRpc + iRpc][iCh][iSide].resize(nbClWalkBinX);
            for (int32_t iWx = 0; iWx < nbClWalkBinX; iWx++) {
              fvCPWalk[iSmType][iSm * iNbRpc + iRpc][iCh][iSide][iWx] = 0.;
            }
          }
        }
      }
    }
  }
  LOG(info) << "CbmTaskTofClusterizerParWrite::InitCalibParameter: defaults set";

  if (fCalMode <= 0) {
    return true;
  }  // Skip calibration from histograms in mode zero

  /// Save old global file and folder pointer to avoid messing with FairRoot
  // <= To prevent histos from being sucked in by the param file of the TRootManager!
  TFile* oldFile     = gFile;
  TDirectory* oldDir = gDirectory;

  LOG(info) << "CbmTaskTofClusterizerParWrite::InitCalibParameter: read histos from "
            << "file " << fCalParFileName;

  // read parameter from histos
  if (fCalParFileName.IsNull()) return true;

  TFile* calParFile = new TFile(fCalParFileName, "READ");
  if (NULL == calParFile) {
    if (fCalMode % 10 == 9) {  //modify reference file name
      int iCalMode              = fCalParFileName.Index("tofClust") - 3;
      fCalParFileName(iCalMode) = '3';
      LOG(info) << "Modified CalFileName = " << fCalParFileName;
      calParFile = new TFile(fCalParFileName, "update");
      if (NULL == calParFile)
        LOG(fatal) << "CbmTaskTofClusterizerParWrite::InitCalibParameter: "
                   << "file " << fCalParFileName << " does not exist!";

      return true;
    }
    LOG(fatal) << "Calibration parameter file not existing!";
  }

  for (int32_t iSmType = 0; iSmType < iNbSmTypes; iSmType++) {
    int32_t iNbSm   = fDigiBdfPar->GetNbSm(iSmType);
    int32_t iNbRpc  = fDigiBdfPar->GetNbRpc(iSmType);
    TProfile* hSvel = (TProfile*) gDirectory->FindObjectAny(Form("cl_SmT%01d_Svel", iSmType));
    for (int32_t iSm = 0; iSm < iNbSm; iSm++)
      for (int32_t iRpc = 0; iRpc < iNbRpc; iRpc++) {

        std::vector<std::vector<double>>& vCPTotGain           = fvCPTotGain[iSmType][iSm * iNbRpc + iRpc];
        std::vector<std::vector<double>>& vCPTOff              = fvCPTOff[iSmType][iSm * iNbRpc + iRpc];
        std::vector<std::vector<double>>& vCPTotOff            = fvCPTotOff[iSmType][iSm * iNbRpc + iRpc];
        std::vector<std::vector<std::vector<double>>>& vCPWalk = fvCPWalk[iSmType][iSm * iNbRpc + iRpc];

        std::vector<double>& vCPTOffY = fvCPTOffY[iSmType][iSm * iNbRpc + iRpc];
        double& vCPTOffYBinWidth      = fvCPTOffYBinWidth[iSmType][iSm * iNbRpc + iRpc];
        double& vCPTOffYRange         = fvCPTOffYRange[iSmType][iSm * iNbRpc + iRpc];

        // update default parameter
        if (NULL != hSvel) {
          double Vscal = hSvel->GetBinContent(iSm * iNbRpc + iRpc + 1);
          if (Vscal == 0.) Vscal = 1.;
          Vscal *= fdModifySigvel;  //1.03; // testing the effect of wrong signal velocity, FIXME
          fDigiBdfPar->SetSigVel(iSmType, iSm, iRpc, fDigiBdfPar->GetSigVel(iSmType, iSm, iRpc) * Vscal);
          LOG(info) << "Modify " << iSmType << iSm << iRpc << " Svel by " << Vscal << " to "
                    << fDigiBdfPar->GetSigVel(iSmType, iSm, iRpc);
        }
        TH2D* htempPos_pfx =
          (TH2D*) gDirectory->FindObjectAny(Form("cl_CorSmT%01d_sm%03d_rpc%03d_Pos_pfx", iSmType, iSm, iRpc));
        TH2D* htempTOff_pfx =
          (TH2D*) gDirectory->FindObjectAny(Form("cl_CorSmT%01d_sm%03d_rpc%03d_TOff_pfx", iSmType, iSm, iRpc));
        TH1D* htempTot_Mean =
          (TH1D*) gDirectory->FindObjectAny(Form("cl_CorSmT%01d_sm%03d_rpc%03d_Tot_Mean", iSmType, iSm, iRpc));
        TH1D* htempTot_Off =
          (TH1D*) gDirectory->FindObjectAny(Form("cl_CorSmT%01d_sm%03d_rpc%03d_Tot_Off", iSmType, iSm, iRpc));
        if (NULL != htempPos_pfx && NULL != htempTOff_pfx && NULL != htempTot_Mean && NULL != htempTot_Off) {
          int32_t iNbCh = fDigiBdfPar->GetNbChan(iSmType, iRpc);
          //int32_t iNbinTot = htempTot_Mean->GetNbinsX();//not used any more
          for (int32_t iCh = 0; iCh < iNbCh; iCh++) {
            for (int32_t iSide = 0; iSide < 2; iSide++) {
              double TotMean = htempTot_Mean->GetBinContent(iCh * 2 + 1 + iSide);  //nh +1 empirical(?)
              if (0.001 < TotMean) {
                vCPTotGain[iCh][iSide] *= fdTTotMean / TotMean;
              }
              vCPTotOff[iCh][iSide] = htempTot_Off->GetBinContent(iCh * 2 + 1 + iSide);
            }
            double YMean = ((TProfile*) htempPos_pfx)->GetBinContent(iCh + 1);  //nh +1 empirical(?)
            double TMean = ((TProfile*) htempTOff_pfx)->GetBinContent(iCh + 1);
            if (std::isnan(YMean) || std::isnan(TMean)) {
              LOG(warn) << "Invalid calibration for TSRC " << iSmType << iSm << iRpc << iCh << ", use default!";
              continue;
            }
            double dTYOff = YMean / fDigiBdfPar->GetSigVel(iSmType, iSm, iRpc);
            if (5 == iSmType || 8 == iSmType) dTYOff = 0.;  // no valid Y positon for pad counters
            vCPTOff[iCh][0] += -dTYOff + TMean;
            vCPTOff[iCh][1] += +dTYOff + TMean;

            if (5 == iSmType || 8 == iSmType) {  // for PAD counters
              vCPTOff[iCh][1]    = vCPTOff[iCh][0];
              vCPTotGain[iCh][1] = vCPTotGain[iCh][0];
              vCPTotOff[iCh][1]  = vCPTotOff[iCh][0];
            }
            //if(iSmType==0 && iSm==4 && iRpc==2 && iCh==26)
            if (iSmType == 0 && iSm == 2 && iRpc == 0)
              //if (iSmType == 9 && iSm == 0 && iRpc == 0 && iCh == 10)  // select specific channel
              LOG(info) << "InitCalibParameter:"
                        << " TSRC " << iSmType << iSm << iRpc << iCh
                        << Form(": YMean %6.3f, TYOff %6.3f, TMean %6.3f", YMean, dTYOff, TMean) << " -> "
                        << Form(" TOff %f, %f, TotG %f, %f ", vCPTOff[iCh][0], vCPTOff[iCh][1], vCPTotGain[iCh][0],
                                vCPTotGain[iCh][1]);

            TH1D* htempWalk0 = (TH1D*) gDirectory->FindObjectAny(
              Form("Cor_SmT%01d_sm%03d_rpc%03d_Ch%03d_S0_Walk_px", iSmType, iSm, iRpc, iCh));
            TH1D* htempWalk1 = (TH1D*) gDirectory->FindObjectAny(
              Form("Cor_SmT%01d_sm%03d_rpc%03d_Ch%03d_S1_Walk_px", iSmType, iSm, iRpc, iCh));
            if (NULL == htempWalk0 && NULL == htempWalk1) {  // regenerate Walk histos
              int iSide  = 0;
              htempWalk0 = new TH1D(Form("Cor_SmT%01d_sm%03d_rpc%03d_Ch%03d_S0_Walk_px", iSmType, iSm, iRpc, iCh),
                                    Form("Walk in SmT%01d_sm%03d_rpc%03d_Ch%03d_S%01d_Walk; Tot [a.u.];  #DeltaT [ns]",
                                         iSmType, iSm, iRpc, iCh, iSide),
                                    nbClWalkBinX, fdTOTMin, fdTOTMax);
              iSide      = 1;
              htempWalk1 = new TH1D(Form("Cor_SmT%01d_sm%03d_rpc%03d_Ch%03d_S0_Walk_px", iSmType, iSm, iRpc, iCh),
                                    Form("Walk in SmT%01d_sm%03d_rpc%03d_Ch%03d_S%01d_Walk; Tot [a.u.];  #DeltaT [ns]",
                                         iSmType, iSm, iRpc, iCh, iSide),
                                    nbClWalkBinX, fdTOTMin, fdTOTMax);
            }
            if (NULL != htempWalk0 && NULL != htempWalk1) {  // reinitialize Walk array
              LOG(debug) << "Initialize Walk correction for "
                         << Form(" SmT%01d_sm%03d_rpc%03d_Ch%03d", iSmType, iSm, iRpc, iCh);
              if (htempWalk0->GetNbinsX() != nbClWalkBinX)
                LOG(error) << "CbmTaskTofClusterizerParWrite::InitCalibParameter: "
                              "Inconsistent Walk histograms";
              for (int32_t iBin = 0; iBin < nbClWalkBinX; iBin++) {
                vCPWalk[iCh][0][iBin] = htempWalk0->GetBinContent(iBin + 1);
                vCPWalk[iCh][1][iBin] = htempWalk1->GetBinContent(iBin + 1);
                if (iSmType == 0 && iSm == 0 && iRpc == 2 && iCh == 15)  // debugging
                  LOG(info) << Form("Read new SmT%01d_sm%03d_rpc%03d_Ch%03d bin %d cen %f walk %f %f", iSmType, iSm,
                                    iRpc, iCh, iBin, htempWalk0->GetBinCenter(iBin + 1), vCPWalk[iCh][0][iBin],
                                    vCPWalk[iCh][1][iBin]);
                if (5 == iSmType || 8 == iSmType) {  // Pad structure, enforce consistency
                  if (vCPWalk[iCh][1][iBin] != vCPWalk[iCh][0][iBin]) {
                    LOG(fatal) << "Inconsisten walk values for TSRC " << iSmType << iSm << iRpc << iCh << ", Bin "
                               << iBin << ": " << vCPWalk[iCh][0][iBin] << ", " << vCPWalk[iCh][1][iBin];
                  }
                  vCPWalk[iCh][1][iBin] = vCPWalk[iCh][0][iBin];
                  htempWalk1->SetBinContent(iBin + 1, vCPWalk[iCh][1][iBin]);
                }
              }
            }
            else {
              LOG(info) << "No Walk histograms for TSRC " << iSmType << iSm << iRpc << iCh;
            }
          }
          // look for TcorY corrections
          LOG(info) << "Check for TCorY in " << gDirectory->GetName();
          TH1* hTCorY =
            (TH1*) gDirectory->FindObjectAny(Form("calXY_SmT%d_sm%03d_rpc%03d_TOff_z_all_TcorY", iSmType, iSm, iRpc));
          if (NULL != hTCorY) {
            vCPTOffYBinWidth = hTCorY->GetBinWidth(0);
            vCPTOffYRange    = hTCorY->GetXaxis()->GetXmax();
            LOG(info) << "Initialize TCorY: TSR " << iSmType << iSm << iRpc << ", Bwid " << vCPTOffYBinWidth
                      << ", Range " << vCPTOffYRange;
            vCPTOffY.resize(hTCorY->GetNbinsX());
            for (int iB = 0; iB < hTCorY->GetNbinsX(); iB++) {
              vCPTOffY[iB] = hTCorY->GetBinContent(iB + 1);
            }
          }
        }
        else {
          LOG(warning) << " Calibration histos " << Form("cl_SmT%01d_sm%03d_rpc%03d_XXX", iSmType, iSm, iRpc)
                       << " not found. ";
        }
      }
  }

  /// Restore old global file and folder pointer to avoid messing with FairRoot
  // <= To prevent histos from being sucked in by the param file of the TRootManager!
  gFile      = oldFile;
  gDirectory = oldDir;
  LOG(info) << "CbmTaskTofClusterizerParWrite::InitCalibParameter: initialization done";
  return true;
}

bool CbmTaskTofClusterizerParWrite::InitAlgos()
{
  // Needed as external TOT values might be different than ones used for input histo reading (TO DO: FIX)
  if (fTotMax != 0.) fdTOTMax = fTotMax;
  if (fTotMin != 0.) fdTOTMin = fTotMin;
  LOG(info) << "ToT init to Min " << fdTOTMin << " Max " << fdTOTMax;

  /// Go to Top volume of the geometry in the GeoManager to make sure our nodes are found
  gGeoManager->CdTop();

  int32_t iNbSmTypes = fDigiBdfPar->GetNbSmTypes();

  // Create map with unique detector IDs and fill (needed only for dead strip array)
  std::map<uint32_t, uint32_t> detIdIndexMap;
  for (int32_t ind = 0; ind < fDigiBdfPar->GetNbDet(); ind++) {
    int32_t iUniqueId        = fDigiBdfPar->GetDetUId(ind);
    detIdIndexMap[iUniqueId] = ind;
  }

  /* Hitfinding parameters */

  cbm::algo::tof::HitfindSetup setup;
  setup.rpcs.resize(iNbSmTypes);

  for (int32_t iSmType = 0; iSmType < iNbSmTypes; iSmType++) {

    int32_t iNbSm  = fDigiBdfPar->GetNbSm(iSmType);
    int32_t iNbRpc = fDigiBdfPar->GetNbRpc(iSmType);
    setup.NbSm.push_back(iNbSm);
    setup.NbRpc.push_back(iNbRpc);

    setup.rpcs[iSmType].resize(iNbSm * iNbRpc);

    for (int32_t iSm = 0; iSm < iNbSm; iSm++) {
      for (int32_t iRpc = 0; iRpc < iNbRpc; iRpc++) {

        cbm::algo::tof::HitfindSetup::Rpc par;

        /* Cell geometry for hitfinding (can in principle be set channel-wise but is not needed for now) */

        const int32_t rpcAddress = CbmTofAddress::GetUniqueAddress(iSm, iRpc, 0, 0, iSmType);
        CbmTofCell* channelInfo  = fDigiPar->GetCell(rpcAddress);
        if (channelInfo == nullptr) {
          continue;
        }

        // prepare local->global trafo
        gGeoManager->FindNode(channelInfo->GetX(), channelInfo->GetY(), channelInfo->GetZ());
        const double* tra_ptr = gGeoManager->MakePhysicalNode()->GetMatrix()->GetTranslation();
        const double* rot_ptr = gGeoManager->GetCurrentMatrix()->GetRotationMatrix();
        memcpy(par.cell.translation.data(), tra_ptr, 3 * sizeof(double));
        memcpy(par.cell.rotation.data(), rot_ptr, 9 * sizeof(double));

        par.cell.sizeX = channelInfo->GetSizex();
        par.cell.sizeY = channelInfo->GetSizey();

        /* Other hitfinding parameters */

        const int32_t iDetId   = CbmTofAddress::GetUniqueAddress(iSm, iRpc, 0, 0, iSmType);
        const int32_t iDetIndx = detIdIndexMap[iDetId];
        par.deadStrips         = fvDeadStrips[iDetIndx];
        par.posYMaxScal        = fPosYMaxScal;
        par.maxTimeDist        = fdMaxTimeDist;
        par.maxSpaceDist       = fdMaxSpaceDist;
        par.sigVel             = fDigiBdfPar->GetSigVel(iSmType, iSm, iRpc);
        par.timeRes            = 0.08;
        par.trackingStationId  = fDigiBdfPar->GetTrackingStation(iSmType, iSm, iRpc);
        par.CPTOffYBinWidth    = fvCPTOffYBinWidth[iSmType][iSm * iNbRpc + iRpc];
        par.CPTOffY            = fvCPTOffY[iSmType][iSm * iNbRpc + iRpc];
        par.CPTOffYRange       = fvCPTOffYRange[iSmType][iSm * iNbRpc + iRpc];

        int32_t iNbChan = fDigiBdfPar->GetNbChan(iSmType, iRpc);
        par.chanPar.resize(iNbChan);

        for (int32_t iCh = 0; iCh < iNbChan; iCh++) {
          cbm::algo::tof::HitfindSetup::Channel& chan = par.chanPar[iCh];

          const int32_t address = CbmTofAddress::GetUniqueAddress(iSm, iRpc, iCh, 0, iSmType);
          chan.address          = address;
        }
        setup.rpcs[iSmType][iSm * iNbRpc + iRpc] = par;
      }
    }
  }

  /* Calibration parameters */

  cbm::algo::tof::CalibrateSetup calib;
  calib.rpcs.resize(iNbSmTypes);

  for (int32_t iSmType = 0; iSmType < iNbSmTypes; iSmType++) {
    int32_t iNbSm  = fDigiBdfPar->GetNbSm(iSmType);
    int32_t iNbRpc = fDigiBdfPar->GetNbRpc(iSmType);
    calib.NbSm.push_back(iNbSm);
    calib.NbRpc.push_back(iNbRpc);
    calib.rpcs[iSmType].resize(iNbSm * iNbRpc);

    for (int32_t iSm = 0; iSm < iNbSm; iSm++) {
      for (int32_t iRpc = 0; iRpc < iNbRpc; iRpc++) {

        cbm::algo::tof::CalibrateSetup::Rpc par;
        par.numClWalkBinX    = nbClWalkBinX;
        par.TOTMax           = fdTOTMax;
        par.TOTMin           = fdTOTMin;
        par.swapChannelSides = fbSwapChannelSides;
        par.channelDeadtime  = fdChannelDeadtime;

        int32_t iNbChan = fDigiBdfPar->GetNbChan(iSmType, iRpc);
        par.chanPar.resize(iNbChan);

        for (int32_t iCh = 0; iCh < iNbChan; iCh++) {
          cbm::algo::tof::CalibrateSetup::Channel& chan = par.chanPar[iCh];

          chan.vCPTOff    = fvCPTOff[iSmType][iSm * iNbRpc + iRpc][iCh];
          chan.vCPTotGain = fvCPTotGain[iSmType][iSm * iNbRpc + iRpc][iCh];
          chan.vCPTotOff  = fvCPTotOff[iSmType][iSm * iNbRpc + iRpc][iCh];
          chan.vCPWalk    = fvCPWalk[iSmType][iSm * iNbRpc + iRpc][iCh];
        }
        calib.rpcs[iSmType][iSm * iNbRpc + iRpc] = par;
      }
    }
  }

  /* Write Yaml files */

  cbm::algo::yaml::Dump dump;
  std::ofstream fout("TofHitfinderPar.yaml");
  fout << dump(setup);
  fout.close();

  std::ofstream fcalout("TofCalibratePar.yaml");
  fcalout << dump(calib);
  fcalout.close();

  return true;
}
