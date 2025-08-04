/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#include "Options.h"

#include "util/StlUtils.h"

#include <boost/program_options.hpp>

#include <iostream>
#include <iterator>
#include <unordered_map>

using namespace cbm::algo;
namespace po = boost::program_options;

using fles::Subsystem;


namespace std
{
  template<class T>
  std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
  {
    copy(v.begin(), v.end(), std::ostream_iterator<T>(os, " "));
    return os;
  }
}  // namespace std

#ifndef CBM_ONLINE_USE_FAIRLOGGER
void validate(boost::any& v, const std::vector<std::string>& values, severity_level*, int)
{

  static const std::unordered_map<std::string, severity_level> levels{
    {"trace", severity_level::trace}, {"debug", severity_level::debug},     {"status", severity_level::status},
    {"info", severity_level::info},   {"warning", severity_level::warning}, {"error", severity_level::error},
    {"fatal", severity_level::fatal}};

  po::validators::check_first_occurrence(v);

  const std::string& s = po::validators::get_single_string(values);

  auto it = levels.find(s);

  if (it == levels.end()) throw po::validation_error(po::validation_error::invalid_option_value);

  v = it->second;
}
#endif  // Not CBM_ONLINE_USE_FAIRLOGGER

Options::Options(int argc, char** argv)
{

  po::options_description required("Required options");
  // clang-format off
  required.add_options()
    ("param-dir,p", po::value(&fParamsDir)->value_name("<folder>")->required(),
      "read program options from this folder")
    ("input-locator,i", po::value(&fInputLocator)->value_name("<locator>")->required(),
      "URI specifying input timeslice source")
  ;
  // clang-format on

  po::options_description generic("Other options");
  // clang-format off
  generic.add_options()
    ("output,o", po::value(&fOutputFile)->default_value("")->value_name("<file>"),
      "write results to file")
    ("device,d", po::value(&fDevice)->default_value("cpu")->value_name("<device>"),
      "select device (cpu, cuda0, cuda1, hip0, ...)")
#ifndef CBM_ONLINE_USE_FAIRLOGGER
    ("log-level,l", po::value(&fLogLevel)->default_value(info)->value_name("<level>"),
      "set log level (debug, info, warning, error, fatal)")
#endif  // Not CBM_ONLINE_USE_FAIRLOGGER
    ("monitor,m", po::value(&fMonitorUri)->value_name("<uri>")->implicit_value("file:cout"),
      "URI specifying monitor output (e.g. file:/tmp/monitor.txt, influx1:login:8086:cbmreco_status). Prints to cout when no argument is given. Monitor is disabled when flag is not set.")
    ("histogram", po::value(&fHistogramUri)->value_name("<uri>"), "URI to specify histogram server")
    ("histoshwm", po::value(&fHistogramHwm)->default_value(1)->value_name("<num>"),
     "High-Water Mark for ZMQ socket to histogram server in messages:\n"
     " 0 = no buffering, num = nb updates kept in buffer if not pulled by server \n"
     " Tune to avoid too high memory usage but also adapt to server load!")
    ("aux-data", po::value(&fCollectAuxData)->implicit_value(true), "Enables collecting of auxiliary data from algorithms")
    ("qa", po::value(&fQaSteps)->multitoken()->default_value({QaStep::UnpackSts, QaStep::EventBuilding, QaStep::Tracking})->value_name("<qa steps>"),
      "space separated list of QA Steps to enable (BeamBmon, UnpackSts, EventBuilding, Tracking, ...)")
#ifdef BOOST_IOS_HAS_ZSTD
    ("hist-compr", po::bool_switch(&fCompressHistograms)->default_value(false),
      "enables ZSTD compression of the outgoing histograms stream (decompression needed in target server!)")
#endif
    ("log-file,L", po::value(&fLogFile)->value_name("<file>"),
      "write log messages to file")
    ("output-types,O", po::value(&fOutputTypes)->multitoken()->value_name("<types>"),
      "space separated list of reconstruction output types (Hit, Tracks, DigiTimeslice, DigiEvent, ...)")
    ("compress-archive", po::bool_switch(&fCompressArchive)->default_value(false), "Enable compression for output archives")
    ("steps", po::value(&fRecoSteps)->multitoken()->default_value({Step::Unpack, Step::DigiTrigger, Step::LocalReco, Step::Tracking})->value_name("<steps>"),
      "space separated list of reconstruction steps (unpack, digitrigger, localreco, ...)")
    ("event-reco", po::bool_switch(&fReconstructDigiEvents)->default_value(false), "runs digi event reconstruction (local reco, tracking, trigger)")
    ("systems,s", po::value(&fDetectors)->multitoken()->default_value({Subsystem::STS, Subsystem::TOF, Subsystem::BMON, Subsystem::MUCH, Subsystem::RICH, Subsystem::TRD, Subsystem::TRD2D})->value_name("<detectors>"),
      "space separated list of detectors to process (sts, mvd, ...)")
    ("child-id,c", po::value(&fChildId)->default_value("00")->value_name("<id>"), "online process id on node")
    ("run-id,r", po::value(&fRunId)->default_value(2391)->value_name("<RunId>"), "Run ID, for now flesctl run index, later run start time")
    ("run-start", po::value(&fRunStartTime)->default_value(0)->value_name("<RunStart >"), "Run start time in ns, can be fles start or online start")
    ("num-ts,n", po::value(&fNumTimeslices)->default_value(-1)->value_name("<num>"),
      "Stop after <num> timeslices (-1 = all)")
    ("skip-ts", po::value(&fSkipTimeslices)->default_value(0)->value_name("<num>"),
      "Skip first <num> timeslices")
    ("omp", po::value(&fNumOMPThreads)->default_value(-1)->value_name("<num>"),
      "Set number of OpenMP threads (-1 = use OMP_NUM_THREADS environment variable)")
    ("times,t", po::value(&fProfilingLevel)->default_value(ProfilingNone)->implicit_value(ProfilingPerTS),
      "Print kernel times (Can opt. be given a value: Use none to disable or summary to only print aggregated times.)")
    ("timings-file", po::value(&fTimingsFile)->value_name("<file>"),
      "Write profiling times to yaml file (only when '-t' is set)")
    ("dump-archive", po::bool_switch(&fDumpArchive)->default_value(false),
      "Dump archive content to stdout and exit. Provide archive with '-i'. (This is a hack to quick check archive content until we have proper tooling.)")
    ("release-mode,R",po::value<bool>(&fReleaseMode)->implicit_value(true),
      "Copy and release each timeslice immediately after receiving it")
    ("help,h",
      "produce help message")
  ;
  // clang-format on

  po::options_description cmdline_options;
  cmdline_options.add(required).add(generic);

  po::variables_map vm;
  po::command_line_parser parser{argc, argv};
  parser.options(cmdline_options);
  try {
    auto result = parser.run();
    po::store(result, vm);
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::cerr << "Use '-h' to display all valid options." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  if (vm.count("help") > 0) {
    std::cout << cmdline_options << std::endl;
    std::exit(EXIT_SUCCESS);
  }

  try {
    po::notify(vm);
  }
  catch (const po::required_option& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::cerr << "Use '-h' to display all valid options." << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

bool Options::HasOutput(RecoData recoData) const { return Contains(fOutputTypes, recoData); }

bool Options::Has(fles::Subsystem detector) const { return Contains(fDetectors, detector); }

bool Options::Has(Step step) const { return Contains(fRecoSteps, step); }

bool Options::Has(QaStep qastep) const { return Contains(fQaSteps, qastep); }
