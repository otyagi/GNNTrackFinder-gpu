/* Copyright (C) 2019 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#include "CbmFormatMsHeaderPrintout.h"

std::string FormatMsHeaderPrintout(const fles::MicrosliceDescriptor& msDescriptor)
{
  std::stringstream ss;
  ss << "hi hv eqid flag si sv idx/start        crc      size     offset"
     << "\n"
     << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(msDescriptor.hdr_id) << " "
     << std::setw(2) << static_cast<unsigned int>(msDescriptor.hdr_ver) << " " << std::setw(4) << msDescriptor.eq_id
     << " " << std::setw(4) << msDescriptor.flags << " " << std::setw(2)
     << static_cast<unsigned int>(msDescriptor.sys_id) << " " << std::setw(2)
     << static_cast<unsigned int>(msDescriptor.sys_ver) << " " << std::setw(16) << msDescriptor.idx << " "
     << std::setw(8) << msDescriptor.crc << " " << std::setw(8) << msDescriptor.size << " " << std::setw(16)
     << msDescriptor.offset;
  return ss.str();
}

std::string FormatMsHeaderHelp()
{
  std::stringstream ss;
  ss << " Description of the microslice header format:\n"
     << "==> hdr_id = Header format identifier (0xDD)                             \n"
     << "|  ==> hdr_ver = Header format version (0x01)                            \n"
     << "|  |   ==> eq_id = Equipment identifier                                  \n"
     << "|  |   |    ==> flags = Status and error flags                           \n"
     << "|  |   |    |   ==> sys_id = Subsystem identifier                        \n"
     << "|  |   |    |   |  ==> sys_ver = Subsystem format/version                \n"
     << "|  |   |    |   |  |      ==> idx = Microslice index / start time        \n"
     << "|  |   |    |   |  |      |                                              \n"
     << "hi hv eqid flag si sv idx/start        crc      size     offset          \n"
     << "dd 01 3001 0000 30 03 180c1f234b1b6000 00000000 00000008 0000000000000080\n"
     << "                                        |        |         |             \n"
     << "        crc = CRC-32C of data content <==        |         |             \n"
     << "              (Castagnoli polynomial)            |         |             \n"
     << "                   size = Content size (bytes) <==         |             \n"
     << "                 offset = Offset in event buffer (bytes) <==             \n";
  return ss.str();
}

std::ostream& operator<<(std::ostream& os, const fles::MicrosliceDescriptor& msDescriptor)
{
  return os << FormatMsHeaderPrintout(msDescriptor);
}
