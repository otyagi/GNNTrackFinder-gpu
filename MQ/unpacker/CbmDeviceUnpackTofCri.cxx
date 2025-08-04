/* Copyright (C) 2018-2020 PI-UHd, GSI
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig, Norbert Herrmann [committer] */

/**
 * CbmDeviceUnpackTofMcbm2018.cxx
 *
 * @since 2018-04-24
 * @author F. Uhlig
 */

#include "CbmDeviceUnpackTofCri.h"

#include "CbmDefs.h"
#include "CbmMQDefs.h"
#include "CbmMcbm2018TofPar.h"
#include "CbmTofUnpackAlgo.h"
#include "CbmTofUnpackConfig.h"
//#include "CbmHistManager.h"
#include "CbmTbDaqBuffer.h"
#include "CbmTimeSlice.h"
#include "CbmTofAddress.h"
#include "CbmTofDetectorId_v21a.h"  // in cbmdata/tof
#include "CbmTofDigi.h"
//#include "CbmRawEvent.h"

#include "StorableTimeslice.hpp"

#include "FairMQLogger.h"
#include "FairMQProgOptions.h"  // device->fConfig
#include "FairParGenericSet.h"
#include "FairRuntimeDb.h"

#include "TH1.h"
#include "TH2.h"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

// include this header to serialize vectors
#include <boost/serialization/vector.hpp>

#include <array>
#include <iomanip>
#include <stdexcept>
#include <string>
struct InitTaskError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

using namespace std;

static Int_t iReportMess = 0;

//const Int_t DetMask = 0x001FFFFF;
std::shared_ptr<CbmTofUnpackConfig> fTofConfig = nullptr;  //!

CbmDeviceUnpackTofCri::CbmDeviceUnpackTofCri()
  : fNumMessages(0)
  , fiSelectComponents(0)
  , fNumTint(0)
  , fEventHeader()
  , fiReqMode(0)
  , fiReqTint(0)
  , fiReqBeam(-1)
  , fiReqDigiAddr()
  , fiPulserMode(0)
  , fiPulMulMin(0)
  , fiPulTotMin(0)
  , fiPulTotMax(1000)
  , fuTotalMsNb(0)
  , fuOverlapMsNb(0)
  , fuCoreMs(0)
  , fdMsSizeInNs(0)
  , fdTsCoreSizeInNs(0)
  , fuMinNbGdpb(0)
  , fuNrOfGdpbs(0)
  , fuNrOfFeePerGdpb(0)
  , fuNrOfGet4PerFee(0)
  , fuNrOfChannelsPerGet4(0)
  , fuNrOfChannelsPerFee(0)
  , fuNrOfGet4(0)
  , fuNrOfGet4PerGdpb(0)
  , fuNrOfChannelsPerGdpb(0)
  , fuGdpbId(0)
  , fuGdpbNr(0)
  , fuGet4Id(0)
  , fuGet4Nr(0)
  , fMsgCounter(11, 0)  // length of enum MessageTypes initialized with 0
  , fGdpbIdIndexMap()
  , fvulCurrentEpoch()
  , fvbFirstEpochSeen()
  , fNofEpochs(0)
  , fulCurrentEpochTime(0.)
  //, fdMsIndex(0.)
  , fdToffTof(0.)
  , fiAddrRef(-1)
  //, fuDiamondDpbIdx(3)
  //, fbEpochSuppModeOn( kTRUE )
  //, fbGet4M24b( kFALSE )
  //, fbGet4v20( kTRUE )
  //, fbMergedEpochsOn( kTRUE )
  , fUnpackPar(nullptr)
  , fdLastDigiTime(0.)
  , fdFirstDigiTimeDif(0.)
  //, fdEvTime0(0.)
  , fhRawTDigEvBmon(nullptr)
  , fhRawTDigRef0(nullptr)
  , fhRawTDigRef(nullptr)
  , fhRawTRefDig0(nullptr)
  , fhRawTRefDig1(nullptr)
  , fhRawDigiLastDigi(nullptr)
  , fhRawTotCh()
  , fhChCount()
  , fvbChanThere()
  , fhChanCoinc()
  , fhDetChanCoinc(nullptr)
  , fvuPadiToGet4()
  , fvuGet4ToPadi()
  , fvuElinkToGet4()
  , fvuGet4ToElink()
  , fviRpcType()
  , fviModuleId()
  , fviNrOfRpc()
  , fviRpcSide()
  , fviRpcChUId()
  , fBuffer(CbmTbDaqBuffer::Instance())
  , fUnpackerAlgo(nullptr)
{
  //fUnpackerAlgo = new CbmTofUnpackAlgo();
}

CbmDeviceUnpackTofCri::~CbmDeviceUnpackTofCri() {}

