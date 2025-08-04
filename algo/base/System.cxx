/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#include "System.h"

#include <cstdio>

#ifdef __linux__
#include <sys/resource.h>
#include <unistd.h>
#endif


size_t cbm::algo::GetCurrentRSS()
{
  // Implementation copied from https://stackoverflow.com/a/14927379
#ifndef __linux__
  return 0;
#else
  unsigned long rss = 0L;
  FILE* fp          = nullptr;
  if ((fp = fopen("/proc/self/statm", "r")) == nullptr) {
    return size_t(0L); /* Can't open? */
  }
  if (fscanf(fp, "%*s%lu", &rss) != 1) {
    fclose(fp);
    return size_t(0L); /* Can't read? */
  }
  fclose(fp);
  return size_t(rss) * size_t(sysconf(_SC_PAGESIZE));
#endif
}

size_t cbm::algo::GetPeakRSS()
{
  // Implementation copied from https://stackoverflow.com/a/14927379
#ifndef __linux__
  return 0;
#else
  struct rusage rusage;
  getrusage(RUSAGE_SELF, &rusage);

  return size_t(rusage.ru_maxrss * 1024L);
#endif
}
