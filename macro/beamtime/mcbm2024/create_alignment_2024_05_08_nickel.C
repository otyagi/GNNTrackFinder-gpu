/* Copyright (C) 2024 Horia Hulubei National Institute of Physics and Nuclear Engineering, Bucharest
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Alexandru Bercuci [committer]*/

#include <TFile.h>
#include <TGeoMatrix.h>

#include <map>
#include <string>

// FIXME: as of 20/11/2024, this macro most probably does NOT match the AlignmentMatrics file in the params repo !!!!!

// shift detection systems such that Vx is centered @ (0, 0, 0)
const float vx = +0.15, vy = -0.65, vz = 0.00;
std::pair<std::string, TGeoHMatrix> AlignNode(std::string path, double shiftX, double shiftY, double shiftZ,
                                              double rotX = 0., double rotY = 0., double rotZ = 0.)
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


int create_alignment_2024_05_08_nickel()
{
  // Define the basic structure which needs to be filled with information
  // This structure is stored in the output file and later passed to the
  // FairRoot framework to do the (miss)alignment
  std::map<std::string, TGeoHMatrix> matrices;

  std::string dtag = "v24c_mcbm";
  // ----------------  STS  ----------------------------//
  // Align full STS
  // U0
  matrices.insert(AlignNode(Form("/cave_1/sts_%s_0/Station01_1", dtag.data()), vx, vy, vz));
  // U1
  matrices.insert(AlignNode(Form("/cave_1/sts_%s_0/Station02_2", dtag.data()), vx, vy, vz));
  // U2
  matrices.insert(AlignNode(Form("/cave_1/sts_%s_0/Station03_3", dtag.data()), vx, vy, vz));

  // Align individual STS Modules
  // U0L0M0
  matrices.insert(AlignNode(
    Form("/cave_1/sts_%s_0/Station01_1/Ladder13_1/HalfLadder13u_1/HalfLadder13u_Module03_1/Sensor03_1", dtag.data()), 0,
    -0.044 - 0.02, -0.36 - 0.2));

  // U1
  // "U1L0",
  matrices.insert(
    AlignNode(Form("/cave_1/sts_%s_0/Station02_2/Ladder09_1/HalfLadder09d_2", dtag.data()), -0.02, 0, 0.18));  // xv
  // "U1L0M0",
  matrices.insert(AlignNode(
    Form("/cave_1/sts_%s_0/Station02_2/Ladder09_1/HalfLadder09d_2/HalfLadder09d_Module03_1/Sensor03_1", dtag.data()),
    -0.02, 0, 0.));
  // // "U1L0M1",
  // matrices.insert(AlignNode(Form("/cave_1/sts_%s_0/Station02_2/Ladder09_1/HalfLadder09d_2/HalfLadder09d_Module03_2/Sensor03_1", dtag.data()), 0., 0, 0.));
  //
  // "U1L1",
  matrices.insert(AlignNode(Form("/cave_1/sts_%s_0/Station02_2/Ladder09_2/HalfLadder09d_2", dtag.data()), 0.014,
                            -0.012 - 0.014, 0.));  //
  // "U1L1M0",
  // matrices.insert(AlignNode(Form("/cave_1/sts_%s_0/Station02_2/Ladder09_2/HalfLadder09d_2/HalfLadder09d_Module03_1/Sensor03_1", dtag.data()), 0, 0, 0.18));
  // // "U1L1M1",
  // matrices.insert(AlignNode(Form("/cave_1/sts_%s_0/Station02_2/Ladder09_2/HalfLadder09d_2/HalfLadder09d_Module03_2/Sensor03_1", dtag.data()), 0, 0, 0.18));

  // U2
  matrices.insert(AlignNode(Form("/cave_1/sts_%s_0/Station03_3", dtag.data()), 0.03, 0., 0.));
  // "U2L0",
  matrices.insert(
    AlignNode(Form("/cave_1/sts_%s_0/Station03_3/Ladder10_1/HalfLadder10d_2", dtag.data()), 0., -0.05 - 0.03, 0.));
  // "U2L0M0",
  // matrices.insert(AlignNode(Form("/cave_1/sts_%s_0/Station03_3/Ladder10_1/HalfLadder10d_2/HalfLadder10d_Module03_1/Sensor03_1", dtag.data()), 0., 0., 0.));
  // // "U2L0M1",
  // matrices.insert(AlignNode(Form("/cave_1/sts_%s_0/Station03_3/Ladder10_1/HalfLadder10d_2/HalfLadder10d_Module04_2/Sensor04_1", dtag.data()), 0., 0., 0.));
  //
  // "U2L1",
  matrices.insert(
    AlignNode(Form("/cave_1/sts_%s_0/Station03_3/Ladder12_2/HalfLadder12d_2", dtag.data()), 0., -0.03 - 0.02, 0.42));
  // "U2L1M0",
  matrices.insert(AlignNode(
    Form("/cave_1/sts_%s_0/Station03_3/Ladder12_2/HalfLadder12d_2/HalfLadder12d_Module03_1/Sensor03_1", dtag.data()),
    0., 0., 0.10));  // zv arbitrary
  // // "U2L1M1",
  // matrices.insert(AlignNode(Form("/cave_1/sts_%s_0/Station03_3/Ladder12_2/HalfLadder12d_2/HalfLadder12d_Module04_2/Sensor04_1", dtag.data()), 0., 0., 0.305));
  //
  // "U2L2",
  matrices.insert(
    AlignNode(Form("/cave_1/sts_%s_0/Station03_3/Ladder11_3/HalfLadder11d_2", dtag.data()), 0., -0.08, 0. /*0.27*/));
  // "U2L2M0",
  matrices.insert(AlignNode(
    Form("/cave_1/sts_%s_0/Station03_3/Ladder11_3/HalfLadder11d_2/HalfLadder11d_Module03_1/Sensor03_1", dtag.data()),
    0., -0.03, 0.));  // yv to align it with U2L2M2
  // // "U2L2M1",
  // matrices.insert(AlignNode(Form("/cave_1/sts_%s_0/Station03_3/Ladder11_3/HalfLadder11d_2/HalfLadder11d_Module03_2/Sensor03_1", dtag.data()), 0., 0.08, 0.27));
  // "U2L2M2",
  matrices.insert(AlignNode(
    Form("/cave_1/sts_%s_0/Station03_3/Ladder11_3/HalfLadder11d_2/HalfLadder11d_Module03_3/Sensor03_1", dtag.data()),
    -0.03, 0., 0.));

  // ----------------  TRD  ----------------------------//
  dtag = "v24e_mcbm";
  // Align full TRD to translate Vx to (0, 0, 0)
  matrices.insert(AlignNode(Form("/cave_1/trd_%s_0", dtag.data()), vx, vy, vz));
  // /* TRD2D  */
  // matrices.insert(
  //   AlignNode(Form("/cave_1/trd_%s_0/layer01_20101/module9_101001001", dtag.data()),
  //             0., 0., 0.));
  /* TRD1Dx */
  matrices.insert(AlignNode(Form("/cave_1/trd_%s_0/layer02_10202/module5_101002001", dtag.data()),
                            //    x                      y      z
                            -1.85 - 0.4 + 0.17 - 0.25, 0.382, 0.753));
  /* TRD1Dy */
  matrices.insert(AlignNode(Form("/cave_1/trd_%s_0/layer03_11303/module5_101103001", dtag.data()),
                            // -y             -x       z
                            0.19, -0.27 + 1.25 + 0.55, 0.));


  // ----------------  TOF  ----------------------------//
  // Align tracking Tof
  dtag = "v24d_mcbm";
  // Align full ToF to translate Vx to (0, 0, 0)
  matrices.insert(AlignNode(Form("/cave_1/tof_%s_0", dtag.data()), vx, vy, vz));
  matrices.insert(AlignNode(Form("/cave_1/tof_%s_0/tof_%sStand_1", dtag.data(), dtag.data()), -3., 0.5, 0.));
  float xTof[5][5] = {
    {0.5, 0.5, 0.5, 0.5, 0.5},  // Sm0
    {0., 0., 0., 0., 0.},       // Sm1
    {0.1, 0.1, 0.1, 0.1, 0.1},  // Sm2
    {0.4, 0.4, 0.4, 0.4, 0.4},  // Sm3
    {0., 0., 0., 0., 0.}        // Sm4
  };
  float yTof[5][5] = {
    {1.5, 1.5, 1.5, 1.5, 1.5},  // Sm0
    {0.3, 0.3, 0.3, 0.3, 0.3},  // Sm1
    {2., 2., 2., 2., 2.},       // Sm2
    {10., 10., 10., 10., 10.},  // Sm3
    {0., 0., 0., 0., 0.}        // Sm4
  };
  float zTof[5][5] = {
    {0., 0., 0., 0., 0.},  // Sm0
    {0., 0., 0., 0., 0.},  // Sm1
    {0., 0., 0., 0., 0.},  // Sm2
    {0., 0., 0., 0., 0.},  // Sm3
    {0., 0., 0., 0., 0.}   // Sm4
  };
  int tofSelect[3] = {0};
  for (int ism(0); ism < 5; ism++) {
    tofSelect[0] = ism;
    for (int irpc(0); irpc < 5; irpc++) {
      tofSelect[2] = irpc;
      matrices.insert(AlignNode(Form("/cave_1/tof_%s_0/tof_%sStand_1/module_%d_%d/gas_box_0/counter_%d", dtag.data(),
                                     dtag.data(), tofSelect[1], tofSelect[0], tofSelect[2]),
                                xTof[ism][irpc], yTof[ism][irpc], zTof[ism][irpc]));
    }
  }

  // ---------------  RICH  ----------------------------//
  // Align full Rich
  //   matrices.insert(AlignNode("/cave_1/rich_v21c_mcbm_0/box_1", 0.0, 0.0, 0.0, 0., 0.0, 0.));


  // save matrices to disk
  TFile* misalignmentMatrixRootfile = new TFile("AlignmentMatrices_mcbm_beam_2024_05_08_nickel.root", "RECREATE");
  if (misalignmentMatrixRootfile->IsOpen()) {
    gDirectory->WriteObject(&matrices, "MisalignMatrices");
    misalignmentMatrixRootfile->Write();
    misalignmentMatrixRootfile->Close();
  }

  return 0;
}