void CbmDeviceUnpackTofCri::InitTask()
try {
  // Get the information about created channels from the device
  // Check if the defined channels from the topology (by name)
  // are in the list of channels which are possible/allowed
  // for the device
  // The idea is to check at initilization if the devices are
  // properly connected. For the time beeing this is done with a
  // nameing convention. It is not avoided that someone sends other
  // data on this channel.
  //logger::SetLogLevel("INFO");

  int noChannel = fChannels.size();
  LOG(info) << "Number of defined channels: " << noChannel;
  for (auto const& entry : fChannels) {
    LOG(info) << "Channel name: " << entry.first;
    if (!IsChannelNameAllowed(entry.first)) throw InitTaskError("Channel name does not match.");
    if (entry.first == "syscmd") {
      OnData(entry.first, &CbmDeviceUnpackTofCri::HandleMessage);
      continue;
    }
    //if(entry.first != "tofdigis") OnData(entry.first, &CbmDeviceUnpackTofCri::HandleData);
    if (entry.first != "tofdigis") OnData(entry.first, &CbmDeviceUnpackTofCri::HandleParts);
    else {
      fChannelsToSend[0].push_back(entry.first);
      LOG(info) << "Init to send data to channel " << fChannelsToSend[0][0];
    }
  }

  // ---- TOF ----
  Int_t runid = 111;

  fTofConfig = std::make_shared<CbmTofUnpackConfig>("", runid);
  // fTofConfig->SetDoWriteOutput();
  // fTofConfig->SetDoWriteOptOutA("CbmTofErrors");
  if (fTofConfig) {
    LOG(info) << "Configure Tof Unpacker ";
    fTofConfig->SetDebugState();
    fTofConfig->SetAlgo();
    fUnpackerAlgo = fTofConfig->GetUnpacker();
    InitContainers();
    //fTofConfig->InitUnpacker();
  }
  else
    LOG(fatal) << "Tof Unpacker not configured ";

  const Int_t iHeaderSize = 5;
  fEventHeader.resize(iHeaderSize);  // define size of eventheader int[]
  for (int i = 0; i < iHeaderSize; i++)
    fEventHeader[i] = 0;
  LOG(info) << "Read config";
  fiSelectComponents = fConfig->GetValue<uint64_t>("SelectComponents");
  fiReqMode          = fConfig->GetValue<uint64_t>("ReqMode");
  fiReqTint          = fConfig->GetValue<uint64_t>("ReqTint");
  fiReqBeam          = fConfig->GetValue<uint64_t>("ReqBeam");
  fiPulserMode       = fConfig->GetValue<int64_t>("PulserMode");
  fiPulMulMin        = fConfig->GetValue<uint64_t>("PulMulMin");
  fiPulTotMin        = fConfig->GetValue<uint64_t>("PulTotMin");
  fiPulTotMax        = fConfig->GetValue<uint64_t>("PulTotMax");
  fdToffTof          = fConfig->GetValue<double_t>("ToffTof");
  Int_t iRefModType  = fConfig->GetValue<int64_t>("RefModType");
  Int_t iRefModId    = fConfig->GetValue<int64_t>("RefModId");
  Int_t iRefCtrType  = fConfig->GetValue<int64_t>("RefCtrType");
  Int_t iRefCtrId    = fConfig->GetValue<int64_t>("RefCtrId");
  if (iRefModType > -1)
    fiAddrRef = CbmTofAddress::GetUniqueAddress(iRefModId, iRefCtrId, 0, 0, iRefModType, iRefCtrType);
  LOG(info) << " Using reference counter address 0x" << std::hex << fiAddrRef << " and offset shift " << std::dec
            << fdToffTof;

  //    Int_t iMaxAsicInactive = fConfig->GetValue<uint64_t>("MaxAsicInactive");
  //    fUnpackerAlgo->SetMaxAsicInactive( iMaxAsicInactive );
  Int_t iReqDet       = 1;
  Int_t iNReq         = 0;
  const Int_t iMaxReq = 50;

  while (iNReq < iMaxReq) {  // FIXME, setup parameter hardwired!
    iReqDet = fConfig->GetValue<uint64_t>(Form("ReqDet%d", iNReq));
    if (iReqDet == 0) break;
    AddReqDigiAddr(iReqDet);
    iNReq++;
  }
  LOG(info) << "Setup request";
  if (fiReqMode > 0)
    if (iNReq == 0) {  // take all defined detectors
      for (UInt_t iGbtx = 0; iGbtx < fviNrOfRpc.size(); iGbtx++) {
        switch (fviRpcType[iGbtx]) {
          case 0:  // mTof modules
          case 1:  // eTof modules
          case 2:  // M6   modules
            if (iGbtx % 2 == 0)
              for (Int_t iRpc = 0; iRpc < fviNrOfRpc[iGbtx]; iRpc++) {
                Int_t iAddr = CbmTofAddress::GetUniqueAddress(fviModuleId[iGbtx], iRpc, 0, 0, fviRpcType[iGbtx]);
                AddReqDigiAddr(iAddr);
              }
            break;

          case 4:
          case 9:  // HD 2-RPC boxes
            for (Int_t iRpc = 0; iRpc < 2; iRpc++) {
              Int_t iAddr = CbmTofAddress::GetUniqueAddress(fviModuleId[iGbtx], iRpc, 0, 0, fviRpcType[iGbtx]);
              AddReqDigiAddr(iAddr);
            }
            break;
          case 6:  // Buc box
            for (Int_t iRpc = 0; iRpc < 2; iRpc++) {
              Int_t iAddr = CbmTofAddress::GetUniqueAddress(fviModuleId[iGbtx], iRpc, 0, 0, fviRpcType[iGbtx]);
              AddReqDigiAddr(iAddr);
            }
            break;

          case 7:  // CERN box
            for (Int_t iRpc = 0; iRpc < 1; iRpc++) {
              Int_t iAddr = CbmTofAddress::GetUniqueAddress(fviModuleId[iGbtx], iRpc, 0, 0, 7);
              AddReqDigiAddr(iAddr);
            }
            break;
          case 8:  // ceramics
            for (Int_t iRpc = 0; iRpc < 8; iRpc++) {
              Int_t iAddr = CbmTofAddress::GetUniqueAddress(fviModuleId[iGbtx], iRpc, 0, 0, fviRpcType[iGbtx]);
              AddReqDigiAddr(iAddr);
            }
            break;
          case 5:  // add Diamond, single cell RPC
            Int_t iAddr = CbmTofAddress::GetUniqueAddress(fviModuleId[iGbtx], 0, 0, 0, 5);
            AddReqDigiAddr(iAddr);
            break;
        }
      }
    }

  LOG(info) << "ReqMode " << fiReqMode << " in " << fiReqTint << " ns "
            << " with " << fiReqDigiAddr.size() << " detectors out of " << fviNrOfRpc.size() << " GBTx, PulserMode "
            << fiPulserMode << " with Mul " << fiPulMulMin << ", TotMin " << fiPulTotMin;
  LOG(info) << Form("ReqBeam 0x%08x", (uint) fiReqBeam);
}
catch (InitTaskError& e) {
  LOG(error) << e.what();
  // Wrapper defined in CbmMQDefs.h to support different FairMQ versions
  cbm::mq::ChangeState(this, cbm::mq::Transition::ErrorFound);
}

bool CbmDeviceUnpackTofCri::IsChannelNameAllowed(std::string channelName)
{
  for (auto const& entry : fAllowedChannels) {
    LOG(info) << "Inspect " << entry;
    std::size_t pos1 = channelName.find(entry);
    if (pos1 != std::string::npos) {
      const vector<std::string>::const_iterator pos =
        std::find(fAllowedChannels.begin(), fAllowedChannels.end(), entry);
      const vector<std::string>::size_type idx = pos - fAllowedChannels.begin();
      LOG(info) << "Found " << entry << " in " << channelName;
      LOG(info) << "Channel name " << channelName << " found in list of allowed channel names at position " << idx;
      return true;
    }
  }
  LOG(info) << "Channel name " << channelName << " not found in list of allowed channel names.";
  LOG(error) << "Stop device.";
  return false;
}

