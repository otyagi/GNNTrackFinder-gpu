/* Copyright (C) 2011-2021 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Semen Lebedev, Andrey Lebedev [committer], Florian Uhlig */

/**
 * \file CbmHistManager.h
 * \brief Histogram manager.
 * \author Semen Lebedev <s.lebedev@gsi.de>
 * \date 2011
 */

#ifndef CBMHISTMANAGER_H_
#define CBMHISTMANAGER_H_

#include <Logger.h>  // for Logger, LOG

#include <Rtypes.h>      // for THashConsistencyHolder, ClassDef
#include <RtypesCore.h>  // for Double_t, Int_t, Bool_t, Option_t
#include <TCanvas.h>     // iwyu: keep for RootCling
#include <TGraph.h>      // for TGraph
#include <TGraph2D.h>    // for TGraph2D
#include <TH1.h>         // for TH1
#include <TH2.h>         // for TH2
#include <TH3.h>         // for TH3
#include <THnSparse.h>   // for THnSparse
#include <TNamed.h>      // for TNamed
#include <TObject.h>     // for TObject
#include <TProfile.h>    // for TProfile
#include <TProfile2D.h>  // for TProfile2D

#include <array>        // for array
#include <cassert>      // for assert
#include <map>          // for map, __map_const_iterator, operator!=
#include <ostream>      // for string, operator<<, ostream
#include <string>       // for operator<
#include <type_traits>  // for is_base_of
#include <utility>      // for pair, make_pair
#include <vector>       // for vector

class TFile;

/**
 * \class CbmHistManager
 * \brief Histogram manager.
 * \author Semen Lebedev <s.lebedev@gsi.de>
 * \date 2011
 */
class CbmHistManager : public TObject {
public:
  /**
    * \brief Constructor.
    */
  CbmHistManager();

  /**
    * \brief Destructor.
    */
  virtual ~CbmHistManager();

  /**
    * \brief Add new named object to manager.
    * \param[in] name Name of the object.
    * \param[in] object Pointer to object.
    */
  void Add(const std::string& name, TNamed* object)
  {

    std::map<std::string, TNamed*>::iterator it = fMap.find(name);
    if (it != fMap.end()) {
      LOG(warn) << "CbmHistManager::Add Object with name:" << name << " was already added. Set new object.";
    }

    std::pair<std::string, TNamed*> newpair = std::make_pair(name, object);
    fMap.insert(newpair);
  }

  /**
    * \brief Helper function for creation of 1-dimensional histograms and profiles.
    * Template argument is a real object type that has to be created, for example,
    * Create1<TH1F>("name", "title", 100, 0, 100);
    * \param[in] name Object name.
    * \param[in] title Object title.
    * \param[in] nofBins Number of bins.
    * \param[in] minBin Low axis limit.
    * \param[in] maxBin Upper axis limit.
    */
  template<class T>
  void Create1(const std::string& name, const std::string& title, Int_t nofBins, Double_t minBin, Double_t maxBin)
  {
    T* h = new T(name.c_str(), title.c_str(), nofBins, minBin, maxBin);
    Add(name, h);
  }

  /**
    * \brief Helper function for creation of 1-dimensional histograms and profiles.
    * Template argument is a real object type that has to be created, for example,
    * Create1<TH1F>("name", "title", binsX);
    * \param[in] name Object name.
    * \param[in] title Object title.
    * \param[in] binsX array of low-edges for each bin in X
    */
  template<class T>
  void Create1(const std::string& name, const std::string& title, const std::vector<Double_t>& binsX)
  {
    T* h = new T(name.c_str(), title.c_str(), binsX.size() - 1, binsX.data());
    Add(name, h);
  }

