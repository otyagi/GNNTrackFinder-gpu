/* Copyright (C) 2020 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#include "CbmFormatTsPrintout.h"

#include "CbmFormatMsBufferPrintout.h"
#include "CbmFormatMsHeaderPrintout.h"

#include <ios>

std::string FormatTsHeaderPrintout(const fles::Timeslice& ts)
{
  std::stringstream ss;

  uint64_t min_num_microslices   = UINT64_MAX;
  uint64_t max_num_microslices   = 0;
  uint64_t total_num_microslices = 0;
  uint64_t min_microslice_size   = UINT64_MAX;
  uint64_t max_microslice_size   = 0;
  uint64_t total_microslice_size = 0;

  size_t nbComps           = ts.num_components();
  size_t nbMicroslicesCore = ts.num_core_microslices();
  for (uint64_t compIdx = 0; compIdx < nbComps; ++compIdx) {
    uint64_t num_microslices = ts.num_microslices(compIdx);
    min_num_microslices      = std::min(min_num_microslices, num_microslices);
    max_num_microslices      = std::max(max_num_microslices, num_microslices);
    total_num_microslices += num_microslices;
    for (uint64_t msIdx = 0; msIdx < num_microslices; ++msIdx) {
      uint64_t size = ts.descriptor(compIdx, msIdx).size;
      total_microslice_size += size;
      min_microslice_size = std::min(min_microslice_size, size);
      max_microslice_size = std::max(max_microslice_size, size);
    }  // for( uint64_t msIdx = 0; msIdx < num_microslices; ++msIdx )
  }    // for( uint64_t compIdx = 0; compIdx < nbComps; ++compIdx )

  uint64_t min_overlap = min_num_microslices - nbMicroslicesCore;
  uint64_t max_overlap = max_num_microslices - nbMicroslicesCore;

  ss << "Timeslice " << std::setw(6) << ts.index() << " with " << std::setw(3) << nbComps << " components "
     << " x " << std::setw(6) << nbMicroslicesCore << " core microslices";
  if (0 != nbComps) {
    ss << " (+";
    if (min_overlap != max_overlap) {
      ss << std::setw(3) << min_overlap << std::setw(3) << ".." << max_overlap;
    }  // if( min_overlap != max_overlap )
    else {
      ss << std::setw(3) << min_overlap;
    }  // else of if( min_overlap != max_overlap )
    ss << " overlap) = " << std::setw(9) << total_num_microslices << "\n";
    ss
      << "\tmicroslice size min/avg/max: " << std::setw(6) << min_microslice_size << " / " << std::fixed
      << std::setprecision(0) << std::setw(6)
      << (static_cast<double>(total_microslice_size) / total_num_microslices)
      //         << std::defaultfloat // commented out as not included in GCC 4.9.2 => restore defaults, probably not needed
      << std::setprecision(6)  // restore defaults, probably not needed
      << " / " << std::setw(6) << max_microslice_size << " bytes"
      << "\n";
  }  // if( 0 != nbComps )
  else {
    ss << " = " << std::setw(9) << total_num_microslices << "\n";
  }

  return ss.str();
}

std::string FormatTsContentPrintout(const fles::Timeslice& ts, std::underlying_type_t<fles::Subsystem> selSysId,
                                    size_t nbMsPerComp)
{
  std::stringstream ss;

  size_t nbComps = ts.num_components();
  if (0 != nbComps) {
    size_t nbMicroslicesCore    = ts.num_core_microslices();
    size_t nbMicroslicesOverlap = ts.num_microslices(0) - ts.num_core_microslices();
    size_t nbCoreMsToLoop       = nbMicroslicesCore;
    size_t nbOverMsToLoop       = nbMicroslicesOverlap;
    if (nbMsPerComp < nbMicroslicesCore) {
      nbCoreMsToLoop = nbMsPerComp;
      nbOverMsToLoop = 0;
    }  // if (nbMsPerComp < nbMicroslicesCore)
    else if (nbMsPerComp < nbMicroslicesCore + nbMicroslicesOverlap) {
      nbOverMsToLoop = nbMicroslicesOverlap - (nbMsPerComp - nbMicroslicesCore);
    }  // else if (nbMsPerComp < nbMicroslicesCore + nbMicroslicesOverlap)

    if (0 < nbMicroslicesCore) {
      for (size_t compIdx = 0; compIdx < nbComps; ++compIdx) {
        if (0 < nbMicroslicesCore && 0x00 != selSysId && ts.descriptor(compIdx, 0).sys_id != selSysId) {
          // FIXME: define "reserved/undefined" system ID somewhere (best in flesnet microslice descriptor header)
          continue;
        }  // if (0 < nbMicroslicesCore && 0x00 != selSysId && ts.descriptor(compIdx, 0).sys_id != selSysId)
        // FIXME: Need safe accessor with range check for the cast, to be done in flesnet side
        ss << "Component " << std::setw(3) << compIdx << ", Subsystem "
           << fles::to_string(static_cast<fles::Subsystem>(ts.descriptor(compIdx, 0).sys_id)) << "\n";
        if (0 < nbCoreMsToLoop) {
          ss << "Core Microslices for component " << std::setw(3) << compIdx << " ("
             << fles::to_string(static_cast<fles::Subsystem>(ts.descriptor(compIdx, 0).sys_id)) << ")"
             << "\n";
          for (size_t msIdx = 0; msIdx < nbCoreMsToLoop; ++msIdx) {
            ss << ts.descriptor(compIdx, msIdx) << "\n";
            ss << FormatMsBufferPrintout(ts, compIdx, msIdx);
            ss << "----------------------------------------------"
               << "\n";
          }  // for( size_t msIdx = 0; msIdx < nbOverMsToLoop; ++msIdx )
        }    // if (0 < nbCoreMsToLoop )
        if (0 < nbOverMsToLoop) {
          ss << "Overlap Microslices for component " << std::setw(3) << compIdx << " ("
             << fles::to_string(static_cast<fles::Subsystem>(ts.descriptor(compIdx, 0).sys_id)) << ")"
             << "\n";
          for (size_t msIdx = 0; msIdx < nbOverMsToLoop; ++msIdx) {
            ss << ts.descriptor(compIdx, msIdx + nbMicroslicesCore) << "\n";
            ss << FormatMsBufferPrintout(ts, compIdx, msIdx + nbMicroslicesCore);
            ss << "----------------------------------------------"
               << "\n";
          }  // for( size_t msIdx = 0; msIdx < nbOverMsToLoop; ++msIdx )
        }    // if (0 < nbOverMsToLoop )
        ss << "++++++++++++++++++++++++++++++++++++++++++++++"
           << "\n";
      }  // for( size_t comp = 0; comp < nbComps; ++comp )
    }    // if (0 < nbMicroslicesCore) )
  }      // if( 0 != nbComps )
  ss << "**********************************************"
     << "\n";

  return ss.str();
}

std::string FormatTsPrintout(const fles::Timeslice& ts, std::underlying_type_t<fles::Subsystem> selSysId,
                             size_t nbMsPerComp)
{
  std::stringstream ss;
  ss << FormatTsHeaderPrintout(ts);
  ss << FormatTsContentPrintout(ts, selSysId, nbMsPerComp);
  return ss.str();
}

std::ostream& operator<<(std::ostream& os, const fles::Timeslice& ts)
{
  os << FormatTsHeaderPrintout(ts);
  os << FormatTsContentPrintout(ts);
  return os;
}