Bool_t CbmDeviceUnpackTofCri::InitContainers()
{
  LOG(info) << "Init parameter containers for CbmDeviceUnpackTofCri";
  //  FairRuntimeDb* fRtdb = FairRuntimeDb::instance();

  // NewSimpleMessage creates a copy of the data and takes care of its destruction (after the transfer takes place).
  // Should only be used for small data because of the cost of an additional copy
  std::string message {"CbmMcbm2018TofPar,111"};
  LOG(info) << "Requesting parameter container CbmMcbm2018TofPar, sending message: " << message;

  FairMQMessagePtr req(NewSimpleMessage("CbmMcbm2018TofPar,111"));
  FairMQMessagePtr rep(NewMessage());

  if (Send(req, "parameters") > 0) {
    if (Receive(rep, "parameters") >= 0) {
      if (rep->GetSize() != 0) {
        CbmMQTMessage tmsg(rep->GetData(), rep->GetSize());
        fUnpackPar = dynamic_cast<CbmMcbm2018TofPar*>(tmsg.ReadObject(tmsg.GetClass()));
        LOG(info) << "Received unpack parameter from parmq server: " << fUnpackPar;
        LOG(info) << "NrOfGdpbs: " << fUnpackPar->GetNrOfGdpbs();
      }
      else {
        LOG(error) << "Received empty reply. Parameter not available";
      }
    }
  }

  //FairParGenericSet* parset=
  SetParContainers();

  Bool_t initOK = kTRUE;
  //Int_t fRunId=111;
  //string fGeoSetupTag="v21c";
  //initOK &= fUnpackerAlgo->InitContainers();
  //auto reqparvec = fUnpackerAlgo->GetParContainerRequest(fGeoSetupTag, fRunId);
  //initOk &= initParContainers(reqparvec);
  initOK &= ReInitContainers();  // needed for TInt parameters

  // CreateHistograms();
  // initOK &= fUnpackerAlgo->CreateHistograms();

  fvulCurrentEpoch.resize(fuNrOfGdpbs * fuNrOfGet4PerGdpb);
  fvbFirstEpochSeen.resize(fuNrOfGdpbs * fuNrOfGet4PerGdpb);
  fvbChanThere.resize(fviRpcChUId.size(), kFALSE);
  for (UInt_t i = 0; i < fuNrOfGdpbs; ++i) {
    for (UInt_t j = 0; j < fuNrOfGet4PerGdpb; ++j) {
      fvulCurrentEpoch[GetArrayIndex(i, j)]  = 0;
      fvbFirstEpochSeen[GetArrayIndex(i, j)] = kFALSE;
    }  // for( UInt_t j = 0; j < fuNrOfGet4PerGdpb; ++j )
  }    // for( UInt_t i = 0; i < fuNrOfGdpbs; ++i )

  fNumTint = 0;
  return initOK;
}

void CbmDeviceUnpackTofCri::SetParContainers()
{
  //FairRuntimeDb* fRtdb = FairRuntimeDb::instance();
  LOG(info) << "NrOfGdpbs: " << fUnpackPar->GetNrOfGdpbs();

  // PAL, 2022/07/05: use algo native pointer interface + sanity checks to set the parameter
  auto reqparvec = fTofConfig->GetParContainerRequest();
  if (1 == reqparvec->size() && (reqparvec->at(0).second->IsA() == CbmMcbm2018TofPar::Class())) {
    reqparvec->at(0).second.reset(fUnpackPar);
  }
  else {
    LOG(fatal) << "CbmDeviceUnpackTofCri::SetParContainers => wronf number of parameters needed for the algo or wrong "
               << "Parameter type";
  }
  /*
  TList* fParCList; // = fUnpackerAlgo->GetParList();

  LOG(info) << "Setting parameter containers for " << fParCList->GetEntries() << " entries ";

  for (Int_t iparC = 0; iparC < fParCList->GetEntries(); ++iparC) {
    FairParGenericSet* tempObj = (FairParGenericSet*) (fParCList->At(iparC));
    fParCList->Remove(tempObj);

    std::string sParamName {tempObj->GetName()};

    FairParGenericSet* newObj = dynamic_cast<FairParGenericSet*>(fRtdb->getContainer(sParamName.data()));
    LOG(info) << " - Get " << sParamName.data() << " at " << newObj;
    if (nullptr == newObj) {

      LOG(error) << "Failed to obtain parameter container " << sParamName << ", for parameter index " << iparC;
      return;
    }  // if( nullptr == newObj )
    if (iparC == 0) {
      newObj = (FairParGenericSet*) fUnpackPar;
      LOG(info) << " - Mod " << sParamName.data() << " to " << newObj;
    }
    fParCList->AddAt(newObj, iparC);
    //      delete tempObj;
  }  // for( Int_t iparC = 0; iparC < fParCList->GetEntries(); ++iparC )
  */
}

void CbmDeviceUnpackTofCri::AddMsComponentToList(size_t /*component*/, UShort_t /*usDetectorId*/)
{
  //fUnpackerAlgo->AddMsComponentToList(component, usDetectorId);
}

Bool_t CbmDeviceUnpackTofCri::ReInitContainers()
{
  LOG(info) << "ReInit parameter containers for CbmDeviceUnpackCriTofPar.";

  UInt_t uNrOfGbtx = fUnpackPar->GetNrOfGbtx();
  fviRpcType.resize(uNrOfGbtx);
  fviModuleId.resize(uNrOfGbtx);
  fviNrOfRpc.resize(uNrOfGbtx);
  fviRpcSide.resize(uNrOfGbtx);

  for (UInt_t iGbtx = 0; iGbtx < uNrOfGbtx; ++iGbtx) {
    fviNrOfRpc[iGbtx]  = fUnpackPar->GetNrOfRpc(iGbtx);
    fviRpcType[iGbtx]  = fUnpackPar->GetRpcType(iGbtx);
    fviRpcSide[iGbtx]  = fUnpackPar->GetRpcSide(iGbtx);
    fviModuleId[iGbtx] = fUnpackPar->GetModuleId(iGbtx);
  }
  return kTRUE;
}

