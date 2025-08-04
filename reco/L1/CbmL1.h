/* Copyright (C) 2006-2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Ivan Kisel,  Sergey Gorbunov, Maksym Zyzak, Valentina Akishina, Igor Kulakov, Denis Bertini [committer], Sergei Zharko */

/*
 *====================================================================
 *
 *  CBM Level 1 Reconstruction
 *
 *  Authors: I.Kisel,  S.Gorbunov
 *
 *  e-mail : ikisel@kip.uni-heidelberg.de
 *
 *====================================================================
 *
 *  CbmL1 header file
 *
 *====================================================================
 */

#ifndef _CbmL1_h_
#define _CbmL1_h_

#include "CaDataManager.h"
#include "CaFramework.h"
#include "CaInitManager.h"
#include "CaMonitor.h"
#include "CaVector.h"
#include "CbmCaMCModule.h"
#include "CbmCaTimeSliceReader.h"
#include "CbmL1DetectorID.h"
#include "CbmL1Hit.h"
#include "CbmL1MCPoint.h"
#include "CbmL1MCTrack.h"
#include "CbmL1Track.h"
#include "FairDetector.h"
#include "FairRootManager.h"
#include "FairTask.h"
#include "KfMaterialMonitor.h"
#include "KfTrackParam.h"
#include "TClonesArray.h"
#include "TH1.h"

#include <Logger.h>

#include <boost/functional/hash.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string_view>
#include <unordered_map>
#include <utility>

class CbmL1MCTrack;
class CbmMCDataObject;
class CbmEvent;
class TProfile2D;
class TNtuple;
class TGeoNode;


namespace
{
  namespace ca = cbm::algo::ca;
  namespace kf = cbm::algo::kf;
}

/// Internal structure to handle link keys
struct CbmL1LinkKey {
  /// Constructor from links
  CbmL1LinkKey(int32_t index, int32_t entry, int32_t file) : fIndex(index), fEntry(entry), fFile(file) {}
  bool operator==(const CbmL1LinkKey& other) const
  {
    return fFile == other.fFile && fEntry == other.fEntry && fIndex == other.fIndex;
  }

  int32_t fIndex = -1;  ///< index of point/track, saved to link
  int32_t fEntry = -1;  ///< index of link entry
  int32_t fFile  = -1;  ///< index of link file
};

/// Hash for CbmL1LinkKey
namespace std
{
  template<>
  struct hash<CbmL1LinkKey> {
    std::size_t operator()(const CbmL1LinkKey& key) const
    {
      std::size_t res = 0;
      boost::hash_combine(res, key.fFile);
      boost::hash_combine(res, key.fEntry);
      boost::hash_combine(res, key.fIndex);
      return res;
    }
  };
}  // namespace std


// TODO: insert documentation! (S.Zh.)
//
/// ca::Framework runtime constants modification can be performed in run_reco.C. Example:
///
///   l1->GetInitManager()->GetParameters()->SetMaxDoubletsPerSinglet(149);
/// TODO: L1InitManager - main interface of communication between cbmroot/bmnroot and ca::Framework (S.Zharko)
///
class CbmL1 : public FairTask {
 public:
  // **********************
  // ** Types definition **
  // **********************

  using DFSET = std::set<std::pair<int, int>>;

  // **************************
  // ** Friends classes list **
  // **************************

  friend class L1AlgoDraw;
  friend class L1AlgoPulls;

  template<int NHits>
  friend class L1AlgoEfficiencyPerformance;

  friend class CbmL1MCTrack;
  friend class CbmL1PFFitter;

  enum class EInitMode
  {
    Full,  ///< Full initialization (default)
    Param  ///< Parameter initialization (when algorithm execution is not required)
  };

  // **********************************
  // ** Member functions declaration **
  // **********************************

  // ** Constructors and destructor **

  /// Default constructor
  CbmL1();

  /// Constructor from parameters
  /// \param  name              Name of the task
  /// \param  verbose           Verbosity level
  /// \param  performance       Performance run flag:
  ///                           - #0 run without performance measurement
  ///                           - #1 require 4 consecutive hits on reconstructible mc track
  ///                           - #2 require 4 hits on reconstructible mc track
  ///                           - #3 require 4 consecutive mc points on reconstructible mc track
  CbmL1(const char* name, Int_t verbose = 1, Int_t performance = 0);

