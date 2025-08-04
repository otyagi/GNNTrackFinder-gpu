/* Copyright (C) 2010-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, P.-A. Loizeau, Florian Uhlig [committer] */

/**********************************************************************************************************************/
#include "FairLogger.h"
namespace geo_ci
{
  /**
   ** Temporary solution to bring capability of checking geometries after alignment.
   ** Basically a copy of the FairAlignmentHandler of FairRoot v18.6.9 and v18.8.1
   ** FIXME: remove once a solution is found to make the FairRoot alignment functionalities available in ROOT CLI.
   **/
  class FairAlignmentHandler {
  public:
    std::map<std::string, TGeoHMatrix> fAlignmentMatrices;

    void AlignGeometry() const;
    void AlignGeometryByFullPath() const;
    void AlignGeometryBySymlink() const;

    void AddAlignmentMatrices(const std::map<std::string, TGeoHMatrix>& alignmentMatrices, bool invertMatrices);

    void RecomputePhysicalAssmbBbox() const;

    FairAlignmentHandler();
    virtual ~FairAlignmentHandler();
  };
  /**********************************************************************************************************************/
  /**********************************************************************************************************************/
  FairAlignmentHandler::FairAlignmentHandler() {}

  FairAlignmentHandler::~FairAlignmentHandler() {}

  void FairAlignmentHandler::AlignGeometry() const
  {
    if (fAlignmentMatrices.size() > 0) {
      LOG(info) << "aligning the geometry...";

      LOG(info) << "aligning in total " << fAlignmentMatrices.size() << " volumes.";
      if (gGeoManager->GetNAlignable() > 0) {  //
        AlignGeometryBySymlink();
      }
      else {
        AlignGeometryByFullPath();
      }

      // --- Force BoundingBox recomputation for AssemblyVolumes as they may have been corrupted by alignment
      // FIXME: will hopefully be fixed in Root in near future, temp fix in meantime
      RecomputePhysicalAssmbBbox();

      LOG(info) << "Refreshing geometry...";
      gGeoManager->RefreshPhysicalNodes(kFALSE);

      LOG(info) << "alignment finished!";
    }
  }

  void FairAlignmentHandler::AlignGeometryByFullPath() const
  {
    TString volume_path;

    LOG(info) << "aligning using full path.";
    for (auto const& alignment_entry : fAlignmentMatrices) {
      volume_path = alignment_entry.first;

      gGeoManager->cd(volume_path);

      TGeoNode* volume_node     = gGeoManager->GetCurrentNode();
      TGeoMatrix* volume_matrix = volume_node->GetMatrix();
      // Need to do this as since ROOT 6.14 TGeoMatrix has no multiplication operator anymore
      // it is implimnted now in TGeoHMatrix

      TGeoHMatrix local_volume_matrix = TGeoHMatrix(*volume_matrix);

      TGeoHMatrix* new_volume_matrix = new TGeoHMatrix(local_volume_matrix * alignment_entry.second);
      // new matrix, representing real position (from new local mis RS to the global one)

      TGeoPhysicalNode* pn = gGeoManager->MakePhysicalNode(volume_path);

      pn->Align(new_volume_matrix);
    }
    LOG(info) << "alignments applied!";
  }

  void FairAlignmentHandler::AlignGeometryBySymlink() const
  {
    TString volume_path;

    LOG(info) << "aligning using symlinks";
    for (auto const& alignment_entry : fAlignmentMatrices) {
      volume_path = alignment_entry.first;

      TGeoPhysicalNode* node = NULL;
      TGeoPNEntry* entry     = gGeoManager->GetAlignableEntry(volume_path);
      if (entry) {  //
        node = gGeoManager->MakeAlignablePN(entry);
      }

      TGeoMatrix* volume_matrix = NULL;
      if (node) {  //
        volume_matrix = node->GetMatrix();
      }
      else {
        continue;
      }
      // Need to do this as since ROOT 6.14 TGeoMatrix has no multiplication operator anymore
      // it is implimnted now in TGeoHMatrix
      TGeoHMatrix local_volume_matrix = TGeoHMatrix(*volume_matrix);

      TGeoHMatrix* new_volume_matrix = new TGeoHMatrix(local_volume_matrix * alignment_entry.second);
      // new matrix, representing real position (from new local mis RS to the global one)
      node->Align(new_volume_matrix);
    }
  }