void CbmDeviceUnpackTofCri::CreateHistograms()
{
  LOG(info) << "create Histos for " << fuNrOfGdpbs << " gDPBs ";

  fhRawTDigEvBmon =
    new TH1F(Form("Raw_TDig-EvBmon"), Form("Raw digi time difference to 1st digi ; time [ns]; cts"), 500, 0, 100.);
  //   fHM->Add( Form("Raw_TDig-EvBmon"), fhRawTDigEvBmon);

  fhRawTDigRef0 =
    new TH1F(Form("Raw_TDig-Ref0"), Form("Raw digi time difference to Ref ; time [ns]; cts"), 6000, -10000, 50000);
  //   fHM->Add( Form("Raw_TDig-Ref0"), fhRawTDigRef0);

  fhRawTDigRef =
    new TH1F(Form("Raw_TDig-Ref"), Form("Raw digi time difference to Ref ; time [ns]; cts"), 6000, -1000, 5000);
  //   fHM->Add( Form("Raw_TDig-Ref"), fhRawTDigRef);

  fhRawTRefDig0 = new TH1F(Form("Raw_TRef-Dig0"), Form("Raw Ref time difference to last digi  ; time [ns]; cts"), 9999,
                           -50000, 50000);
  //   fHM->Add( Form("Raw_TRef-Dig0"), fhRawTRefDig0);

  fhRawTRefDig1 =
    new TH1F(Form("Raw_TRef-Dig1"), Form("Raw Ref time difference to last digi  ; time [ns]; cts"), 9999, -5000, 5000);
  //   fHM->Add( Form("Raw_TRef-Dig1"), fhRawTRefDig1);

  fhRawDigiLastDigi = new TH1F(Form("Raw_Digi-LastDigi"),
                               Form("Raw Digi time difference to last digi  ; time [ns]; cts"), 9999, -5000, 5000);
  //                                 9999, -5000000, 5000000);
  //   fHM->Add( Form("Raw_Digi-LastDigi"), fhRawDigiLastDigi);

  fhRawTotCh.resize(fuNrOfGdpbs);
  fhChCount.resize(fuNrOfGdpbs);
  fhChanCoinc.resize(fuNrOfGdpbs * fuNrOfFeePerGdpb / 2);
  for (UInt_t uGdpb = 0; uGdpb < fuNrOfGdpbs; uGdpb++) {
    fhRawTotCh[uGdpb] = new TH2F(Form("Raw_Tot_gDPB_%02u", uGdpb), Form("Raw TOT gDPB %02u; channel; TOT [bin]", uGdpb),
                                 fuNrOfChannelsPerGdpb, 0., fuNrOfChannelsPerGdpb, 256, 0., 256.);
    //      fHM->Add( Form("Raw_Tot_gDPB_%02u", uGdpb), fhRawTotCh[ uGdpb ]);

    fhChCount[uGdpb] =
      new TH1I(Form("ChCount_gDPB_%02u", uGdpb), Form("Channel counts gDPB %02u; channel; Hits", uGdpb),
               fuNrOfChannelsPerGdpb, 0., fuNrOfChannelsPerGdpb);
    //      fHM->Add( Form("ChCount_gDPB_%02u", uGdpb), fhChCount[ uGdpb ]);
    /*
	for( UInt_t uLeftFeb = uGdpb*fuNrOfFebsPerGdpb / 2;
	uLeftFeb < (uGdpb + 1 )*fuNrOfFebsPerGdpb / 2;
	++uLeftFeb )
	{
	fhChanCoinc[ uLeftFeb ] = new TH2F( Form("fhChanCoinc_%02u", uLeftFeb),
	Form("Channels Coincidence %02; Left; Right", uLeftFeb),
	fuNrOfChannelsPerFee, 0., fuNrOfChannelsPerFee,
	fuNrOfChannelsPerFee, 0., fuNrOfChannelsPerFee );
	} // for( UInt_t uLeftFeb = 0; uLeftFeb < fuNrOfFebsPerGdpb / 2; uLeftFeb ++ )
      */
    fhChanCoinc[uGdpb] =
      new TH2F(Form("fhChanCoinc_%02u", uGdpb), Form("Channels Coincidence %02u; Left; Right", uGdpb),
               fuNrOfChannelsPerGdpb, 0., fuNrOfChannelsPerGdpb, fuNrOfChannelsPerGdpb, 0., fuNrOfChannelsPerGdpb);
  }  // for( UInt_t uGdpb = 0; uGdpb < fuMinNbGdpb; uGdpb ++)
  fhDetChanCoinc = new TH2F("fhDetChanCoinc", "Det Channels Coincidence; Left; Right", 32, 0., 32, 32, 0., 32);
}

// handler is called whenever a message arrives on "data", with a reference to the message and a sub-channel index (here 0)
bool CbmDeviceUnpackTofCri::HandleData(FairMQMessagePtr& msg, int /*index*/)
{
  // Don't do anything with the data
  // Maybe add an message counter which counts the incomming messages and add
  // an output
  fNumMessages++;
  LOG(debug) << "Received message number " << fNumMessages << " with size " << msg->GetSize();

  std::string msgStr(static_cast<char*>(msg->GetData()), msg->GetSize());
  std::istringstream iss(msgStr);
  boost::archive::binary_iarchive inputArchive(iss);

  fles::StorableTimeslice component {0};
  inputArchive >> component;

  //  CheckTimeslice(component);

  DoUnpack(component, 0);

  BuildTint(component.start_time(), 0);

  if (fNumMessages % 1000 == 0) LOG(info) << "Processed " << fNumMessages << " time slices";

  return true;
}