  /// Copy constructor
  CbmL1(const CbmL1&) = delete;

  /// Move constructor
  CbmL1(CbmL1&&) = delete;

  /// Copy assignment operator
  CbmL1& operator=(const CbmL1&) = delete;

  /// Move assignment operator
  CbmL1& operator=(CbmL1&&) = delete;

  /// Destructor
  ~CbmL1();

  /// Pointer to CbmL1 instance
  static CbmL1* Instance() { return fpInstance; }

  // ** Member functions override from FairTask **

  /// Defines action in the beginning of the run (initialization)
  virtual InitStatus Init();

  /// Reruns the initialization
  virtual InitStatus ReInit();

  /// Defines action in the end of the run (saves results)
  void Finish();

  /// \brief Gets reference to MC data object
  const cbm::ca::tools::MCData& GetMCData() const { return fMCData; }

  // ** Specific member functions **

  /// @brief  Disables tracking station for a given detector subsystem
  /// @param  detID  Detector ID
  /// @param  iSt    Index of station in tracking station interface
  ///
  /// Possible entries of Detector ID are listed in CbmL1DetectorID.h.
  void DisableTrackingStation(ca::EDetectorID detID, int iSt);

  /// \brief Sets initialization mode
  /// \param mode  A CbmL1::EInitMode enumeration entry
  void SetInitMode(EInitMode mode) { fInitMode = mode; }

  /// Sets safe but slow material initialisation to get around potential problems in TGeoVoxelFinder
  /// (use it only in case of a crash at the initialisation of material maps)
  void SetSafeMaterialInitialization(bool val = true) { fDoSafeMaterialInitialization = val; }

  /// Sets material budget binning
  void SetMaterialBudgetNbins(int nBinsPerDimension) { fMatBudgetNbins = nBinsPerDimension; }

  /// Sets material budget n rays per dimansion in each bin
  void SetMaterialBudgetNrays(int nRaysPerDimension) { fMatBudgetNrays = nRaysPerDimension; }

  /// Sets material budget minimal bin size in cm
  void SetMaterialBudgetPitch(double pitchCm) { fMatBudgetPitch = pitchCm; }

  /// Calculate material budget with rays, parallel to Z axis
  /// Only needed in debug mode to produce detailed picture of the material
  void SetMaterialBudgetParallelProjection() { fMatBudgetParallelProjection = true; }

  /// @brief Sets user configuration filename
  /// @param path  Path to the config
  void SetConfigUser(const char* path) { fInitManager.SetConfigUser(path); }

  /// \brief Sets a name for the input search window file
  /// If the filename is not defined, a default track finder with Kalman filter is used. Otherwise, an experimental
  /// version of track finder with estimated hit search windows is utilised
  /// \param filename Name of the input search window file
  void SetInputSearchWindowFilename(const char* filename) { fsInputSearchWindowsFilename = filename; }

  /// \brief Sets a name for the output configuration file
  /// \param filename  Name of the input tracking configuration file
  void SetOutputConfigName(const char* filename) { fInitManager.SetOutputConfigName(std::string(filename)); }

  /// \brief Sets a name for the ca-parameter file
  /// \param filename  Name of the parameter file
  void SetParameterFilename(const char* filename) { fSTAPParamFile = TString(filename); }

  /// \brief Sets output file for MC triplets tree
  /// If the filename is empty string, tree is not filled
  /// \param filename Name of the output file name
  void SetOutputMcTripletsTreeFilename(const char* filename) { fsMcTripletsOutputFilename = std::string(filename); }

  /// Gets vector of the extended QA hits
  /// TODO: It is a temporary function, do not rely on it
  const auto& GetQaHits() const { return fvHitDebugInfo; }

  /// Utility to map the L1DetectorID items into detector names
  static constexpr const char* GetDetectorName(ca::EDetectorID detectorID)
  {
    switch (detectorID) {
      case ca::EDetectorID::kMvd: return "MVD";
      case ca::EDetectorID::kSts: return "STS";
      case ca::EDetectorID::kMuch: return "MuCh";
      case ca::EDetectorID::kTrd: return "TRD";
      case ca::EDetectorID::kTof: return "TOF";
      case ca::EDetectorID::END: break;
    }
    return "";
  }