  void FairAlignmentHandler::AddAlignmentMatrices(const std::map<std::string, TGeoHMatrix>& alignmentMatrices,
                                                  bool invertMatrices)
  {
    LOG(info) << "adding inverting matrices...";
    for (auto const& m : alignmentMatrices) {
      if (invertMatrices)  //
        fAlignmentMatrices[m.first] *= m.second.Inverse();
      else
        fAlignmentMatrices[m.first] *= m.second;
    }
  }

  void FairAlignmentHandler::RecomputePhysicalAssmbBbox() const
  {
    TObjArray* pPhysNodesArr = gGeoManager->GetListOfPhysicalNodes();

    TGeoPhysicalNode* pPhysNode  = nullptr;
    TGeoShapeAssembly* pShapeAsb = nullptr;

    Int_t iNbNodes = pPhysNodesArr->GetEntriesFast();
    for (Int_t iInd = 0; iInd < iNbNodes; ++iInd) {
      pPhysNode = dynamic_cast<TGeoPhysicalNode*>(pPhysNodesArr->At(iInd));
      if (pPhysNode) {
        pShapeAsb = dynamic_cast<TGeoShapeAssembly*>(pPhysNode->GetShape());
        if (pShapeAsb) {
          // Should reach here only if the original node was a TGeoShapeAssembly
          pShapeAsb->ComputeBBox();
        }
      }
    }
  }
}  // namespace geo_ci
/**********************************************************************************************************************/

/// Disable clang formatting to keep easier exceptions lists reading (and better comparison to error output)
/* clang-format off */
std::vector<std::string> mcbm_pipevac_bmon_overlaps = {
  "cave/pipe_v19b_0/vacu20_1 overlapping cave/tof_v19d_mcbm_0/tof_v19d_mcbmStand_1/module_5_0",
  "cave/pipe_v19b_0/vacu20_1 overlapping cave/tof_v19e_mcbm_0/tof_v19e_mcbmStand_1/module_5_0",
  "cave/pipe_v19b_0/vacu20_1 overlapping cave/tof_v20f_mcbm_0/tof_v20f_mcbmStand_1/module_5_0",
  "cave/pipe_v19f_0/vacu20_1 overlapping cave/tof_v20c_mcbm_0/tof_v20c_mcbmStand_1/module_5_0",
  "cave/pipe_v19f_0/vacu20_1 overlapping cave/tof_v21d_mcbm_0/tof_v21d_mcbmStand_1/module_5_0",
  "cave/pipe_v19f_0/vacu20_1 overlapping cave/tof_v21f_mcbm_0/tof_v21f_mcbmStand_1/module_5_0",
  "cave/pipe_v19f_0/vacu20_1 overlapping cave/tof_v21j_mcbm_0/tof_v21j_mcbmStand_1/module_5_0",
  "cave/pipe_v19f_0/vacu20_1 overlapping cave/tof_v22a_mcbm_0/tof_v22a_mcbmStand_1/module_5_0",
  "cave/pipe_v19f_0/vacu20_1 overlapping cave/tof_v24a_mcbm_0/tof_v24a_mcbmStand_1/module_5_0",
  "cave/pipe_v19f_0/vacu20_1 overlapping cave/tof_v24b_mcbm_0/tof_v24b_mcbmStand_1/module_5_0",
  "cave/pipe_v19f_0/vacu20_1 overlapping cave/tof_v24d_mcbm_0/tof_v24d_mcbmStand_1/module_5_0",
  "cave/pipe_v19f_0/vacu20_1 overlapping cave/tof_v24f_mcbm_0/tof_v24f_mcbmStand_1/module_5_0"
};

std::vector<std::pair<std::string, std::string>> mcbm_dets_overlaps = {
  {"gas_box extruded by: gas_box/counter_0", "between gas_box and gas_box/counter_0 (internal to mTOF v19e)"},
  {"gas_box/counter_0 overlapping gas_box/counter_1", "between counters 0 and 1 (internal to mTOF v19e)"},
  {"HalfLadder12d_Module04 extruded by: HalfLadder12d_Module04/Sensor04_1",
   ": module 4 sticking out of ladder 12 (internal to mSTS v22f)"},
  {"tof_v21j_mcbmStand/module_2_0 overlapping tof_v21j_mcbmStand/module_9_0",
   "between modules 2 and 9 (internal to mTOF v21j)"}
};

