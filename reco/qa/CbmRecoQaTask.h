/* Copyright (C) 2024 Hulubei National Institute of Physics and Nuclear Engineering - Horia Hulubei, Bucharest
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Alexandru Bercuci [committer], Omveer Singh */

#ifndef CBMRECOQATASK_H
#define CBMRECOQATASK_H 1

#include "CbmKfTrackFitter.h"
#include "CbmTrack.h"

#include <FairTask.h>

#include <TDirectoryFile.h>
#include <TGeoNode.h>

#include <bitset>
#include <map>
#include <vector>

class FairMCPoint;
class CbmEvent;
class CbmHit;
class CbmMCDataManager;
class CbmMCDataArray;
class CbmTimeSlice;
class TClonesArray;
class TH2;
class TH2D;
class TVector3;
class CbmRecoQaTask : public FairTask {
 public:
  enum eRecoConfig
  {
    kRecoEvents = 0,  /// has events reconstructed (CbmEvent branch)
    kRecoTracks,      /// has tracks reconstructed (GlobalTrack branch)
    kStsHits,         /// has STS hits (StsHit branch)
    kTrdHits,         /// has TRD` hits (TrdHit branch)
    kTofHits,         /// has ToF hits (TofHit branch)
    kRichHits,        /// has Rich hits (RichHit branch)
    kMuchHits,        /// has Much hits (MuchHit branch)
    kUseMC,           /// use MC even if available
    kRecoQaNConfigs   /// no of configuration flags
  };
  enum eSetup
  {
    kMcbm22 = 0,
    kMcbm24,
    kDefault
  };
  enum class eViewType : int
  {
    kDetUnit = 0,  /// detector view
    kTrkProj,      /// set of track projection views
    kPV            /// primary vertex view
  };

#define kNtrkProjections 6
  enum class eProjectionType : int
  {
    kXYa = 0,     /// X-Y hit coorelation in track filtered data
    kXYp,         /// X-Y track projections on detection unit
    kXdX,         /// X to TRK residuals as function of local X in view
    kXdXMC,       /// X to TRK residuals w.r.t MC points
    kYdY,         /// Y to TRK residuals as function of local Y in view
    kYdYMC,       /// Y to TRK residuals w.r.t MC points
    kWdT,         /// Time to TRK residuals as function of local high resolution
                  /// coordinate in view
    kXpX,         /// X to TRK pulls as function of local X in view
    kYpY,         /// Y to TRK pulls as function of local Y in view
    kChdT,        /// Time to EV residuals as function of coordinate in view
    kXYh,         /// X-Y hit coorelation in local view
    kDmult,       /// local view hit multiplicity
    kDmultMC,     /// local view MC point multiplicity
    kXYhMC,       /// X-Y MC point coorelation in local view (using HitMatch)
    kPullX,       /// Pull distribution X: (RC - MC) / dx_RC
    kPullY,       /// Pull distribution Y:
    kResidualX,   /// Residual distribution X: (x_RC - x_MC) in cm
    kResidualY,   /// Residual distribution Y:
    kResidualTX,  /// Residual distribution T:
    kResidualTY,  /// Residual distribution T:
    kPVxy,        /// x-y projection of the primary vertex:
    kPVxz,        /// x-z projection of the primary vertex:
    kPVyz,        /// y-z projection of the primary vertex:
    kPVmult,      /// y-z projection of the primary vertex:
    kXYt0,        /// X-Y track projections on a random plane (value 0)
    kXYt1,        /// X-Y track projections on a random plane (value 1)
    kXYt2,        /// X-Y track projections on a random plane (value 2)
    kXYt3,        /// X-Y track projections on a random plane (value 3)
    kXYt4,        /// X-Y track projections on a random plane (value 4)
    kXYt5         /// X-Y track projections on a random plane (value 5)
  };

  // Generic view definition
  struct Detector;
  struct View {
    friend struct Detector;
    friend class CbmRecoQaTask;

    View() = default;
    View(const char* n, const char* p, std::vector<int> set) : name(n), path(p), fSelector(set) { ; }
    virtual ~View() = default;
    bool SetProjection(eProjectionType prj, float range, const char* unit);
    template<class Hit>
    bool HasAddress(const CbmHit* h, double& x, double& y, double& dx, double& dy) const;
    template<class Hit>
    bool Load(const CbmHit* h, const FairMCPoint* point, const CbmEvent* ev);
    bool Load(const CbmKfTrackFitter::TrajectoryNode* n, const FairMCPoint* point);
    bool Load(TVector3* p);
    std::string ToString() const;
    static std::string ToString(eProjectionType prj);
    void SetSetup(CbmRecoQaTask::eSetup setup) { fSetup = setup; }
    void SetType(CbmRecoQaTask::eViewType type) { fType = type; }

