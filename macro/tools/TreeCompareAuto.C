/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [author], Pierre-Alain Loizeau [committer] */

/// DISCLAIMER: Work in Progress, can be used but with some levels of cross-checks as some checks not symmetric!!!

// The macro compares all the leaves of two TTree objects.
// The first and strongest comparison is that the entries in the two leaves
// to compare  are idential.
// To check this one loops over the entries in the trees, calculate the
// difference between the two values and fills the result in a histogram.
// If all entries are absolutely identical the result in the histogram is a
// delta function at 0.
// If the first check for identical entries fails it is checked if there are
// only some changes in the order of the entries. In this case the two
// produced histgrams (one for each tree) are identical.
// The last check which again is only done if both previous test fails uses a
// Kolmogorov test to check if the produced histograms are comparable on a
// statistical base.
// If at least for one leaf all three comparisons fail the complete test
// fails.

#include <RtypesCore.h>
#include <TBranch.h>
#include <TFile.h>
#include <TH1.h>
#include <TMath.h>
#include <TObjArray.h>
#include <TString.h>
#include <TTree.h>

#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>


std::vector<TString> GetLeafNames(TTree& cbmsimTree, bool noTracks, bool noCovMatrix);

bool operator==(TH1 const&, TH1 const&);

/// Parameters description:
/// - fileName1, fileName2 = mandatory full path to each of the ROOT files to compare
/// - treeName1, treeName2 = optional (full) tree name for each of the files
///                          e.g. "cbmsim" if multiple trees with different names are present (ROOT uses latest cycle)
///                          e.g "cbmsim;2" to select a specific cycle of a TTree different from most recent one
/// - noTracks = Optional flag, do not compare reco tracks if true
/// - noCovMatrix = Optional flag, do not compare covariance matrix(ces) if true
int TreeCompareAuto(TString fileName1, TString fileName2, TString selTreeName1 = "", TString selTreeName2 = "",
                    Bool_t noTracks = kFALSE, Bool_t noCovMatrix = kFALSE)
{
  if (fileName1.IsNull() || fileName2.IsNull()) {
    cout << "Filenames are not defined" << endl;
    return 42;
  }

  // Get the output tree from the original file
  std::unique_ptr<TFile> file1{TFile::Open(fileName1, "READ")};
  if (!file1) {
    std::cout << "Could not open file " << fileName1 << std::endl;
    return 42;
  }

  // Find a tree in the file
  TString treeName1;
  std::cout << "------------------------------------------------------------" << std::endl;
  if ("" != selTreeName1) {
    treeName1 = selTreeName1;
    std::cout << selTreeName1 << " hand-picked by user for " << fileName1 << std::endl;
  }
  else {
    TKey* key;
    TList* keylist1 = file1->GetListOfKeys();
    TIter keyIterator1(keylist1);
    Short_t treeCycle1{-1};
    int numTreeInFile1{0};
    std::cout << "Looking for TTree(s) in File " << fileName1 << std::endl;
    while ((key = static_cast<TKey*>(keyIterator1()))) {
      if (key->ReadObj()->InheritsFrom("TTree")) {
        TString newName  = key->GetName();
        Short_t newCycle = key->GetCycle();
        std::cout << "Found TTree with name " << newName << " and cycle " << newCycle << std::endl;
        if (treeName1 == newName) {
          if (treeCycle1 < newCycle) {
            treeCycle1 = newCycle;
          }
        }
        else {
          numTreeInFile1++;
        }
        treeName1 = key->GetName();
        if (-1 == treeCycle1) {
          treeCycle1 = newCycle;
        }
      }
    }
    if (0 == numTreeInFile1) {
      std::cout << "File " << fileName1 << " does not contain any TTree" << std::endl;
      return 42;
    }
    else if (1 == numTreeInFile1) {
      /// Cycle can be ignored if only one Tree is present (ROOT automatically picks the most recent one)
      std::cout << "Picked " << treeName1 << " (Newest cycle " << treeCycle1 << " should be picked auto by ROOT)"
                << std::endl;
    }
    else {
      std::cout << "File " << fileName1 << " contains more than one TTree and no TTree hand-picked" << std::endl;
      return 42;
    }
  }

  std::unique_ptr<TTree> tree1{file1->Get<TTree>(treeName1)};
  if (!tree1) {
    std::cout << "File " << fileName1 << " does not have the tree " << treeName1 << std::endl;
    return 42;
  }

  // Get the output tree from the file which should be compared
  std::unique_ptr<TFile> file2{TFile::Open(fileName2, "READ")};
  if (!file2) {
    std::cout << "Could not open file " << fileName2 << std::endl;
    return 42;
  }

  TString treeName2;
  std::cout << "------------------------------------------------------------" << std::endl;
  if ("" != selTreeName2) {
    treeName2 = selTreeName2;
    std::cout << selTreeName2 << " hand-picked by user for " << fileName2 << std::endl;
  }
  else {
    // Check for multiple trees also in this file
    TKey* key;
    TList* keylist2 = file2->GetListOfKeys();
    TIter keyIterator2(keylist2);
    Short_t treeCycle2{-1};
    int numTreeInFile2{0};
    bool bFirstTreeFound = false;
    std::cout << "Looking for TTree(s) in File " << fileName2 << std::endl;
    while ((key = static_cast<TKey*>(keyIterator2()))) {
      if (key->ReadObj()->InheritsFrom("TTree")) {
        TString newName  = key->GetName();
        Short_t newCycle = key->GetCycle();
        std::cout << "Found TTree with name " << newName << " and cycle " << newCycle << std::endl;
        if (newName == treeName1) {
          std::cout << "=> Matching selected tree in " << fileName1 << std::endl;
          bFirstTreeFound = true;
        }
        if (treeName2 == newName) {
          if (treeCycle2 < newCycle) {
            treeCycle2 = newCycle;
          }
        }
        else {
          numTreeInFile2++;
        }
        treeName2 = key->GetName();
        if (-1 == treeCycle2) {
          treeCycle2 = newCycle;
        }
      }
    }

    if (0 == numTreeInFile2) {
      std::cout << "File " << fileName2 << " does not contain any TTree" << std::endl;
      return 42;
    }
    else if (1 == numTreeInFile2) {
      /// Cycle can be ignored if only one Tree is present (ROOT automatically picks the most recent one)
      std::cout << "Picked " << treeName2 << " (Newest cycle " << treeCycle2 << " should be picked auto by ROOT)"
                << std::endl;
    }
    else {
      if (bFirstTreeFound) {
        treeName2 = treeName1;
      }
      else {
        std::cout << "File " << fileName2
                  << " contains more than one TTree, picked TTree from first file not found and no TTree hand-picked"
                  << std::endl;
        return 42;
      }
    }
  }

  // The name of the tree must be the same in both files
  std::unique_ptr<TTree> tree2{file2->Get<TTree>(treeName2)};
  if (!tree2) {
    std::cout << "File " << fileName2 << " does not have the tree" << treeName2 << std::endl;
    return 42;
  }

  // Add the output tree from the file to compare as friend to the tree
  // of the original file. This allows to access a data member of the
  // original file by e.g. StsHit.fX and the data element of the second tree
  // by tree2.StsHit.fX
  TString const friendName{"tree2"};
  tree1->AddFriend(tree2.get(), friendName);


  // Get the leaf names for all leaves which should be compared
  // from the first input tree
  auto leaves = GetLeafNames(*tree1.get(), noTracks, noCovMatrix);

  if (0 == leaves.size()) {
    std::cout << "Test passed. Tree does not contain any branches which must be checked" << std::endl;
    return 0;
  }

  TCanvas c1;

  std::stringstream outstream;
  bool okay{true};
  int numTestedLeaves{0};
  int numEmptyLeaves{0};
  int numLeaves{0};
  int numFailedLeaves{0};
  int numIdenticalEntries{0};
  int numIdenticalHistograms{0};
  int numKolmogorovHistograms{0};

  for (auto leaf : leaves) {
    TString leafName1 = leaf;
    TString leafName2 = friendName + "." + leafName1;
    outstream << "Comparing " << leafName1 << " and " << leafName2 << std::endl;

    TString command1 = leafName1 + ">>hist1";
    tree1->Draw(command1);
    int entries1{0};
    double_t low1{0.};
    double_t high1{0.};
    int nBins1{0};
    auto hist1 = static_cast<TH1F*>(gPad->GetPrimitive("hist1"));
    if (hist1) {
      entries1 = hist1->GetEntries();
      nBins1   = hist1->GetNbinsX();
      low1     = hist1->GetXaxis()->GetXmin();
      high1    = hist1->GetXaxis()->GetXmax();
    }

    if (nullptr == tree2->FindLeaf(leaf)) {
      /// FIXME: need reverse check for leaves found only in tree2!
      outstream << "Leaf " << leaf << " found only in first tree" << std::endl;
      hist1->Clear();
      okay = false;
      numTestedLeaves++;
      numFailedLeaves++;
      continue;
    }

    /// Proper Double full-range printout needed to avoid a "Bin w/ different edges" error in Kolmogorov test
    command1 = leafName2 + ">>hist2(" + nBins1 + ", " + Form("%10f", low1) + ", " + Form("%10f", high1) + ")";
    tree1->Draw(command1);
    auto hist2 = static_cast<TH1F*>(gPad->GetPrimitive("hist2"));
    int entries2{0};
    double_t low2{0.};
    double_t high2{0.};
    int nBins2{0};
    if (hist2) {
      entries2 = hist2->GetEntries();
      nBins2   = hist2->GetNbinsX();
      low2     = hist2->GetXaxis()->GetXmin();
      high2    = hist2->GetXaxis()->GetXmax();
    }

    if ((0 == entries1 && 0 != entries2) || (0 != entries1 && 0 == entries2)) {
      std::cout << "One of the distributions is empty" << std::endl;
      okay = false;
    }
    if (0 == entries1 && 0 == entries2) {
      outstream << "Both Histograms are empty." << std::endl;

      hist1->Clear();
      hist2->Clear();
      continue;
    }
    if (entries1 != entries2) {
      outstream << Form("Different number of entries: %10i vs %10i", entries1, entries2) << std::endl;
    }


    // When executing the draw command "Leaf1 - Leaf2" the subtraction is
    // executed entry by entry. If the content of the class members are
    // identical the result is a histogram with a delta function at 0
    // If the content is different one gets a distribution which is
    // detected.

    TString command = leafName1 + "-" + leafName2 + ">>histDiff(20, -10.,10.)";
    tree1->Draw(command);
    auto histDiff = static_cast<TH1F*>(gPad->GetPrimitive("histDiff"));
    numTestedLeaves++;

    // Check if the entries in the tree are identical
    outstream << "Checking for identical entries" << endl;

    if (TMath::Abs(histDiff->GetMean()) < 0.000001 && TMath::Abs(histDiff->GetRMS()) < 0.000001
        && 0 == histDiff->GetBinContent(0) && 0 == histDiff->GetBinContent(histDiff->GetNbinsX() + 1)) {
      numIdenticalEntries++;
      outstream << "Entries are identical." << std::endl;
      outstream << "**********************" << std::endl;
      hist1->Clear();
      hist2->Clear();
      histDiff->Clear();
      continue;
    }

    // If the entries are not identical check if the histograms are
    // identical. This is the case if the entries in the tree are sorted
    // differently
    outstream << "Checking for identical histograms" << endl;

    if (*hist1 == *hist2) {
      numIdenticalHistograms++;
      outstream << "Histograms are identical." << std::endl;
      outstream << "**********************" << std::endl;
      hist1->Clear();
      hist2->Clear();
      histDiff->Clear();
      continue;
    }

    // if also the histograms are not identical check if the histograms
    // are equal on a statistical base. Use The Kolmogorov test for
    // this.
    outstream << "Checking Kolmogorov" << endl;

    double kolmo = hist1->KolmogorovTest(hist2);

    outstream << "Result of Kolmogorov test: " << kolmo << endl;
    if (kolmo > 0.99) {
      numKolmogorovHistograms++;
      outstream << "**********************" << std::endl;
      hist1->Clear();
      hist2->Clear();
      histDiff->Clear();
      continue;
    }

    outstream << "Data are different" << std::endl;
    outstream << "**********************" << std::endl;
    outstream << "Entries: " << hist1->GetEntries() << std::endl;
    outstream << "XMin: " << hist1->GetXaxis()->GetXmin() << std::endl;
    outstream << "XMax: " << hist1->GetXaxis()->GetXmax() << std::endl;
    outstream << "Mean: " << hist1->GetMean() << std::endl;
    outstream << "RMS: " << hist1->GetRMS() << std::endl;
    outstream << "Underflow: " << hist1->GetBinContent(0) << std::endl;
    outstream << "Overflow: " << hist1->GetBinContent(hist1->GetNbinsX() + 1) << std::endl;
    outstream << "----------------------" << std::endl;
    outstream << "Entries: " << hist2->GetEntries() << std::endl;
    outstream << "XMin: " << hist2->GetXaxis()->GetXmin() << std::endl;
    outstream << "XMax: " << hist2->GetXaxis()->GetXmax() << std::endl;
    outstream << "Mean: " << hist2->GetMean() << std::endl;
    outstream << "RMS: " << hist2->GetRMS() << std::endl;
    outstream << "Underflow: " << hist2->GetBinContent(0) << std::endl;
    outstream << "Overflow: " << hist2->GetBinContent(hist2->GetNbinsX() + 1) << std::endl;
    outstream << "----------------------" << std::endl;
    outstream << "Entries: " << histDiff->GetEntries() << std::endl;
    outstream << "XMin: " << histDiff->GetXaxis()->GetXmin() << std::endl;
    outstream << "XMax: " << histDiff->GetXaxis()->GetXmax() << std::endl;
    outstream << "Mean: " << histDiff->GetMean() << std::endl;
    outstream << "RMS: " << histDiff->GetRMS() << std::endl;
    outstream << "Underflow: " << histDiff->GetBinContent(0) << std::endl;
    outstream << "Overflow: " << histDiff->GetBinContent(histDiff->GetNbinsX() + 1) << std::endl;
    outstream << "**********************" << std::endl;
    okay = false;
    numFailedLeaves++;
    hist1->Clear();
    hist2->Clear();
    histDiff->Clear();
  }

  if (!okay) {
    std::cout << outstream.str();
    std::cout << "Test failed." << std::endl;
    std::cout << numFailedLeaves << " of " << numTestedLeaves << " leaves are different." << std::endl;
    return 1;
  }

  //  std::cout << outstream.str();
  std::cout << "Tested leaves:                    " << numTestedLeaves << std::endl;
  //  std::cout << "Empty leaves:                     " << numEmptyLeaves << std::endl;
  std::cout << "Leaves with identical entries:    " << numIdenticalEntries << std::endl;
  std::cout << "Leaves with identical histograms: " << numIdenticalHistograms << std::endl;
  std::cout << "Leaves with kolmo histograms:     " << numKolmogorovHistograms << std::endl;
  std::cout << "Test passed. All leaves of all branches are exactly identical." << std::endl;

  return 0;
}