std::vector<std::pair<std::string, std::string>> mcbm_dets_overlaps_sampling = {
  {"tof_v19d_mcbmStand: node module_8_0 overlapping module_8_0", "between module 8 and itself (internal to mTOF v19d)"},
  {"tof_v19e_mcbmStand: node module_8_0 overlapping module_8_0", "between module 8 and itself (internal to mTOF v19e)"},
  {"gas_box: node counter_0 overlapping counter_1", "between counters 0 and 1 (internal to mTOF v19e)"},
  {"gas_box: node counter_1 overlapping counter_2", "between counters 1 and 2 (internal to mTOF v19e)"},
  {"gas_box: node counter_2 overlapping counter_3", "between counters 2 and 3 (internal to mTOF v19e)"},
  {"gas_box: node counter_3 overlapping counter_4", "between counters 3 and 4 (internal to mTOF v19e)"},
  {"gas_box: node counter_4 overlapping counter_5", "between counters 4 and 5 (internal to mTOF v19e)"},
  {"gas_box: node counter_5 overlapping counter_6", "between counters 5 and 6 (internal to mTOF v19e)"},
  {"gas_box: node counter_6 overlapping counter_7", "between counters 6 and 7 (internal to mTOF v19e)"},
  {"counter: node Gap_3 overlapping tof_glass_4", "between gap and glass (internal to mTOF v21f)"},
  {"tof_v21j_mcbmStand: node module_2_0 overlapping module_9_0", "between modules 2 and 9 (internal to mTOF v21j)"}
};

std::vector<std::string> cbm_pipevac_bmon_overlaps = {
  "cave/pipe_v20a_1m_0/pipevac7_0 overlapping cave/tof_v20b_1m_0/module_5_0",
  "cave/pipe_v20a_1m_0/pipe7_0 overlapping cave/tof_v20b_1m_0/module_5_0"
};

std::vector<std::pair<std::string, std::string>> cbm_dets_overlaps = {
  {"cave/magnet_container_0 overlapping cave/rich_v17a_3e_0/rich_container_288", "between rich v17a_3e and magnet"},
  {"cave/magnet_container_0 overlapping cave/rich_v17a_1e_0/rich_container_290", "between rich v17a_1e and magnet"},
  {"cave/magnet_container_0 overlapping cave/rich_v21a_0/rich_container_1", "between rich v21a & magnet v21a, DEC21"},
  {"cave/pipe_v24i_0/conical_beam_pipe_1/conus_volume_1 overlapping cave/rich_v23a_0/rich_v23a_1/rich_cont_1",
   "between rich v23_a and cone, known beampipe fix pending"},
  {"cave/pipe_v24i_0/vacuum_conical_beam_pipe_1/vacuum_conus_volume_1 overlapping cave/rich_v23a_0/rich_v23a_1/rich_cont_1",
   "between rich v23_a and cone, known beamtime fix pending"}
};

std::vector<std::pair<std::string, std::string>> cbm_dets_overlaps_sampling = {
  {"cave: node magnet_container_0 overlapping rich_v17a_3e_0/rich_container_288", "between rich v17a_3e and magnet"},
  {"cave: node magnet_container_0 overlapping rich_v17a_1e_0/rich_container_290", "between rich v17a_1e and magnet"},
  {"cave: node magnet_container_0 overlapping rich_v21a_0/rich_container_1", "between rich v21a & magnet v21a, DEC21"},
  {"belt_assembly_1: node belt_part3_156 overlapping belt_part4_157", "between parts of belt in RICH (s300e)"},
  {"belt: node belt2_1 overlapping belt4_1", "between parts of belt in RICH v23"},
  {"shielding_box: node shielding_box_1 overlapping shieling_wing_1", "between shielding box parts (RICH internal)"},
  {"MVDscripted: node station_S0_1/heatsink_S0_1/heatsinkpart_2_2 overlapping top_bottom_plate_2",
   "between heatsink and bottom plate (internal to MVD)"},
  {"TwoStation: node station_S0_1/heatsink_S0_1/heatsinkpart_2_2 overlapping top_bottom_plate_2",
   "between heatsink and bottom plate (internal to MVD)"},
  {"conical_beam_pipe: node rich_much_downstream_flange_1 overlapping conus_volume_1",  // Fix in next beampipe geo
   "between R/M flange and cone (beampipe internal), known fix pending"}
};