    std::string name           = "";    /// name describing the module
    std::string path           = "";    /// path to the geo volume describing the module
    double size[3]             = {0.};  /// detection element geometrical dx dy dz dimmensions
    double pos[3]              = {0.};  /// detection element center x0 y0 z0
    std::vector<int> fSelector = {};    /// defining subset of the address set for this view
    std::map<eProjectionType, std::tuple<int, float, TH2*>> fProjection =
      {};                         /// map of projections indexed by their type. Each projection
                                  /// contains also the relative scale [int] wrt to default units
                                  /// (ns, cm, keV) and the range [float].
    int fMult               = 0;  /// multiplicity between 2 reset signals
    mutable eSetup fSetup   = eSetup::kMcbm22;
    mutable eViewType fType = eViewType::kDetUnit;

   protected:
    bool AddProjection(eProjectionType prj, float range = -1, const char* unit = "cm");
    /** \brief Define all type of QA histo known to the class.
      * In case of detector view type, convert geo address into pointer to geometry.
      * By the time of the call the geometry has to be available in memory.
      * Failing to identify the named physical node will rezult in error */
    bool Init(const char* dname, bool mc = false);
    /** \brief build directory structure for all projections of current
      * view.*/
    uint Register(TDirectoryFile* f);
    /** \brief helper functions to estimate the representation (y) axis
      * \param[in] scale read-only unit defining parameter
      * \param[in] range read-write full range on the ordinate
      * \return units for the ordinate and the range value
      */
    std::string makeTrange(const int scale, float& range);
    std::string makeYrange(const int scale, float& range);
    ClassDef(CbmRecoQaTask::View,
             1);  // Stand-alone detection set to which QA is applied
  };              // QA View definition

  // Detector unit definition
  struct Detector {
    struct Data {
      Data() = default;
      Data(ECbmDataType i, const char* n) : id(i), name(n) { ; }
      ECbmDataType id  = ECbmDataType::kUnknown;
      std::string name = "nn";
    };  // Identifier of data to be monitored

    Detector(ECbmModuleId did = ECbmModuleId::kNotExist);
    virtual ~Detector() = default;
    View* AddView(const char* n, const char* p, std::vector<int> set);
    View* GetView(const char* n);
    View* FindView(double x, double y, double z);
    /** \brief Check geometry and trigger Init() for all registered views. Build
     * main directory outut for the current detector. Failing to identify the
     * geometry will rezult in fatal error. \return true If ALL the subsequent
     * calls to Init result in a true */
    bool Init(TDirectoryFile* f, bool mc = false);
    void Print() const;

    ECbmModuleId id;
    Data hit, point;
    Data trk;
    std::vector<View> fViews = {};

    ClassDef(CbmRecoQaTask::Detector,
             1);  // QA representation of a detector unit
  };              // Detection system agregate

  struct TrackFilter {
    enum class eTrackCut : int
    {
      kSts = 0,  /// cut on no of STS hits / track
      kMuch,     /// cut on no of Much hits / track
      kRich,     /// cut on no of Rich hits / track
      kTrd,      /// cut on no of TRD hits / track
      kTof,      /// cut on no of Tof hits / track
      kNone      /// no cut
    };
    TrackFilter(eTrackCut typ = eTrackCut::kNone) : fType(typ) { ; }
    virtual ~TrackFilter() = default;
    bool Accept(const CbmGlobalTrack* ptr, const CbmRecoQaTask* lnk);
    bool SetFilter(std::vector<float> cuts);
    std::string ToString() const;

    eTrackCut fType = eTrackCut::kNone;

   private:
    // STS definition of track
    int fNSts                  = -1;
    std::vector<bool> fStsHits = {};
    // TRD definition of track
    int fNTrd = -1;
    // ToF definition of track
    int fNTof = -1;
    ClassDef(CbmRecoQaTask::TrackFilter, 1);  // Track cut implementation
  };                                          // TrackFilter definition

  struct EventFilter {
    enum class eEventCut : int
    {
      kMultTrk = 0,  /// cut on track multiplicity
      kMultHit,      /// cut on hit multiplicity
      kTrigger,      /// cut on trigger conditions
      kVertex,       /// cut on vertex definition
      kNone          /// no cut
    };
    enum class eEventDef : int
    {
      kNofDetHit = 3  /// no of detectors considered for the hit cut [STS TRD ToF]
    };
    EventFilter(eEventCut typ = eEventCut::kNone) : fType(typ) { ; }
    virtual ~EventFilter() = default;
    bool Accept(const CbmEvent* ptr, const CbmRecoQaTask* lnk);
    bool SetFilter(std::vector<float> cuts);
    std::string ToString() const;