bool CbmDeviceUnpackTofCri::HandleParts(FairMQParts& parts, int /*index*/)
{
  // Don't do anything with the data
  // Maybe add an message counter which counts the incomming messages and add
  // an output
  fNumMessages++;
  LOG(debug) << "Received message number " << fNumMessages << " with " << parts.Size() << " parts";

  fles::StorableTimeslice ts {0};
  uint64_t ulTsStartTime = 0;

  switch (fiSelectComponents) {
    case 0: {
      std::string msgStr(static_cast<char*>(parts.At(0)->GetData()), (parts.At(0))->GetSize());
      std::istringstream iss(msgStr);
      boost::archive::binary_iarchive inputArchive(iss);
      inputArchive >> ts;
      //CheckTimeslice(ts);
      for (size_t c {0}; c < ts.num_components(); c++) {
        auto systemID = static_cast<int>(ts.descriptor(c, 0).sys_id);
        if (1 == fNumMessages) {
          LOG(info) << "Found systemID: " << std::hex << systemID << std::dec;
          //fUnpackerAlgo->AddMsComponentToList(c, systemID);  // TOF data
        }
        if (systemID == 0x60 || systemID == 0x90) {  // FIXME hardwired numbers
          //LOG(info) << "Call unpacker for component " << c;
          DoUnpack(ts, c);
        }
      }
      ulTsStartTime = ts.start_time();
    } break;
    case 1: {
      fles::StorableTimeslice component {0};

      uint ncomp = parts.Size();
      for (uint i = 0; i < ncomp; i++) {
        std::string msgStr(static_cast<char*>(parts.At(i)->GetData()), (parts.At(i))->GetSize());
        std::istringstream iss(msgStr);
        boost::archive::binary_iarchive inputArchive(iss);
        //fles::StorableTimeslice component{i};
        inputArchive >> component;

        //      CheckTimeslice(component);
        //fUnpackerAlgo->AddMsComponentToList(0, 0x60);  // TOF data
        LOG(debug) << "HandleParts message " << fNumMessages << ", component " << i << ",size "
                   << (parts.At(i))->GetSize();
        DoUnpack(component, 0);
      }
      ulTsStartTime = component.start_time();
    } break;
    default:;
  }

  BuildTint(ulTsStartTime, 0);

  if (fNumMessages % 100 == 0) LOG(info) << "Processed " << fNumMessages << " time slices";
  //LOG(info) << "HandleParts done with message number " << fNumMessages;

  return true;
}

bool CbmDeviceUnpackTofCri::HandleMessage(FairMQMessagePtr& msg, int /*index*/)
{
  const char* cmd    = (char*) (msg->GetData());
  const char cmda[4] = {*cmd};
  LOG(info) << "Handle message " << cmd << ", " << cmd[0];
  cbm::mq::LogState(this);

  // only one implemented so far "Stop"

  if (strcmp(cmda, "STOP")) {
    LOG(info) << "STOP";
    fUnpackerAlgo->Finish();
    cbm::mq::ChangeState(this, cbm::mq::Transition::Ready);
    cbm::mq::LogState(this);
    cbm::mq::ChangeState(this, cbm::mq::Transition::DeviceReady);
    cbm::mq::LogState(this);
    cbm::mq::ChangeState(this, cbm::mq::Transition::Idle);
    cbm::mq::LogState(this);
    cbm::mq::ChangeState(this, cbm::mq::Transition::End);
    cbm::mq::LogState(this);
  }
  return true;
}

Bool_t CbmDeviceUnpackTofCri::DoUnpack(const fles::Timeslice& ts, size_t component)
{
  LOG(debug) << "Timeslice " << ts.index() << " contains " << ts.num_microslices(component)
             << " microslices of component " << component << ", NCoreMs: " << ts.num_core_microslices();

  fUnpackerAlgo->SetTsStartTime(ts.start_time());
  std::vector<CbmTofDigi> vDigi = fUnpackerAlgo->Unpack(&ts, component);

  LOG(debug) << "TS " << ts.index() << ", startTime " << ts.start_time() << ": insert " << vDigi.size()
             << " digis of component " << component << " into DAQ buffer  with size " << fBuffer->GetSize();

  CbmTofDigi* fDigi     = NULL;
  CbmTofDigi* fDigiLast = NULL;
  for (auto digi : vDigi) {
    // Memorize last digi
    if (NULL != fDigi) fDigiLast = fDigi;
    // copy Digi for insertion into DAQ buffer
    fDigi = new CbmTofDigi(digi);

    //if( (fDigi->GetAddress() & 0x000F00F ) != fiAddrRef )  fDigi->SetTime(fDigi->GetTime()+fdToffTof); // shift all Tof Times for v14a geometries
    if ((fDigi->GetAddress() & 0x000780F) != fiAddrRef)
      fDigi->SetTime(fDigi->GetTime() + fdToffTof);  // shift all Tof Times for V21a

    LOG(debug) << "BufferInsert TSRCS "
               << Form("%d%d%d%02d%d", (int) fDigi->GetType(), (int) fDigi->GetSm(), (int) fDigi->GetRpc(),
                       (int) fDigi->GetChannel(), (int) fDigi->GetSide())
               << Form(", 0x%08x, t %012.2f", fDigi->GetAddress(), fDigi->GetTime())
               << Form(", first %012.2f, last %012.2f, size %u", fBuffer->GetTimeFirst(), fBuffer->GetTimeLast(),
                       fBuffer->GetSize());
    if (NULL != fDigiLast) {
      if ((fDigi->GetTime() * 1000) == (fDigiLast->GetTime() * 1000)) {
        if (iReportMess++ < 1000)
          LOG(warn) << "Successive digis with same time: "
                    << Form("%12.3f, TSRCS %d%d%d%02d%d - %d%d%d%02d%d", fDigi->GetTime(), (Int_t) fDigi->GetType(),
                            (Int_t) fDigi->GetSm(), (Int_t) fDigi->GetRpc(), (Int_t) fDigi->GetChannel(),
                            (Int_t) fDigi->GetSide(), (Int_t) fDigiLast->GetType(), (Int_t) fDigiLast->GetSm(),
                            (Int_t) fDigiLast->GetRpc(), (Int_t) fDigiLast->GetChannel(), (Int_t) fDigiLast->GetSide());
      }
      else
        fBuffer->InsertData<CbmTofDigi>(fDigi);
    }
    else
      fBuffer->InsertData<CbmTofDigi>(fDigi);

    LOG(debug) << "I "
               << " TSRC " << fDigi->GetType() << fDigi->GetSm() << fDigi->GetRpc() << fDigi->GetChannel() << " S "
               << fDigi->GetSide() << " : " << Form("T %15.3f, Tot %5.1f", fDigi->GetTime(), fDigi->GetTot());
  }
  vDigi.clear();
  //fUnpackerAlgo->ClearVector();

  return kTRUE;
}

