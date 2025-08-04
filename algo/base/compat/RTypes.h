/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#pragma once

/**
 * @file RTypes.h
 * @brief Compatibility header for basic ROOT macros.
**/

#if __has_include(<Rtypes.h>)
#include <Rtypes.h>
#else
#define BIT(n) (1ULL << (n))
#define SETBIT(n, i) ((n) |= BIT(i))
#define CLRBIT(n, i) ((n) &= ~BIT(i))
#define TESTBIT(n, i) ((bool) (((n) &BIT(i)) != 0))
#endif