bool operator==(TH1 const& lhs, TH1 const& rhs)
{
  for (int x = 0; x < lhs.GetNbinsX() + 1; ++x) {
    if (lhs.GetBinContent(x) != rhs.GetBinContent(x)) {
      return false;
    }
  }
  return true;
}

std::vector<TString> GetLeafNames(TTree& cbmsimTree, bool noTracks, bool noCovMatrix)
{

  std::vector<TString> ListOfLeaves;

  TObjArray* leavesList = cbmsimTree.GetListOfLeaves();
  TIter leafIter(leavesList);
  TBranch* branch;
  while ((branch = (TBranch*) leafIter.Next())) {
    TString branchName = branch->GetName();
    //      cout << "Branch Name: " << branchName << endl;

    // exclude branches which contain tracks
    if (noTracks && branchName.Contains("Track") && !branchName.Contains("MCTrack")) {
      continue;
    }

    // exclude branches which contain CovMatrix
    if (noCovMatrix && branchName.Contains("CovMatrix")) {
      continue;
    }

    // Generate leaf names for transport file
    if (branchName.Contains("Point") && !branchName.Contains("MvdPileUpMC")) {
      TObjArray* brTokens = branchName.Tokenize(".");
      if (brTokens->GetEntriesFast() == 4) {
        TString _branch = ((TObjString*) (brTokens->At(2)))->GetString();
        TString _leaf   = ((TObjString*) (brTokens->At(3)))->GetString();
        if (_leaf.EqualTo("fLink")) {
          TString name = _branch + "." + _leaf + ".fLinks";
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fLinks.fFile");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fLinks.fType");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fLinks.fEntry");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fLinks.fIndex");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fLinks.fWeight");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fEntryNr.fFile");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fEntryNr.fType");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fEntryNr.fEntry");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fEntryNr.fIndex");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fEntryNr.fWeight");
        }
        else {
          ListOfLeaves.emplace_back(_branch + "." + _leaf);
        }
      }
    }
    if (branchName.Contains("MCTrack")) {
      TObjArray* brTokens = branchName.Tokenize(".");
      if (brTokens->GetEntriesFast() == 4) {
        TString _branch = ((TObjString*) (brTokens->At(2)))->GetString();
        TString _leaf   = ((TObjString*) (brTokens->At(3)))->GetString();
        ListOfLeaves.emplace_back(_branch + "." + _leaf);
      }
    }

    // Generate leaf names for digitization file
    if (branchName.Contains("Digi") && !(branchName.Contains("Match") || branchName.Contains("DigiEvent"))) {
      TObjArray* brTokens = branchName.Tokenize(".");
      if (brTokens->GetEntriesFast() == 2) {
        TString _branch = ((TObjString*) (brTokens->At(0)))->GetString();
        TString _leaf   = ((TObjString*) (brTokens->At(1)))->GetString();
        ListOfLeaves.emplace_back(_branch + "." + _leaf);
      }
    }

    // Generate leaf names for reconstruction file
    if (branchName.Contains("Hit") && !(branchName.Contains("Track") || branchName.Contains("DigiMatch"))) {
      TObjArray* brTokens = branchName.Tokenize(".");
      if (brTokens->GetEntriesFast() == 4) {
        TString _branch = ((TObjString*) (brTokens->At(2)))->GetString();
        TString _leaf   = ((TObjString*) (brTokens->At(3)))->GetString();
        if (_leaf.EqualTo("CbmHit")) {
          TString name = _branch + "." + _leaf + ".fType";
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fType");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fZ");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fDz");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fRefId");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fAddress");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fTime");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fTimeError");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fMatch.fTotalWeight");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fMatch.fMatchedIndex");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fMatch.fLinks.fFile");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fMatch.fLinks.fEntry");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fMatch.fLinks.fIndex");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fMatch.fLinks.fWeight");
        }
        else {
          ListOfLeaves.emplace_back(_branch + "." + _leaf);
        }
      }
    }
    if (branchName.Contains("Cluster") && !(branchName.Contains("Track") || branchName.Contains("DigiMatch"))) {
      TObjArray* brTokens = branchName.Tokenize(".");
      if (brTokens->GetEntriesFast() == 4) {
        TString _branch = ((TObjString*) (brTokens->At(2)))->GetString();
        TString _leaf   = ((TObjString*) (brTokens->At(3)))->GetString();
        if (_leaf.EqualTo("fMatch")) {
          TString name = _branch + "." + _leaf + ".fType";
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fTotalWeight");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fMatchedIndex");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fLinks.fFile");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fLinks.fEntry");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fLinks.fIndex");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fLinks.fWeight");
        }
        else {
          ListOfLeaves.emplace_back(_branch + "." + _leaf);
        }
      }
    }
    if (branchName.Contains("Track") && !(branchName.Contains("PrimaryVertex") || branchName.Contains("CbmEvent"))) {
      TObjArray* brTokens = branchName.Tokenize(".");
      if (brTokens->GetEntriesFast() == 4) {
        TString _branch = ((TObjString*) (brTokens->At(2)))->GetString();
        TString _leaf   = ((TObjString*) (brTokens->At(3)))->GetString();
        if (_leaf.EqualTo("fMatch")) {
          TString name = _branch + "." + _leaf + ".fType";
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fTotalWeight");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fMatchedIndex");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fLinks.fFile");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fLinks.fEntry");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fLinks.fIndex");
          ListOfLeaves.emplace_back(_branch + "." + _leaf + ".fLinks.fWeight");
        }
        else {
          ListOfLeaves.emplace_back(_branch + "." + _leaf);
        }
      }
      else if (brTokens->GetEntriesFast() == 5) {
        TString _branch = ((TObjString*) (brTokens->At(2)))->GetString();
        TString _leaf   = ((TObjString*) (brTokens->At(3)))->GetString();
        TString _leaf1  = ((TObjString*) (brTokens->At(4)))->GetString();
        ListOfLeaves.emplace_back(_branch + "." + _leaf + "." + _leaf1);
      }
    }
  }

  return ListOfLeaves;
}