  /**
    * \brief Helper function for creation of 2-dimensional histograms and profiles.
    * Template argument is a real object type that has to be created, for example,
    * Create2<TH2F>("name", "title", 100, 0, 100, 200, 0, 200);
    * \param[in] name Object name.
    * \param[in] title Object title.
    * \param[in] nofBinsX Number of bins for X axis.
    * \param[in] minBinX Low X axis limit.
    * \param[in] maxBinX Upper X axis limit.
    * \param[in] nofBinsY Number of bins for Y axis.
    * \param[in] minBinY Low Y axis limit.
    * \param[in] maxBinY Upper Y axis limit.
    */
  template<class T>
  void Create2(const std::string& name, const std::string& title, Int_t nofBinsX, Double_t minBinX, Double_t maxBinX,
               Int_t nofBinsY, Double_t minBinY, Double_t maxBinY)
  {
    T* h = new T(name.c_str(), title.c_str(), nofBinsX, minBinX, maxBinX, nofBinsY, minBinY, maxBinY);
    Add(name, h);
  }

  /**
    * \brief Helper function for creation of 2-dimensional histograms and profiles.
    * Template argument is a real object type that has to be created, for example,
    * Create2<TH2F>("name", "title", binsX, binsY);
    * \param[in] name Object name.
    * \param[in] title Object title.
    * \param[in] binsX array of low-edges for each bin in X
    * \param[in] binsY array of low-edges for each bin in Y
    */
  template<class T>
  void Create2(const std::string& name, const std::string& title, const std::vector<Double_t>& binsX,
               const std::vector<Double_t>& binsY)
  {
    T* h = new T(name.c_str(), title.c_str(), binsX.size() - 1, binsX.data(), binsY.size() - 1, binsY.data());
    Add(name, h);
  }

  /**
    * \brief Helper function for creation of 3-dimensional histograms and profiles.
    * Template argument is a real object type that has to be created, for example,
    * Create3<TH3F>("name", "title", 100, 0, 100, 200, 0, 200, 300, 0, 300);
    * \param[in] name Object name.
    * \param[in] title Object title.
    * \param[in] nofBinsX Number of bins for X axis.
    * \param[in] minBinX Low X axis limit.
    * \param[in] maxBinX Upper X axis limit.
    * \param[in] nofBinsY Number of bins for Y axis.
    * \param[in] minBinY Low Y axis limit.
    * \param[in] maxBinY Upper Y axis limit.
    * \param[in] nofBinsZ Number of bins for Z axis.
    * \param[in] minBinZ Low Z axis limit.
    * \param[in] maxBinZ Upper Z axis limit.
    */
  template<class T>
  void Create3(const std::string& name, const std::string& title, Int_t nofBinsX, Double_t minBinX, Double_t maxBinX,
               Int_t nofBinsY, Double_t minBinY, Double_t maxBinY, Int_t nofBinsZ, Double_t minBinZ, Double_t maxBinZ)
  {
    T* h = new T(name.c_str(), title.c_str(), nofBinsX, minBinX, maxBinX, nofBinsY, minBinY, maxBinY, nofBinsZ, minBinZ,
                 maxBinZ);
    Add(name, h);
  }

  /**
    * \brief Helper function for creation of THnSparse objects.
    * \tparam <T> { real object type that has to be created, for example THnSparseD }
    * \tparam <nDim> { Array dimensions for nBins, minVals and maxVals }
    * CreateSparse<THnSparseD, 3>("name", "title", nDim, nBins, minVals, maxVals);
    * \param[in] name Object name.
    * \param[in] title Object title.
    * \param[in] nBins Array of size nDim with number of bins for each dimension.
    * \param[in] minVals Array of size nDim with minimum values for each dimension.
    * \param[in] maxVals Array of size nDim with maximum values for each dimension.
    */
  template<class T, int nDim>
  void CreateSparse(const std::string& name, const std::string& title, const std::array<Int_t, nDim>& nBins,
                    const std::array<Double_t, nDim>& minVals, const std::array<Double_t, nDim>& maxVals)
  {
    static_assert(std::is_base_of<THnSparse, T>::value, "Class must be derrived from THnSparse");
    T* h = new T(name.c_str(), title.c_str(), nDim, nBins.data(), minVals.data(), maxVals.data());
    Add(name, h);
  }

  TNamed* GetObject(const std::string& name) const
  {
    if (fMap.count(name) == 0) {  // Temporarily used for debugging
      LOG(error) << "CbmHistManager::GetObject(name): name=" << name;
    }
    assert(fMap.count(name) != 0);
    return fMap.find(name)->second;
  }


