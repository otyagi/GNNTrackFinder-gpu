/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#ifndef CBM_ALGO_BASE_OPTIONS_H
#define CBM_ALGO_BASE_OPTIONS_H

#include "AlgoFairloggerCompat.h"
#include "Definitions.h"
#include "compat/Filesystem.h"
#include "util/ProfilingLevel.h"

#include <set>
#include <string>
#include <vector>

namespace cbm::algo
{

  class Options {

   public:
    Options() = default;
    Options(int argc, char** argv);

    fs::path ParamsDir() const { return fParamsDir; }
    const std::string& InputLocator() const { return fInputLocator; }
    fs::path OutputFile() const { return fOutputFile; }
#ifndef CBM_ONLINE_USE_FAIRLOGGER
    severity_level LogLevel() const { return fLogLevel; }
#endif  // Not CBM_ONLINE_USE_FAIRLOGGER
    fs::path LogFile() const { return fLogFile; }
    const std::string& Device() const { return fDevice; }
    const std::string& MonitorUri() const { return fMonitorUri; }
    const std::string& HistogramUri() const { return fHistogramUri; }
    const int32_t& HistogramHwm() const { return fHistogramHwm; }
    bool CollectAuxData() const { return fCollectAuxData; }
    const bool& CompressHistograms() const { return fCompressHistograms; }
    bool CollectKernelTimes() const { return fProfilingLevel != ProfilingNone; }
    ProfilingLevel Profiling() const { return fProfilingLevel; }
    fs::path TimingsFile() const { return fTimingsFile; }
    int NumTimeslices() const { return fNumTimeslices; }
    int SkipTimeslices() const { return fSkipTimeslices; }

    std::optional<int> NumOMPThreads() const
    {
      // omp doesn't allow negative number of threads, so we use -1 to indicate that the user didn't specify a number
      // and omp should use OMP_NUM_THREADS environment variable or the default instead
      return fNumOMPThreads > 0 ? std::make_optional(fNumOMPThreads) : std::nullopt;
    }
    const std::string& ChildId() const { return fChildId; }
    uint64_t RunId() const { return fRunId; }
    uint64_t RunStart() const { return fRunStartTime; }
    bool DumpArchive() const { return fDumpArchive; }
    bool ReleaseMode() const { return fReleaseMode; }

    const std::vector<Step>& Steps() const { return fRecoSteps; }

    const std::vector<RecoData>& OutputTypes() const { return fOutputTypes; }
    bool HasOutput(RecoData recoData) const;

    bool CompressArchive() const { return fCompressArchive; }

    const std::vector<fles::Subsystem>& Detectors() const { return fDetectors; }

    bool Has(fles::Subsystem detector) const;

    bool Has(Step step) const;

    bool Has(QaStep qastep) const;

    bool ReconstructDigiEvents() const { return fReconstructDigiEvents; }

   private:                  // members
    std::string fParamsDir;  // TODO: can we make this a std::path?
    std::string fInputLocator;
    std::string fOutputFile;
#ifndef CBM_ONLINE_USE_FAIRLOGGER
    severity_level fLogLevel;
#endif  // Not CBM_ONLINE_USE_FAIRLOGGER
    std::string fLogFile;
    std::string fDevice;
    std::string fMonitorUri;
    std::string fHistogramUri;
    int32_t fHistogramHwm;
    bool fCompressHistograms = false;
    std::vector<QaStep> fQaSteps;
    bool fDumpArchive              = false;
    bool fReleaseMode              = false;
    ProfilingLevel fProfilingLevel = ProfilingNone;
    std::string fTimingsFile;
    int fNumTimeslices  = -1;
    int fSkipTimeslices = 0;
    int fNumOMPThreads  = -1;
    std::vector<Step> fRecoSteps;
    std::vector<RecoData> fOutputTypes;
    bool fCompressArchive = false;
    std::vector<fles::Subsystem> fDetectors;
    std::string fChildId   = "00";
    uint64_t fRunId        = 2391;
    uint64_t fRunStartTime = 0;
    bool fCollectAuxData   = false;
    bool fReconstructDigiEvents = false;
  };

}  // namespace cbm::algo

#endif