  void SetExtrapolateToTheEndOfSTS(bool b) { fExtrapolateToTheEndOfSTS = b; }
  void SetStsOnlyMode() { fTrackingMode = ca::TrackingMode::kSts; }
  void SetMcbmMode() { fTrackingMode = ca::TrackingMode::kMcbm; }
  void SetGlobalMode() { fTrackingMode = ca::TrackingMode::kGlobal; }

  ca::TrackingMonitor fMonitor{};  ///< Tracking monitor

  //   void SetTrackingLevel( Int_t iLevel ){ fTrackingLevel = iLevel; }
  //   void MomentumCutOff( Double_t cut ){ fMomentumCutOff = cut; }
  //   void SetDetectorEfficiency( Double_t eff ){ fDetectorEfficiency = eff; }

  /// Reconstructs tracks in a given event
  /// \param event  Pointer to current CbmEvent object
  void Reconstruct(CbmEvent* event = nullptr);

  static double boundedGaus(double sigma);

  /// Obsolete setters to be removed

  /// Sets material budget file name for MVD
  void SetMvdMaterialBudgetFileName(const TString&)
  {
    LOG(warning) << "CbmL1::SetMvdMaterialBudgetFileName() is obsolete and will be deleted soon, don't call it\n"
                 << "   !  The material budget files are not used anymore  !";
  }

  /// Sets material budget file name for STS
  void SetStsMaterialBudgetFileName(const TString&)
  {
    LOG(warning) << "CbmL1::SetStsMaterialBudgetFileName() is obsolete and will be deleted soon, don't call it\n"
                 << "   !  The material budget files are not used anymore  !";
  }

  /// Sets material budget file name for MuCh
  void SetMuchMaterialBudgetFileName(const TString&)
  {
    LOG(warning) << "CbmL1::SetMuchMaterialBudgetFileName() is obsolete and will be deleted soon, don't call it\n"
                 << "   !  The material budget files are not used anymore  !";
  }

  /// Sets material budget file name for TRD
  void SetTrdMaterialBudgetFileName(const TString&)
  {
    LOG(warning) << "CbmL1::SetTrdMaterialBudgetFileName() is obsolete and will be deleted soon, don't call it\n"
                 << "   !  The material budget files are not used anymore  !";
  }

  /// Sets material budget file name for TOF
  void SetTofMaterialBudgetFileName(const TString&)
  {
    LOG(warning) << "CbmL1::SetTofMaterialBudgetFileName() is obsolete and will be deleted soon, don't call it\n"
                 << "   !  The material budget files are not used anymore  !";
  }

 private:
  struct TH1FParameters {
    TString name, title;
    int nbins;
    float xMin, xMax;
  };

  /// \brief Runs ideal track finder: copies all MC-tracks into reconstructed tracks
  void IdealTrackFinder();

  /// \brief Fills the fvMCTracks vector and the fmMCTracksMap
  void Fill_vMCTracks();

  /*
   * Input Performance
   */

  /// \brief Matches hit with MC point
  /// \tparam  DetId Detector ID
  /// \param   iHit  External index of hit
  /// \return        MC-point index in fvMCPoints array
  template<ca::EDetectorID DetId>
  std::tuple<int, std::vector<int>> MatchHitWithMc(int iHit) const;

  void FieldApproxCheck();    // Build histograms with difference between Field map and approximated field
  void FieldIntegralCheck();  // Build 2D histogram: dependence of the field integral on phi and theta
  void TimeHist();

  // ********************************
  // ** Reconstruction Performance **
  // ********************************

  /// @brief  Sets random seed to CA internal random generator
  /// @param  seed  Random seed
  void SetRandomSeed(unsigned int seed) { fInitManager.SetRandomSeed(seed); }

  /// Calculates tracking efficiencies (counters)
  void EfficienciesPerformance(bool doFinish = kFALSE);

  /// Builds pulls and residuals
  /// \note Should be called only after CbmL1::Performance()
  void TrackFitPerformance();

  void FillFitHistos(cbm::algo::kf::TrackParamV& tr, const cbm::ca::tools::MCPoint& mc, bool isTimeFitted, TH1F* h[]);

  /// Fills performance histograms
  void HistoPerformance();