  /**
    * \brief Return pointer to TH1 histogram.
    * \param[in] name Name of TH1 histogram.
    * \return pointer to TH1 histogram.
    */
  TH1* H1(const std::string& name) const
  {
    if (fMap.count(name) == 0) {  // Temporarily used for debugging
      LOG(error) << "CbmHistManager::H1(name): name=" << name;
    }
    assert(fMap.count(name) != 0);
    return dynamic_cast<TH1*>(fMap.find(name)->second);
  }

  /**
    * \brief Return clone of TH1 histogram.
    * \param[in] name Name of TH1 histogram.
    * \return pointer of TH1 clone histogram.
    */
  TH1* H1Clone(const std::string& name) const { return static_cast<TH1*>(H1(name)->Clone()); }

  /**
    * \brief Return vector of pointers to TH1 histogram.
    * \param[in] names Vector of TH1 histogram names.
    * \return Vector of pointers to TH1 histogram.
    */
  std::vector<TH1*> H1Vector(const std::vector<std::string>& names) const;


  /**
    * \brief Return vector of pointers to TH1 histogram.
    * \param[in] pattern Regex for TH1 histogram name.
    * \return Vector of pointers to TH1 histogram.
    */
  std::vector<TH1*> H1Vector(const std::string& pattern) const;

  /**
    * \brief Return pointer to TH2 histogram.
    * \param[in] name Name of TH2 histogram.
    * \return pointer to TH2 histogram.
    */
  TH2* H2(const std::string& name) const
  {
    if (fMap.count(name) == 0) {  // Temporarily used for debugging
      LOG(error) << "CbmHistManager::H2(name): name=" << name;
    }
    assert(fMap.count(name) != 0);
    return dynamic_cast<TH2*>(fMap.find(name)->second);
  }

  /**
    * \brief Return clone of TH2 histogram.
    * \param[in] name Name of TH2 histogram.
    * \return pointer of TH2 clone histogram.
    */
  TH2* H2Clone(const std::string& name) const { return static_cast<TH2*>(H2(name)->Clone()); }

  /**
    * \brief Return vector of pointers to TH2 histogram.
    * \param[in] names Vector of TH2 histogram names.
    * \return Vector of pointers to TH2 histogram.
    */
  std::vector<TH2*> H2Vector(const std::vector<std::string>& names) const;

  /**
    * \brief Return vector of pointers to TH2 histogram.
    * \param[in] pattern Regex for TH2 histogram name.
    * \return Vector of pointers to TH2 histogram.
    */
  std::vector<TH2*> H2Vector(const std::string& pattern) const;

  /**
    * \brief Return pointer to TH3 histogram.
    * \param[in] name Name of TH3 histogram.
    * \return pointer to TH3 histogram.
    */
  TH3* H3(const std::string& name) const
  {
    if (fMap.count(name) == 0) {  // Temporarily used for debugging
      LOG(error) << "CbmHistManager::H3(name): name=" << name;
    }
    assert(fMap.count(name) != 0);
    return dynamic_cast<TH3*>(fMap.find(name)->second);
  }

  /**
    * \brief Return clone of TH3 histogram.
    * \param[in] name Name of TH3 histogram.
    * \return pointer of TH3 clone histogram.
    */
  TH3* H3Clone(const std::string& name) const { return static_cast<TH3*>(H3(name)->Clone()); }

  /**
    * \brief Return pointer to THnSparse histogram.
    * \param[in] name Name of THnSparse histogram.
    * \return pointer to THnSparse histogram.
    */
  THnSparse* HnSparse(const std::string& name) const
  {
    if (fMap.count(name) == 0) {  // Temporarily used for debugging
      LOG(error) << "CbmHistManager::HnSparse(name): name=" << name;
    }
    assert(fMap.count(name) != 0);
    return dynamic_cast<THnSparse*>(fMap.find(name)->second);
  }

  /**
    * \brief Return pointer to TGraph.
    * \param[in] name Name of TGraph.
    * \return pointer to TGraph.
    */
  TGraph* G1(const std::string& name) const
  {
    if (fMap.count(name) == 0) {  // Temporarily used for debugging
      LOG(error) << "CbmHistManager::G1(name): name=" << name;
    }
    assert(fMap.count(name) != 0);
    return dynamic_cast<TGraph*>(fMap.find(name)->second);
  }