void CbmDeviceUnpackTofCri::BuildTint(uint64_t ulTsStartTime, int iMode)
{
  // iMode - sending condition
  // 0 (default)- build time interval only if last buffer entry is older the start + TSLength
  // 1 (finish), empty buffer without checking
  // Steering variables
  double TSLENGTH    = 1.E6;
  double fdMaxDeltaT = (double) fiReqTint;  // in ns

  LOG(debug) << "BuildTint: Buffer size " << fBuffer->GetSize() << ", DeltaT "
             << (fBuffer->GetTimeLast() - fBuffer->GetTimeFirst()) / 1.E9 << " s";
  CbmTbDaqBuffer::Data data;
  CbmTofDigi* digi;

  while (fBuffer->GetSize() > 0) {
    Double_t fTimeBufferLast = fBuffer->GetTimeLast();

    switch (iMode) {
      case 0:
        if (fTimeBufferLast - fBuffer->GetTimeFirst() < TSLENGTH) {
          LOG(debug) << "BuildTint: refill DAQ buffer ";
          return;
        }
        break;
      case 1:; break;
    }

    data = fBuffer->GetNextData(fTimeBufferLast);
    digi = boost::any_cast<CbmTofDigi*>(data.first);
    assert(digi);

    Double_t dTEnd    = digi->GetTime() + fdMaxDeltaT;
    Double_t dTEndMax = digi->GetTime() + 2 * fdMaxDeltaT;
    LOG(debug) << Form("Next event at %f until %f, max %f ", digi->GetTime(), dTEnd, dTEndMax);

    if (dTEnd > fTimeBufferLast) {
      LOG(warn) << Form("Remaining buffer < %f with %d entries is not "
                        "sufficient for digi ending at %f -> skipped ",
                        fTimeBufferLast, fBuffer->GetSize(), dTEnd);
      return;
    }

    LOG(debug) << "BuildTint0 with digi " << Form("0x%08x at %012.2f", digi->GetAddress(), digi->GetTime());

    Bool_t bDet[fiReqDigiAddr.size()][2];
    for (UInt_t i = 0; i < fiReqDigiAddr.size(); i++)
      for (Int_t j = 0; j < 2; j++)
        bDet[i][j] = kFALSE;  //initialize
    Bool_t bPul[fiReqDigiAddr.size()][2];
    for (UInt_t i = 0; i < fiReqDigiAddr.size(); i++)
      for (Int_t j = 0; j < 2; j++)
        bPul[i][j] = kFALSE;  //initialize
    Bool_t bBeam = kFALSE;

    std::vector<CbmTofDigi*> vdigi;
    UInt_t nDigi = 0;
    //const Int_t AddrMask=0x003FFFFF;
    const Int_t AddrMask = 0x001FFFFF;
    Bool_t bOut          = kFALSE;
    Int_t iBucMul        = 0;

    while (data.second != ECbmModuleId::kNotExist) {  // build digi array
      digi = boost::any_cast<CbmTofDigi*>(data.first);
      LOG(debug) << "GetNextData " << digi << ", " << data.second << ",  " << Form("%f %f", digi->GetTime(), dTEnd)
                 << ", Mul " << nDigi;
      assert(digi);

      if (nDigi == vdigi.size()) vdigi.resize(nDigi + 100);
      vdigi[nDigi++] = digi;

      Int_t iAddr = digi->GetAddress() & AddrMask;
      if (iAddr == 0x00003006 || iAddr == 0x0000b006) {
        iBucMul++;
        LOG(debug) << Form("Event %10lu: BucMul %2d, addr 0x%08x, side %d, strip %2d, rpc %d", fEventHeader[0], iBucMul,
                           (uint) digi->GetAddress(), (Int_t) digi->GetSide(), (Int_t) digi->GetChannel(),
                           (int) digi->GetRpc());
      }
      for (UInt_t i = 0; i < fiReqDigiAddr.size(); i++)
        if ((digi->GetAddress() & AddrMask) == fiReqDigiAddr[i]) {
          Int_t j    = ((CbmTofDigi*) digi)->GetSide();
          bDet[i][j] = kTRUE;
          if (fiReqDigiAddr[i] == (Int_t) fiReqBeam) {
            bBeam = kTRUE;
            LOG(debug) << "Found ReqBeam at index " << nDigi - 1 << ", req " << i;
          }
          if ((UInt_t) fiReqDigiAddr[i] == fiAddrRef) bDet[i][1] = kTRUE;  // diamond with pad readout
          if ((fiReqDigiAddr[i] & 0x0000F00F) == 0x00004006) {             // pad counters v21a
            bDet[i][1] = kTRUE;
            LOG(debug) << Form("Pad counter 0x%08x found in ev %10lu", fiReqDigiAddr[i], fEventHeader[0]);
          }
          Int_t str = ((CbmTofDigi*) digi)->GetChannel();

          switch (j) {  // treat both strip ends separately
            case 0: {
              switch (fiPulserMode) {
                case 0:
                case 1:
                  if (str == 31)
                    if (digi->GetTot() > fiPulTotMin && digi->GetTot() < fiPulTotMax) bPul[i][0] = kTRUE;
                  if (str == 0) bPul[i][1] = kFALSE;
                  if ((UInt_t) fiReqDigiAddr[i] == fiAddrRef) {  //special mapping for MAr2019 diamond (Bmon)
                    if (str == 0) bPul[i][0] = kTRUE;
                    if (str == 40) bPul[i][1] = kTRUE;
                  }
                  break;
                case 2:
                  if (str == 0) {
                    if (digi->GetTot() > fiPulTotMin && digi->GetTot() < fiPulTotMax) {
                      bPul[i][0] = kTRUE;
                      if ((fiReqDigiAddr[i] & 0x000FF00F) == 0x00078006) {
                        bPul[i][1] = kTRUE;   // ceramic with pad readout
                        bDet[i][1] = kFALSE;  // remove Hit flag
                      }
                      if (str == 31) bPul[i][1] = kFALSE;
                    }
                  }
                default:;
              }
            } break;

            case 1:
              switch (fiPulserMode) {
                case 0:
                case 1:
                  if (str == 31) bPul[i][0] = kFALSE;
                  if (str == 0)
                    if (digi->GetTot() > fiPulTotMin && digi->GetTot() < fiPulTotMax) bPul[i][1] = kTRUE;
                  break;
                case 2:
                  if (str == 0) bPul[i][0] = kFALSE;
                  if (str == 31)
                    if (digi->GetTot() > fiPulTotMin && digi->GetTot() < fiPulTotMax) bPul[i][1] = kTRUE;
                  break;
                default:;
              }
              break;
            default:;
          }
        }
      //if(bOut) LOG(info)<<Form("Found 0x%08x, Req 0x%08x ", digi->GetAddress(), fiReqDigiAddr);
      if (dTEnd - digi->GetTime() < fdMaxDeltaT * 0.5) {
        if (digi->GetTime() + fdMaxDeltaT * 0.5 < dTEndMax) dTEnd = digi->GetTime() + fdMaxDeltaT * 0.5;
        else
          dTEnd = dTEndMax;
      };
      data = fBuffer->GetNextData(dTEnd);

    }  // end while

    LOG(debug) << Form(" %d digis associated to dTEnd = %15.9f", nDigi, dTEnd);
    //for(UInt_t iDigi=0; iDigi<nDigi; iDigi++) LOG(debug)<<Form(" 0x%08x",vdigi[iDigi]->GetAddress());
    //for (UInt_t iDigi = 0; iDigi < nDigi; iDigi++) LOG(debug) << vdigi[iDigi]->ToString();
    for (UInt_t iDigi = 0; iDigi < nDigi; iDigi++)
      LOG(debug) << "B " << iDigi << " TSRC " << vdigi[iDigi]->GetType() << vdigi[iDigi]->GetSm()
                 << vdigi[iDigi]->GetRpc() << vdigi[iDigi]->GetChannel() << " S " << vdigi[iDigi]->GetSide() << " : "
                 << Form("T %15.3f, Tot %5.1f", vdigi[iDigi]->GetTime(), vdigi[iDigi]->GetTot());

    UInt_t iDetMul = 0;
    if (fiReqDigiAddr.size() == 0) bOut = kTRUE;  // output everything
    else {
      if (fiReqMode == 0) {  // check for presence of requested detectors
        for (UInt_t i = 0; i < fiReqDigiAddr.size(); i++)
          if (bDet[i][0] == kFALSE || bDet[i][1] == kFALSE) break;
          else if (i == fiReqDigiAddr.size() - 1) {
            bOut    = kTRUE;
            iDetMul = i;
          }
      }
      else {  // check for presence of any known detector
        for (UInt_t i = 0; i < fiReqDigiAddr.size(); i++)
          if (bDet[i][0] == kTRUE && bDet[i][1] == kTRUE) { iDetMul++; }
        if (iDetMul >= fiReqMode) { bOut = kTRUE; }
      }
    }

    if (bOut && fiReqDigiAddr.size() > 1) {
      LOG(debug) << "Found Req coinc in event with " << nDigi << " digis in " << iDetMul
                 << " detectors, dTEnd = " << dTEnd;
    }

    // determine Pulser status
    UInt_t iPulMul = 0;  // Count Potential Pulser Signals
    for (UInt_t i = 0; i < fiReqDigiAddr.size(); i++) {
      if (bPul[i][0] == kTRUE && bPul[i][1] == kTRUE) iPulMul++;
    }

    if (fiPulserMode > 0 && iPulMul > fiPulMulMin) {
      LOG(debug) << "@Event " << fEventHeader[0] << ": iPulMul = " << iPulMul;
      bOut = kTRUE;
    }

    LOG(debug) << "Process Ev " << fEventHeader[0] << "  with iDetMul = " << iDetMul << ", iPulMul = " << iPulMul;

    fEventHeader[0]++;

    if ((Int_t) fiReqBeam > -1) {
      if (bBeam) { LOG(debug) << "Beam counter is present "; }
      else {
        LOG(debug) << "Beam counter is not present";
        bOut = kFALSE;  // request beam counter for event
      }
    }

    if (bOut) {
      fEventHeader[1] = iDetMul;
      fEventHeader[2] = fiReqMode;
      fEventHeader[3] = iPulMul;
      fEventHeader[4] = ulTsStartTime;  // PAL, 2022/07/05: no need to go to algo to get this value
      /*
      LOG(info) << "Sevt # " << fEventHeader[0]
				<<", DetMul "<< fEventHeader[1]
				<<", ReqMod "<< fEventHeader[2]
				<<", PulMul "<< fEventHeader[3]
				<<", TsStart "<< fEventHeader[4];
      */
      vdigi.resize(nDigi);
      const Int_t NDigiMax = 10000;  //FIXME constant number in code
      if (nDigi > NDigiMax) {
        LOG(warn) << "Oversized event " << fEventHeader[0] << ", size " << nDigi << " truncated! ";
        for (UInt_t iDigi = NDigiMax; iDigi < nDigi; iDigi++) {
          LOG(debug) << "Discard digi " << iDigi << ": " << vdigi[iDigi]->ToString();
          delete vdigi[iDigi];
        }
        nDigi = 1;  //NDigiMax;
        vdigi.resize(nDigi);
      }
      LOG(debug) << "Send " << nDigi << " digis to HitBuilder";
      SendDigis(vdigi, 0);

      for (UInt_t iDigi = 0; iDigi < nDigi; iDigi++)
        delete vdigi[iDigi];
    }
    else {
      LOG(debug) << " BuildTint cleanup of " << nDigi << " digis";
      for (UInt_t iDigi = 0; iDigi < nDigi; iDigi++) {
        delete vdigi[iDigi];
        //	vdigi[iDigi]->Delete();
      }
      LOG(debug) << " Digis deleted ";
      //vdigi.clear();
      //delete &vdigi;  // crashes, since local variable, will be done at return (?)
    }
  }
}

