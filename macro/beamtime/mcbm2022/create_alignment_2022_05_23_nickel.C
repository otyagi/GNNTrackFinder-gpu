/* Copyright (C) 2022-2024 UGiessen, Giessen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig, Adrian Weber [committer], Alexandru Bercuci*/

#include <TFile.h>
#include <TGeoMatrix.h>

#include <map>
#include <string>

// AB 31.09.23 according to "Discussion and agreement on the alignment version and TOF calibration" @ https://indico.gsi.de/event/18210/
// shift detection systems such that Vx is centered @ (0, 0, 0)
const float vx = -0.41, vy = +0.59, vz = +2.80;
std::pair<std::string, TGeoHMatrix> AlignNode(std::string path, double shiftX, double shiftY, double shiftZ,
                                              double rotX, double rotY, double rotZ)
{

  TGeoHMatrix result;
  result.SetDx(shiftX);
  result.SetDy(shiftY);
  result.SetDz(shiftZ);
  result.RotateX(rotX);
  result.RotateY(rotY);
  result.RotateZ(rotZ);

  std::cout << "Alignment matrix for node " << path << " is: " << std::endl;
  result.Print();
  std::cout << std::endl;

  return std::pair<std::string, TGeoHMatrix>(path, result);
}


int create_alignment_2022_05_23_nickel()
{
  // Define the basic structure which needs to be filled with information
  // This structure is stored in the output file and later passed to the
  // FairRoot framework to do the (miss)alignment
  std::map<std::string, TGeoHMatrix> matrices;

  // ----------------  STS  ----------------------------//
  // Align full STS to translate Vx to (0, 0, 0)
  /* U0 */ matrices.insert(AlignNode("/cave_1/sts_v22f_mcbm_0/Station01_1", -vx, -vy, -vz, 0., 0., 0.));
  /* U1 */ matrices.insert(AlignNode("/cave_1/sts_v22f_mcbm_0/Station02_2", -vx, -vy, -vz, 0., 0., 0.));

  // Align individual STS Modules
  /* U0 L0 HL1 */
  /* M0 */ matrices.insert(
    AlignNode("/cave_1/sts_v22f_mcbm_0/Station01_1/Ladder09_1/HalfLadder09d_2/HalfLadder09d_Module03_1", +0.005, 0.000,
              0.005, 0., 0., 0.));
  /* M1 */ matrices.insert(
    AlignNode("/cave_1/sts_v22f_mcbm_0/Station01_1/Ladder09_1/HalfLadder09d_2/HalfLadder09d_Module03_2", +0.013, 0.000,
              -0.010, 0., 0., 0.));

  /* U0 L1 HL1 */
  /* M0 */ matrices.insert(
    AlignNode("/cave_1/sts_v22f_mcbm_0/Station01_1/Ladder09_2/HalfLadder09d_2/HalfLadder09d_Module03_1", -0.011, 0.000,
              +0.050, 0., 0., 0.));
  /* M1 */ matrices.insert(
    AlignNode("/cave_1/sts_v22f_mcbm_0/Station01_1/Ladder09_2/HalfLadder09d_2/HalfLadder09d_Module03_2", -0.019, 0.000,
              +0.050, 0., 0., 0.));

  matrices.insert(AlignNode("/cave_1/sts_v22f_mcbm_0/Station01_1", -0.41, +0.59, +2.80, 0., 0., 0.));

  /* U1 L0 HL1 */
  /* M0 */ matrices.insert(
    AlignNode("/cave_1/sts_v22f_mcbm_0/Station02_2/Ladder10_1/HalfLadder10d_2/HalfLadder10d_Module03_1", +0.030, -0.080,
              +0.610, 0., 0., 0.));
  /* M1 */ matrices.insert(
    AlignNode("/cave_1/sts_v22f_mcbm_0/Station02_2/Ladder10_1/HalfLadder10d_2/HalfLadder10d_Module04_2", 0.000, -0.080,
              +0.600, 0., 0., 0.));

  /* U1 L1 HL1 */
  /* M0 */ matrices.insert(
    AlignNode("/cave_1/sts_v22f_mcbm_0/Station02_2/Ladder12_2/HalfLadder12d_2/HalfLadder12d_Module03_1", -0.020, -0.020,
              -0.250, 0., 0., 0.));
  /* M1 */ matrices.insert(
    AlignNode("/cave_1/sts_v22f_mcbm_0/Station02_2/Ladder12_2/HalfLadder12d_2/HalfLadder12d_Module04_2", -0.010, -0.030,
              -0.200, 0., 0., 0.));

  /* U1 L2 HL1 */
  /* M0 */ matrices.insert(
    AlignNode("/cave_1/sts_v22f_mcbm_0/Station02_2/Ladder11_3/HalfLadder11d_2/HalfLadder11d_Module03_1", +0.020, -0.040,
              +0.610, 0., 0., 0.));
  /* M1 */ matrices.insert(
    AlignNode("/cave_1/sts_v22f_mcbm_0/Station02_2/Ladder11_3/HalfLadder11d_2/HalfLadder11d_Module03_2", +0.000, 0.000,
              0.000, 0., 0., 0.));
  /* M2 */ matrices.insert(
    AlignNode("/cave_1/sts_v22f_mcbm_0/Station02_2/Ladder11_3/HalfLadder11d_2/HalfLadder11d_Module03_3", -0.020, -0.040,
              +0.500, 0., 0., 0.));

  // ----------------  TRD  ----------------------------//
  // Align full TRD to translate Vx to (0, 0, 0)
  /* TRD */ matrices.insert(AlignNode("/cave_1/trd_v22h_mcbm_0", -vx, -vy, -vz, 0., 0., 0.));
  /* TRD2D  */ matrices.insert(
    AlignNode("/cave_1/trd_v22h_mcbm_0/layer01_20101/module9_101001001", -2.0, 0.0, 0.0, 0., 0., 0.));
  /* TRD1Dx */ matrices.insert(
    AlignNode("/cave_1/trd_v22h_mcbm_0/layer02_10202/module8_101002001", -2.8, 0.0, -1.34, 0., 0., 0.));
  /* TRD1Dy */ matrices.insert(
    AlignNode("/cave_1/trd_v22h_mcbm_0/layer03_11303/module8_101303001", 0.0, -3.0, -1.34, 0., 0., 0.));


  // ----------------  TOF  ----------------------------//
  // Align full ToF to translate Vx to (0, 0, 0)
  /* ToF */ matrices.insert(AlignNode("/cave_1/tof_v21k_mcbm_0", -vx, -vy, -vz, 0., 0., 0.));
  // SM 00 ++++++++++++++++++++++++++
  matrices.insert(AlignNode("/cave_1/tof_v21k_mcbm_0/tof_v21k_mcbmStand_1/module_0_0", -0.60, -2.15, -3.2, 0., 0., 0.));

  // SM 01 ++++++++++++++++++++++++++
  matrices.insert(AlignNode("/cave_1/tof_v21k_mcbm_0/tof_v21k_mcbmStand_1/module_0_1", -0.50, -2.55, -3.6, 0., 0., 0.));

  // SM 20 ++++++++++++++++++++++++++
  matrices.insert(AlignNode("/cave_1/tof_v21k_mcbm_0/tof_v21k_mcbmStand_1/module_2_0", -0.50, -2.96, -4.8, 0., 0., 0.));
  // SM 20, RPC 1
  matrices.insert(AlignNode("/cave_1/tof_v21k_mcbm_0/tof_v21k_mcbmStand_1/module_2_0/gas_box_0/counter_1", -0.25, 0.0,
                            0.0, 0., 0., 0.));
  // SM 20, RPC 2
  matrices.insert(AlignNode("/cave_1/tof_v21k_mcbm_0/tof_v21k_mcbmStand_1/module_2_0/gas_box_0/counter_2", -0.25, 0.0,
                            0.0, 0., 0., 0.));
  // SM 20, RPC 3
  matrices.insert(AlignNode("/cave_1/tof_v21k_mcbm_0/tof_v21k_mcbmStand_1/module_2_0/gas_box_0/counter_3", -0.25, 0.0,
                            0.0, 0., 0., 0.));

  // SM 02 ++++++++++++++++++++++++++
  matrices.insert(AlignNode("/cave_1/tof_v21k_mcbm_0/tof_v21k_mcbmStand_1/module_0_2", -0.70, -3.10, -3.7, 0., 0., 0.));

  // SM 03 ++++++++++++++++++++++++++
  matrices.insert(AlignNode("/cave_1/tof_v21k_mcbm_0/tof_v21k_mcbmStand_1/module_0_3", -0.75, -2.79, +2.3, 0., 0., 0.));

  // ---------------  RICH  ----------------------------//
  // Align full Rich
  //   matrices.insert(AlignNode("/cave_1/rich_v21c_mcbm_0/box_1", 0.0, 0.0, 0.0, 0., 0.0, 0.));


  // save matrices to disk
  TFile* misalignmentMatrixRootfile = new TFile("AlignmentMatrices_mcbm_beam_2022_05_23_nickel.root", "RECREATE");
  if (misalignmentMatrixRootfile->IsOpen()) {
    gDirectory->WriteObject(&matrices, "MisalignMatrices");
    misalignmentMatrixRootfile->Write();
    misalignmentMatrixRootfile->Close();
  }

  return 0;
}