/// Re-enable clang formatting after exceptions lists filling
/* clang-format on */

bool expected_mcbm(TGeoOverlap* ov)
{
  TString OverlapName = ov->GetTitle();
  /// Bmon overlapping vacuum because introduced by mTOF which is placed in cave
  for (auto sVacBmon : mcbm_pipevac_bmon_overlaps) {
    if (OverlapName.Contains(sVacBmon)) {
      std::cout << "Expected Overlap between pipe vacuum and bmon in tof (mCBM)" << endl;
      std::cout << ov->GetTitle() << std::endl << std::endl;
      return true;
    }
  }
  /// Other detectors overlaps (either internals or caused by alignment matrices)
  for (auto pDetOver : mcbm_dets_overlaps) {
    if (OverlapName.Contains(pDetOver.first)) {
      std::cout << "Expected Overlap " << pDetOver.second << endl;
      std::cout << ov->GetTitle() << std::endl << std::endl;
      return true;
    }
  }
  return false;
}

bool expected_mcbm_sampling(TGeoOverlap* ov)
{
  TString OverlapName = ov->GetTitle();
  /// Detectors overlaps only in sampling (either internals or caused by alignment matrices)
  for (auto pDetOver : mcbm_dets_overlaps_sampling) {
    if (OverlapName.Contains(pDetOver.first)) {
      std::cout << "Expected Overlap (sampled) " << pDetOver.second << endl;
      std::cout << ov->GetTitle() << std::endl << std::endl;
      return true;
    }
  }
  return false;
}

bool expected_cbm(TGeoOverlap* ov)
{
  TString OverlapName = ov->GetTitle();
  /// Bmon overlapping vacuum because introduced by TOF which is placed in cave (ONLY IN APR21!!!)
  for (auto sVacBmon : cbm_pipevac_bmon_overlaps) {
    if (OverlapName.Contains(sVacBmon)) {
      std::cout << "Expected Overlap between pipe vacuum and bmon in tof (mCBM)" << endl;
      std::cout << ov->GetTitle() << std::endl << std::endl;
      return true;
    }
  }
  /// Detectors overlaps (either internals or caused by alignment matrices)
  for (auto pDetOver : cbm_dets_overlaps) {
    if (OverlapName.Contains(pDetOver.first)) {
      std::cout << "Expected Overlap " << pDetOver.second << endl;
      std::cout << ov->GetTitle() << std::endl << std::endl;
      return true;
    }
  }
  return false;
}

bool expected_cbm_sampling(TGeoOverlap* ov)
{
  TString OverlapName = ov->GetTitle();
  /// Detectors overlaps only in sampling (either internals or caused by alignment matrices)
  for (auto pDetOver : cbm_dets_overlaps_sampling) {
    if (OverlapName.Contains(pDetOver.first)) {
      std::cout << "Expected Overlap (sampled) " << pDetOver.second << endl;
      std::cout << ov->GetTitle() << std::endl << std::endl;
      return true;
    }
  }
  return false;
}

// SurfaceThresold - if extremal distance of overlap is greater than this, then an overlap is trigged.
// NumSampling is number of sample points per volume. 1 million default. Set to 0 to disable.
// MaxSamplingTolerance is the total unexpected overlap tolerance.