  /// Writes MC triplets to tree
  /// \note Executed only if the filename for MC tracks ntuple output is defined
  void DumpMCTripletsToTree();


  // ** STandAlone Package service-functions **

  /// @brief Defines names for output in STAP mode
  /// @param dirName  Name of output directory for STAP data
  ///
  /// Defines the name of input/output directory [dir] and prefix of the files [pref], which is used to define
  /// input and output data trees in the reconstruction macro. If the output TTree file has name
  /// /path/to/[pref].reco.root, the data files will be:
  ///   [dir]/input_hits/[pref].job[No].ca.input.dat - hits input files, containing serialized ca::InputData objects,
  ///     stored for each job (each call of CbmL1::ReadEvent function)
  ///   [dir]/[pref].ca.par - parameters input files, containing serialized L1Parameters object
  void DefineSTAPNames(const char* dirName);

  /// Writes initialized L1Parameters object to file ""
  void WriteSTAPParamObject();

  /// Writes a sample of an ca::InputData object to defined directory fSTAPDataDir
  /// \param iJob  Number of job, usually is defined by the nCalls of executing function
  /// \note  Creates a file fSTAPDataDir + "/" + fSTAPDataPrefix + "." + TString::Format(kSTAPAlgoIDataSuffix, iJob)
  void WriteSTAPAlgoInputData(int iJob = 0);

  void WriteSTAPPerfInputData();

  /// Reads a sample of an ca::InputData object from defined directory fSTAPDataDir
  /// \param iJob  Number of job, usually is defined by the nCalls of executing function
  /// \note  Reads from a file fSTAPDataDir + "/" + fSTAPDataPrefix + "." + TString::Format(kSTAPAlgoIDataSuffix, iJob)
  void ReadSTAPParamObject();

  void ReadSTAPAlgoInputData(int iJob = 0);

  void ReadSTAPPerfInputData();

  /// Gets a pointer to L1InitManager (for an access in run_reco.C)
  ca::InitManager* GetInitManager() { return &fInitManager; }

  void WriteHistosCurFile(TObject* obj);

  static std::istream& eatwhite(std::istream& is);  // skip spaces
  static void writedir2current(TObject* obj);       // help procedure

  void DumpMaterialToFile(TString fileName);

 private:
  // ***************************
  // ** Member variables list **
  // ***************************

  std::string fInputDataFilename           = "";  ///< File name to read/write input hits
  std::string fsInputSearchWindowsFilename = "";  ///< File name to read search windows

  std::unique_ptr<cbm::ca::TimeSliceReader> fpTSReader = nullptr;  ///< event/TS reader
  std::unique_ptr<cbm::ca::MCModule> fpMCModule        = nullptr;  ///< MC module
  cbm::ca::tools::MCData fMCData;                                  ///< MC Data object

  ca::InitManager fInitManager;                                ///< Tracking parameters data manager
  std::shared_ptr<ca::DataManager> fpIODataManager = nullptr;  ///< Input-output data manager

 public:
  // ** Basic data members **

  ca::Framework* fpAlgo = nullptr;  ///< Pointer to the L1 track finder algorithm

  ca::TrackingMode fTrackingMode = ca::TrackingMode::kSts;  ///< Tracking mode

  ca::Vector<CbmL1Track> fvRecoTracks = {"CbmL1::fvRecoTracks"};  ///< Reconstructed tracks container

 private:
  static CbmL1* fpInstance;  ///< Instance of CbmL1

  bool fDoSafeMaterialInitialization{false};  /// Do safe but slow material initialisation
                                              /// to get around potential problems in TGeoVoxelFinder

  double fTargetX{1.e10};  ///< target position X
  double fTargetY{1.e10};  ///< target position Y
  double fTargetZ{1.e10};  ///< target position Z

  int fNStations = 0;  ///< number of total active detector stations

  Int_t fPerformance   = 0;   ///< performance mode: 0 - w\o perf. 1 - L1-Efficiency definition. 2 - QA-Eff.definition
  double fTrackingTime = 0.;  ///< time of track finding procedure

