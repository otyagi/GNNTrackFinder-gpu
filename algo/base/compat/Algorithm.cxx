/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#include "Algorithm.h"


task_thread_pool::task_thread_pool& cbm::algo::GetGlobalSTLThreadPool()
{
  static task_thread_pool::task_thread_pool pool;
  return pool;
}