void check_overlaps(TString dataset = "test", TString alignment_matrices = "", Double_t SurfaceThreshold = 0.0001,
                    uint32_t NumSampling = 1000000, Double_t MaxSamplingTolerance = 1.0e-04)
{
  // 2014-07-04 - DE  - test CBM setups for collisions in nightly tests
  // 2014-07-04 - DE  - currently there are 2 overlaps between the PIPE and STS layer 8
  // 2014-07-04 - DE  - set the default to 0 overlaps, anyway
  // 2017-29-11 - FU  - define some expected overlaps between magnet and rich or much
  //                    these overlaps are accepted for the time being until there is a new magnet geometry
  // 2023-07-xx - PAL - Test all setups in nightly tests, make the list of expected overlaps less ad-hoc,
  //                    add all mCBM ones as expected, add a few CBM ones, create redmine issues for the rest

  UInt_t unexpectedOverlaps {0};

  TString geoFile = dataset + ".geo.root";
  TFile* f        = new TFile(geoFile);
  if (!f->IsOpen()) {
    std::cout << "check_overlaps: geometry file " << geoFile << " is not accessible!" << std::endl;
    return;
  }

  gGeoManager = (TGeoManager*) f->Get("FAIRGeom");

  if ("" != alignment_matrices) {

    std::cout << " Applying alignment matrices from file " << alignment_matrices << std::endl;
    // Define the basic structure which needs to be filled with information
    // This structure is stored in the output file and later passed to the
    // FairRoot framework to do the (miss)alignment
    std::map<std::string, TGeoHMatrix>* matrices {nullptr};

    // read matrices from disk
    TFile* misalignmentMatrixRootfile = new TFile(alignment_matrices, "READ");
    if (misalignmentMatrixRootfile->IsOpen()) {
      gDirectory->GetObject("MisalignMatrices", matrices);
      misalignmentMatrixRootfile->Close();
    }
    else {
      std::cout << "Could not open file " << alignment_matrices << "\n Exiting";
      return;
    }

    if (matrices) {
      geo_ci::FairAlignmentHandler* handler = new geo_ci::FairAlignmentHandler();
      handler->AddAlignmentMatrices(*matrices, false);
      handler->AlignGeometry();
      delete handler;
    }
    else {
      std::cout << "Alignment required but no matrices found. \n Exiting";
      return;
    }
  }

  gGeoManager->CheckOverlaps(SurfaceThreshold);
  TIter next(gGeoManager->GetListOfOverlaps());
  TGeoOverlap* ov;

  while ((ov = (TGeoOverlap*) next())) {
    TString OverlapName = ov->GetTitle();

    if ((dataset.Contains("mcbm") && expected_mcbm(ov)) || expected_cbm(ov)) {
      /// Detected and logged in special function above
      continue;
    }
    else {
      cout << "Unexpected Overlap:" << endl;
      ov->PrintInfo();
      cout << endl;
      unexpectedOverlaps++;
    }
  }
  std::cout << std::endl;


  /*
   * Disabled by defaults the sampling part of the check as its threshold is broken
   * For explanation see
   * - https://git.cbm.gsi.de/computing/cbmroot/-/merge_requests/1639
   * - https://github.com/root-project/root/issues/14675
   * - https://redmine.cbm.gsi.de/issues/3198
   */

  Double_t SumUnexpectedOverlapSizes = 0;

  if (0 < NumSampling) {
    // Surface threshold may not do anything when using Sampling method
    //                       Threshold | N Samples
    gGeoManager->CheckOverlaps(SurfaceThreshold, Form("s%u", NumSampling));

    TIter next1(gGeoManager->GetListOfOverlaps());
    while ((ov = (TGeoOverlap*) next1())) {
      if ((dataset.Contains("mcbm") && expected_mcbm_sampling(ov)) || expected_cbm_sampling(ov)) {
        /// Detected and logged in special function above
        continue;
      }
      else {
        std::cout << "Unexpected Overlap (sampled):" << std::endl;
        ov->PrintInfo();
        unexpectedOverlaps++;
        SumUnexpectedOverlapSizes += ov->GetOverlap();
        std::cout << std::endl;
      }
    }
    std::cout << std::endl;
  }


  // If both Surface and Sampling show no overlaps
  if (unexpectedOverlaps == 0) {
    std::cout << " There are no unexpected overlaps." << std::endl;
    std::cout << " Test passed" << std::endl;
    std::cout << " All ok " << std::endl;
  }

  // If sampling enabled and total overlap size below the tolerance
  else if (NumSampling > 0 && SumUnexpectedOverlapSizes < MaxSamplingTolerance) {
    std::cout << " There were " << unexpectedOverlaps << " unexpected overlaps." << std::endl;
    std::cout << " Totaling " << SumUnexpectedOverlapSizes << " which is within tolerance of " << MaxSamplingTolerance
              << "." << std::endl;
    std::cout << " Test passed" << std::endl;
    std::cout << " All ok " << std::endl;
  }

  // Other if there wrere overlaps, either in the surface check or in sampling check and above tolearance.
  else {

    std::cout << " Test failed" << std::endl;
    std::cout << " We have in total " << unexpectedOverlaps << " unexpected overlaps." << std::endl;
  }

  //RemoveGeoManager();
}