  /// Option to work with files for the standalone mode
  ///  - #0 standalone mode is not used
  ///  - #1 data for standalone mode is written to configuration file (currently does not work)
  ///  - #2 tracking runs in standalone mode using configuration file (currently does not work)
  ///  - #3 data is written and read (currently does not work)
  ///  - #4 parameter file is saved, but the data does not
  ///
  int fSTAPDataMode = 4;  ///< Option to work with files for the standalone mode

  TString fSTAPDataDir    = ".";     ///< Name of input/output directory for running in a STAP mode
  TString fSTAPDataPrefix = "test";  ///< Name of input/output file prefix. The prefix is defined by output TTree file
  TString fSTAPParamFile  = "";      ///< Name of the parameter file (generated automatically, if not provided manually)

  /// Extension for IO of the L1Parameters object
  static constexpr std::string_view kSTAPParamSuffix = "ca.par";
  static constexpr std::string_view kSTAPSetupSuffix = "kf.setup";

  /// Extension for IO of the ca::InputData object
  /// \note IO of the ca::InputData object is called inside every launch of CbmL1::ReadEvent function. Inside the function
  ///       there is a static counter, which calculates the job (function call) number. One have to define the name of
  ///       the kSTAPAlgoIDataSuffix containing '%d' control symbol, which is replaced with the current job number.
  ///       \example The file name with [pref] = auau.mbias.eb.100ev and [suff] = "job%d.ca.input.dat" for the job
  ///       number 10 is auau.mbias.eb.100ev.job10.ca.input.dat
  static constexpr std::string_view kSTAPAlgoIDataSuffix = "job%d.ca.input.dat";

  /// Name of subdirectory for handling ca::InputData objects
  static constexpr std::string_view kSTAPAlgoIDataDir = "input_hits";


  Int_t fTrackingLevel     = 2;    // currently not used
  Double_t fMomentumCutOff = 0.1;  // currently not used

  int fEventNo       = 0;  ///< Current number of event/TS
  int fNofRecoTracks = 0;  ///< Total number of reconstructed tracks

 public:
  // ** Repacked input data **

  ca::Vector<CbmL1HitId> fvExternalHits = {"CbmL1::fvExternalHits"};  ///< Array of hits

 private:
  ca::Vector<CbmL1HitDebugInfo> fvHitDebugInfo = {
    "CbmL1::fvHitDebugInfo"};  ///< Container of hits with extended information
  // indices of MCPoints in fvMCPoints, indexed by index of hit in algo->vHits array. According to StsMatch. Used for IdealResponce
  //    ca::Vector<int>          vHitMCRef1;
  //   CbmMatch HitMatch;

  cbm::ca::DetIdArr_t<std::set<int>> fvmDisabledStationIDs;  /// Array of local indices of disabled tracking stations

  // *****************************
  // ** Tracking performance QA **
  // *****************************
  // TODO: move to a separate class (S.Zharko)
  TFile* fPerfFile{nullptr};
  TDirectory* fHistoDir{nullptr};
  TDirectory* fTableDir{nullptr};

  static const int fNTimeHistos = 22;
  TH1F* fTimeHisto[fNTimeHistos]{nullptr};

  static const int fNGhostHistos = 9;
  TH1F* fGhostHisto[fNGhostHistos]{nullptr};

  TFile* fpMcTripletsOutFile             = nullptr;  ///< File to save MC-triplets tree
  TTree* fpMcTripletsTree                = nullptr;  ///< Tree to save MC-triplets
  std::string fsMcTripletsOutputFilename = "";       ///< Name of file to save MC-triplets tree

  int fMatBudgetNbins{100};     ///< n bins in mat budget maps (fMatBudgetNbins x fMatBudgetNbins)
  int fMatBudgetNrays{3};       ///< material budget n rays per dimansion in each bin
  double fMatBudgetPitch{0.1};  ///< material budget minimal bin size in cm

  EInitMode fInitMode = EInitMode::Full;  ///< Initialization mode

  bool fMatBudgetParallelProjection{false};  ///< Calculate material budget with rays, parallel to Z axis
                                             ///< Only needed in debug mode to produce detailed picture of the material

  bool fExtrapolateToTheEndOfSTS{false};

  std::vector<cbm::algo::kf::MaterialMonitor> fMaterialMonitor{};  ///< Material monitors for each material budget map

  ClassDef(CbmL1, 0);
};


#endif  //_CbmL1_h_