  /**
    * \brief Return vector of pointers to TGraph.
    * \param[in] names Vector of TGraph names.
    * \return Vector of pointers to TGraph.
    */
  std::vector<TGraph*> G1Vector(const std::vector<std::string>& names) const;

  /**
    * \brief Return vector of pointers to TGraph.
    * \param[in] pattern Regex for TGraph name.
    * \return Vector of pointers to TGraph.
    */
  std::vector<TGraph*> G1Vector(const std::string& pattern) const;

  /**
    * \brief Return pointer to TGraph2D.
    * \param[in] name Name of TGraph2D.
    * \return pointer to TGraph2D.
    */
  TGraph2D* G2(const std::string& name) const
  {
    if (fMap.count(name) == 0) {  // Temporarily used for debugging
      LOG(error) << "CbmHistManager::G2(name): name=" << name;
    }
    assert(fMap.count(name) != 0);
    return dynamic_cast<TGraph2D*>(fMap.find(name)->second);
  }

  /**
    * \brief Return vector of pointers to TGraph2D.
    * \param[in] names Vector of TGraph2D names.
    * \return Vector of pointers to TGraph2D.
    */
  std::vector<TGraph2D*> G2Vector(const std::vector<std::string>& names) const;

  /**
    * \brief Return vector of pointers to TGraph2D.
    * \param[in] pattern Regex for TGraph2D name.
    * \return Vector of pointers to TGraph2D.
    */
  std::vector<TGraph2D*> G2Vector(const std::string& pattern) const;

  /**
    * \brief Return pointer to TProfile.
    * \param[in] name Name of TProfile.
    * \return pointer to TProfile.
    */
  TProfile* P1(const std::string& name) const
  {
    if (fMap.count(name) == 0) {  // Temporarily used for debugging
      LOG(error) << "CbmHistManager::P1(name): name=" << name;
    }
    assert(fMap.count(name) != 0);
    return dynamic_cast<TProfile*>(fMap.find(name)->second);
  }

  /**
    * \brief Return vector of pointers to TProfile.
    * \param[in] names Vector of TProfile names.
    * \return Vector of pointers to TProfile.
    */
  std::vector<TProfile*> P1Vector(const std::vector<std::string>& names) const;

  /**
    * \brief Return vector of pointers to TProfile.
    * \param[in] pattern Regex for TProfile name.
    * \return Vector of pointers to TProfile.
    */
  std::vector<TProfile*> P1Vector(const std::string& pattern) const;

  /**
    * \brief Return pointer to TProfile2D.
    * \param[in] name Name of TProfile2D.
    * \return pointer to TProfile2D.
    */
  TProfile2D* P2(const std::string& name) const
  {
    if (fMap.count(name) == 0) {  // Temporarily used for debugging
      LOG(error) << "CbmHistManager::P2(name): name=" << name;
    }
    assert(fMap.count(name) != 0);
    return dynamic_cast<TProfile2D*>(fMap.find(name)->second);
  }

  /**
    * \brief Return vector of pointers to TProfile2D.
    * \param[in] names Vector of TProfile2D names.
    * \return Vector of pointers to TProfile2D.
    */
  std::vector<TProfile2D*> P2Vector(const std::vector<std::string>& names) const;

  /**
    * \brief Return vector of pointers to TProfile2D.
    * \param[in] pattern Regex for TProfile2D name.
    * \return Vector of pointers to TProfile2D.
    */
  std::vector<TProfile2D*> P2Vector(const std::string& pattern) const;

  /**
    * \brief Check existence of object in manager.
    * \param[in] name Name of object.
    * \return True if object exists in manager.
    */
  Bool_t Exists(const std::string& name) const { return (fMap.count(name) == 0) ? false : true; }

  /**
    * \brief Write all objects to current opened file.
    */
  void WriteToFile();

  /**
    * \brief Write all canvas to current opened file.
    */
  void WriteCanvasToFile();

