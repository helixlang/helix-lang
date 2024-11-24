///--- The Helix Project ------------------------------------------------------------------------///
///                                                                                              ///
///   Part of the Helix Project, under the Attribution 4.0 International license (CC BY 4.0).    ///
///   You are allowed to use, modify, redistribute, and create derivative works, even for        ///
///   commercial purposes, provided that you give appropriate credit, and indicate if changes    ///
///   were made.                                                                                 ///
///                                                                                              ///
///   For more information on the license terms and requirements, please visit:                  ///
///     https://creativecommons.org/licenses/by/4.0/                                             ///
///                                                                                              ///
///   SPDX-License-Identifier: CC-BY-4.0                                                         ///
///   Copyright (c) 2024 The Helix Project (CC BY 4.0)                                           ///
///                                                                                              ///
///------------------------------------------------------------------------------------ Helix ---///
///                                                                                              ///
///  The process of this file is special, the helix compiler will invoke this file first using   ///
///  the llvm/clang toolchain, this file will then be preprocessed and de-tokenized, then the    ///
///  helix compiler will use the generated file to compile the rest of the helix code.           ///
///                                                                                              ///
///  This file is the entry point for the lib-helix runtime (tho. staticly linked), if this file ///
///  is not present, the helix compiler will not be able to compile any helix code.              ///
///                                                                                              ///
///  WARNING: Do not remove this file, it is required for the helix compiler to work properly.   ///
///                                                                                              ///
///----------------------------------------------------------------------------------------------///

#ifndef __$LIBHELIX_CORE__
#define __$LIBHELIX_CORE__

#include "cast.h"
#include "concepts.h"
#include "config.h"
#include "dtypes.h"
#include "finally.h"
#include "generator.h"
#include "libc.h"
#include "libcxx.h"
#include "print.h"
#include "refs.h"
#include "traits.h"
#include "types.h"
#include "question.h"

#endif