bool CbmDeviceUnpackTofCri::SendDigis(std::vector<CbmTofDigi*> vdigi, int idx)
{
  LOG(debug) << "Send Digis for event " << fNumTint << " with size " << vdigi.size() << Form(" at %p ", &vdigi);
  LOG(debug) << "EventHeader: " << fEventHeader[0] << " " << fEventHeader[1] << " " << fEventHeader[2] << " "
             << fEventHeader[3];

  //  Int_t NDigi=vdigi.size();

  std::stringstream ossE;
  boost::archive::binary_oarchive oaE(ossE);
  oaE << fEventHeader;
  std::string* strMsgE = new std::string(ossE.str());

  std::stringstream oss;
  boost::archive::binary_oarchive oa(oss);
  oa << vdigi;
  std::string* strMsg = new std::string(oss.str());

  FairMQParts parts;
  parts.AddPart(NewMessage(
    const_cast<char*>(strMsgE->c_str()),  // data
    strMsgE->length(),                    // size
    [](void*, void* object) { delete static_cast<std::string*>(object); },
    strMsgE));  // object that manages the data

  parts.AddPart(NewMessage(
    const_cast<char*>(strMsg->c_str()),  // data
    strMsg->length(),                    // size
    [](void*, void* object) { delete static_cast<std::string*>(object); },
    strMsg));  // object that manages the data


  /*
  std::vector<CbmTofDigi>vTofDigi;
  vTofDigi.resize(vdigi.size());
  for (Int_t i=0; i<vdigi.size(); i++)   {
    CbmTofDigi *pdigi = (CbmTofDigi *) vdigi[i];
    CbmTofDigi digi = *pdigi;
    vTofDigi[i] = digi;
    LOG(debug) << vTofDigi[i].ToString()<<" bits "<<Form("0x%08x",vTofDigi[i].TestBits(0xFFFF));
  }
    FairMQMessagePtr msg(NewMessage(static_cast<std::vector<CbmTofDigi>*> (&vTofDigi), // data
                                  NDigi*sizeof(CbmTofDigi), // size
				  [](void* , void* object){ delete static_cast<CbmTofDigi*>(object); }
                                  )); // object that manages the data

  // transfer of TofDigi array, ... works
  CbmTofDigi aTofDigi[NDigi];
  //  const Int_t iNDigiOut=100;
  //  NDigi=TMath::Min(NDigi,iNDigiOut);
  //  std::array<CbmTofDigi,iNDigiOut> aTofDigi;
  for (Int_t i=0; i<NDigi; i++) {
    aTofDigi[i] = *vdigi[i];
    LOG(debug) << aTofDigi[i].ToString()<<" bits "<<Form("0x%08x",aTofDigi[i].TestBits(0xFFFF));
  }
  FairMQMessagePtr msg(NewMessage(static_cast<CbmTofDigi*> (&aTofDigi[0]), // data
                                  NDigi*sizeof(CbmTofDigi), // size
				  [](void* , void* object){ delete static_cast<CbmTofDigi*>(object); }
                                  )); // object that manages the data


  LOG(info) << "Send aTofDigi sizes "<<NDigi<<", "<<sizeof(CbmTofDigi)<<", msg size "<<msg->GetSize();

  // serialize the timeslice and create the message

  std::stringstream oss;
  boost::archive::binary_oarchive oa(oss);
  oa << vdigi;
  std::string* strMsg = new std::string(oss.str());

  LOG(debug) << "send strMsg with length " << strMsg->length()<<" "<<strMsg;
  FairMQMessagePtr msg(NewMessage(const_cast<char*>(strMsg->c_str()), // data
                                                    strMsg->length(), // size
                                                    [](void* , void* object){ delete static_cast<std::string*>(object); },
                                                    strMsg)); // object that manages the data
  */
  /*
  FairMQMessagePtr msg(NewMessage(static_cast<CbmTofDigi*> (vTofDigi.data()), // data
                                                vTofDigi.size()*sizeof(CbmTofDigi), // size
                                                [](void* , void* object){ delete static_cast<CbmTofDigi*>(object); }
                                                )); // object that manages the data
  */

  /* --------------------------------------- compiles but crashes .... ---------------------------------------------------
  const Int_t WSize=8;
  FairMQMessagePtr msg(NewMessage(static_cast<std::vector<CbmTofDigi>*> (&vTofDigi), // data
                                  vTofDigi.size()*sizeof(CbmTofDigi)*WSize, // size, FIXME, numerical value in code!
				  [](void* , void* object){ delete static_cast<std::vector<CbmTofDigi>*>(object); }
                                  )); // object that manages the data

  LOG(info) << "Send TofDigi sizes "<<vTofDigi.size()<<", "<<sizeof(CbmTofDigi)<<", msg size "<<msg->GetSize();
  int *pData = static_cast <int *>(vTofDigi.data());

  int *pData = static_cast <int *>(msg->GetData());
  const Int_t NBytes=4;
  for (int iData=0; iData<msg->GetSize()/NBytes; iData++) {
    LOG(info) << Form(" ind %d, poi %p, data: 0x%08x",iData,pData,*pData++);
  }
  */
  /*
    auto msg = NewMessageFor("my_channel", 0,
                         static_cast<void*>(vTofDigi.data()),
                         vTofDigi.size() * sizeof(CbmTofDigi),
			 FairMQNoCleanup, nullptr);
  */

  // TODO: Implement sending same data to more than one channel
  // Need to create new message (copy message??)
  /*
  if (fChannelsToSend[idx]>1) {
    LOG(info) << "Need to copy FairMessage ?";
  }
  */
  // in case of error or transfer interruption,
  // return false to go to IDLE state
  // successfull transfer will return number of bytes
  // transfered (can be 0 if sending an empty message).

  LOG(debug) << "Send data to channel " << idx << " " << fChannelsToSend[idx][0];


  //  if (Send(msg, fChannelsToSend[idx][0]) < 0) {
  if (Send(parts, fChannelsToSend[idx][0]) < 0) {
    LOG(error) << "Problem sending data " << fChannelsToSend[idx][0];
    return false;
  }

  LOG(debug) << "Sent event # " << fNumTint << " from " << vdigi.size() << " Digis at " << vdigi.data() << ", "
             << &vdigi;

  fNumTint++;
  //if(fNumTint==100) FairMQStateMachine::ChangeState(PAUSE); //sleep(10000); // Stop for debugging ...
  /*
  LOG(info) << "Send message " << fNumTint << " with a size of "
            << msg->GetSize();
  */
  return true;
}

void CbmDeviceUnpackTofCri::AddReqDigiAddr(Int_t iAddr)
{
  UInt_t iNReq = fiReqDigiAddr.size();
  for (UInt_t i = 0; i < iNReq; i++)
    if (fiReqDigiAddr[i] == iAddr) return;  // det already present, avoid double counting
  fiReqDigiAddr.resize(iNReq + 1);          // hopefully the old entries are preserved ...
  fiReqDigiAddr[iNReq] = iAddr;
  LOG(info) << Form("Request Digi Address 0x%08x at index %d", iAddr, iNReq);
}

void CbmDeviceUnpackTofCri::SetIgnoreOverlapMs(Bool_t bFlagIn) { fUnpackerAlgo->SetDoIgnoreOverlappMs(bFlagIn); }

//void CbmDeviceUnpackTofCri::SetTimeOffsetNs(Double_t dOffsetIn) { fUnpackerAlgo->SetTimeOffsetNs(dOffsetIn); }
//void CbmDeviceUnpackTofCri::SetDiamondDpbIdx(UInt_t uIdx) { fUnpackerAlgo->SetDiamondDpbIdx(uIdx); }