  /**
    * \brief Read histograms from file.
    * \param[in] file Pointer to file with histograms.
    */
  void ReadFromFile(TFile* file);

  /**
    * \brief Add TName object to map. Used in ReadFromFile method.
    */
  void AddTNamedObject(TObject* obj);

  /**
    * \brief Add all TName objects to map in directory. Used in ReadFromFile method.
    */
  void AddTDirectoryObject(TObject* obj);

  /**
    * \brief Clear memory. Remove all histograms and canvases.
    */
  void Clear(Option_t* = "");

  /**
    * \brief Shrink empty bins in H1.
    * \param[in] histName Name of histogram.
    */
  void ShrinkEmptyBinsH1(const std::string& histName);

  /**
    * \brief Shrink empty bins in H1.
    * \param[in] histPatternName Regular expression for histogram name.
    */
  void ShrinkEmptyBinsH1ByPattern(const std::string& pattern);

  /**
    * \brief Shrink empty bins in H2.
    * \param[in] histName Name of histogram.
    */
  void ShrinkEmptyBinsH2(const std::string& histName);

  /**
    * \brief Shrink empty bins in H2.
    * \param[in] histPatternName Regular expression for histogram name.
    */
  void ShrinkEmptyBinsH2ByPattern(const std::string& pattern);

  /**
    * \brief Scale histogram.
    * \param[in] histName Name of histogram.
    * \param[in] scale Scaling factor.
    */
  void Scale(const std::string& histName, Double_t scale);

  /**
    * \brief Scale histograms which name matches specified pattern.
    * \param[in] histPatternName Regular expression for histogram name.
    * \param[in] scale Scaling factor.
    */
  void ScaleByPattern(const std::string& pattern, Double_t scale);

  /**
    * \brief Normalize histogram to integral.
    * \param[in] histName Name of histogram.
    */
  void NormalizeToIntegral(const std::string& histName);

  /**
    * \brief Normalize histograms to integral which name matches specified pattern.
    * \param[in] histPatternName Regular expression for histogram name.
    */
  void NormalizeToIntegralByPattern(const std::string& pattern);

  /**
    * \brief Rebin histogram.
    * \param[in] histName Name of histogram.
    * \param[in] ngroup Rebining factor.
    */
  void Rebin(const std::string& histName, Int_t ngroup);

  /**
    * \brief Rebin histograms which name matches specified pattern.
    * \param[in] histPatternName Regular expression for histogram name.
    * \param[in] ngroup Rebining factor.
    */
  void RebinByPattern(const std::string& pattern, Int_t ngroup);

  /**
    * \brief Return string representation of class.
    * \return string representation of class.
    */
  std::string ToString() const;

  /**
    * \brief Operator << for convenient output to std::ostream.
    * \return Insertion stream in order to be able to call a succession of insertion operations.
    */
  friend std::ostream& operator<<(std::ostream& strm, const CbmHistManager& histManager)
  {
    strm << histManager.ToString();
    return strm;
  }

  /**
    * \brief Create and draw TCanvas and store pointer to it.
    * \param[in] name Name of the canvas.
    * \param[in] title Title of the canvas.
    * \param[in] width Width of the canvas.
    * \param[in] height Height of the canvas.
    * \return Pointer to the created canvas.
    */
  TCanvas* CreateCanvas(const std::string& name, const std::string& title, Int_t width, Int_t height);

  /**
    * \brief Save all stored canvases to images.
    * \param[in] outputDir Path to the output directory (could be relative path).
    * \param[in] options You can specify image format: eps, png or gif. Example: "gif,eps,png".
    */
  void SaveCanvasToImage(const std::string& outputDir, const std::string& options = "png,eps");

private:
  template<class T>
  std::vector<T> ObjectVector(const std::string& pattern) const;

  template<class T>
  std::vector<T> ObjectVector(const std::vector<std::string>& names) const;

  std::map<std::string, TNamed*> fMap;  // Map of histogram (graph) name to its pointer
  std::vector<TCanvas*> fCanvases;      // Pointers to all created canvases

  ClassDef(CbmHistManager, 1)
};

#endif /* CBMHISTMANAGER_H_ */