    eEventCut fType = eEventCut::kNone;

   private:
    // track multiplicity cut definition
    int fMinTrack = 0;
    int fMaxTrack = 0;
    // hit multiplicity cut definition
    int fMultHit[(int) eEventDef::kNofDetHit] = {0};  /// max no of hits/ev for the systems [STS TRD ToF]

    /** \brief Helper function : display usage message for each filter case*/
    void HelpMess() const;

    ClassDef(CbmRecoQaTask::EventFilter, 1);  // Event cut implementation
  };                                          // EventFilter definition

 public:
  CbmRecoQaTask();
  virtual ~CbmRecoQaTask() { ; }
  /** Copy the qa test defined for detector det from the steering macro to the
   * current class */
  virtual Detector* AddDetector(ECbmModuleId did);
  virtual EventFilter* AddEventFilter(EventFilter::eEventCut cut);
  virtual TrackFilter* AddTrackFilter(TrackFilter::eTrackCut cut);
  virtual Detector* GetDetector(ECbmModuleId did);
  /** \brief Retrieve detector specific track by index.
   * \param[in] did system Identifier
   * \param[in] id track id in the TClonesArray
   * \return pointer to track or nullptr if track/system not available
   */
  virtual const CbmTrack* GetTrack(ECbmModuleId did, int id) const;
  /** \brief Perform initialization of data sources and projections */
  virtual InitStatus Init();
  /** \brief Executed task **/
  virtual void Exec(Option_t* option);

  virtual void Finish();

  // /** \brief Define the set of extra z positions where the track should be
  //  * projected in the x-y plane */
  // void SetProjections(std::vector<double> vzpj);
  void SetSetupClass(CbmRecoQaTask::eSetup setup) { fSetupClass = setup; }
  TString GetGeoTagForDetector(const TString& detector);
  std::vector<TString> GetPath(TGeoNode* node, TString, TString activeNodeName, int depth = 0,
                               const TString& path = "");
  void UseMC(bool set = true)
  {
    if (set) fuRecoConfig.set(kUseMC);
  }

 protected:
  static std::bitset<kRecoQaNConfigs> fuRecoConfig;

 private:
  CbmRecoQaTask(const CbmRecoQaTask&);
  CbmRecoQaTask& operator=(const CbmRecoQaTask&);

  /** \brief Filter events for QA use (e.g. event multiplicity)
   * \param[in] ptr cbm event
   * \return true if event accepted
   */
  virtual bool FilterEvent(const CbmEvent* ptr);
  /** \brief Filter tracks for further use (e.g. track projections)
   * \param[in] ptr global track
   * \return true if track accepted
   */
  virtual bool FilterTrack(const CbmGlobalTrack* ptr);
  /** \brief count views types registered with the task */
  int GetNviews(eViewType type) const;
  /** \brief build QA plots for particular setups */
  void InitMcbm22();
  void InitMcbm24();
  void InitDefault();

  CbmKfTrackFitter fFitter;
  TClonesArray* fGTracks                          = nullptr;  //! reconstructed global tracks / event
  std::map<ECbmModuleId, TClonesArray*> fTracks   = {};       //! reconstructed global tracks / event
  TClonesArray* fTrackMatches                     = nullptr;  //! MC info for the global tracks
  TClonesArray* fEvents                           = nullptr;  //! reconstructed events
  CbmTimeSlice* fTimeSlice                        = nullptr;  //! Time slice info
  std::map<ECbmModuleId, TClonesArray*> fHits     = {};       //! reconstructed hits
  std::map<ECbmModuleId, CbmMCDataArray*> fPoints = {};       //! mc points
  std::map<ECbmModuleId, TClonesArray*> fHitMatch = {};       //! reconstructed hits
  std::vector<EventFilter> fFilterEv              = {};
  std::vector<TrackFilter> fFilterTrk             = {};
  CbmMCDataManager* cbm_mc_manager                = nullptr;
  TDirectoryFile fOutFolder                       = {"RecoQA", "CA track driven reco QA"};  //!
  eSetup fSetupClass                              = eSetup::kMcbm24;
  std::map<ECbmModuleId, Detector> fDetQa         = {};  //! list of detector QA
  std::map<const char*, View> fViews              = {};  //! list of QA views
  std::vector<TVector3> fPrjPlanes = {};  //! local storage for the z positions of track projection planes

  ClassDef(CbmRecoQaTask, 1);  // Reconstruction QA analyzed from CA perspective
};

#endif  // CBMRECOQATASK_H
