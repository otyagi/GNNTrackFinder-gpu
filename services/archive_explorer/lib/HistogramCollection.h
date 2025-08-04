/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

class TH1;

namespace cbm::algo
{
  class StorableRecoResults;
}

namespace cbm::explore
{

  struct Folder {
    std::string path;
    std::string name;
  };

  struct Histo {
    std::string path;
    TH1* histo;
  };

  struct HistoData {
    algo::StorableRecoResults* data = nullptr;
    std::optional<uint32_t> sensor;

    bool Skip(uint32_t address) const
    {
      if (not sensor) return false;
      return address != *sensor;
    }
  };

  class HistogramCollection {

  public:
    HistogramCollection() = default;
    virtual ~HistogramCollection();

    /// @brief Clear all histograms.
    void Reset();

    TH1* GetHisto(const std::string& name) const;
    std::string GetHistoPath(const std::string& name) const;
    std::vector<Histo> GetHistos() const { return fHistos; }

    std::vector<Folder> GetFolders() const { return fFolders; }

    void Div(const HistogramCollection& other);

    virtual void FillHistos(HistoData fill) = 0;

  protected:
    template<typename Histo_t>
    void CreateHisto(Histo_t*& histo, const char* folder, const char* name, const char* title, int nbins, double xlow,
                     double xmax)
    {
      histo = new Histo_t(name, title, nbins, xlow, xmax);
      histo->SetDirectory(nullptr);
      fHistos.push_back({.path = folder, .histo = histo});
    }

    void CreateFolder(const char* path, const char* name);

  private:
    std::vector<Folder> fFolders;
    std::vector<Histo> fHistos;
  };

}  // namespace cbm::explore
