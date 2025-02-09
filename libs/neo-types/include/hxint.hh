/**
 * @file inttypes.hh
 * @brief Header file that provides enhanced integer types and macros for better
 * readability and compatibility.
 *
 * This file, inttypes.hh, is an improved version of the standard cstdint
 * header file, modified for enhanced readability and compatibility with Helix
 * and other projects. It includes standard integer types along with fast and
 * maximum size types, providing a broader range of signed and unsigned integers
 * with specific width guarantees.
 *
 * The definitions align closely with those in the Windows SDK, ensuring that
 * the macros match exactly for compatibility. The file conditionally compiles
 * different macros and type definitions based on the compiler and platform,
 * ensuring optimal performance and size compatibility across different systems
 * (e.g., 32-bit vs. 64-bit).
 *
 * Modifications from the original Visual C++ runtime library include
 * simplifications and adjustments to meet the specific needs of the Helix
 * compiler environment. The copyright belongs to Microsoft Corporation, with
 * the file re-licensed under Apache-2.0 WITH LLVM-exception for broader use
 * within the community.
 *
 * Helix License: CC Attribution 4.0 International
 * @see http://creativecommons.org/licenses/by/4.0/
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 * Original Code Credit: Microsoft Corporation
 *
 * @author Dhruvan Kartik
 * @original-author Microsoft Corporation
 * @doc generated by ChatGPT
 * This work is licensed under the Creative Commons Attribution 4.0
 * International License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by/4.0/ or send a letter to Creative
 * Commons, PO Box 1866, Mountain View, CA 94042, USA.
 * @see http://creativecommons.org/licenses/by/4.0/
 */
#pragma once

#include <cstdint>

using i8   = int8_t;
using i16  = int16_t;
using i32  = int32_t;
using i64  = int64_t;
using i128 = long long int;
using u8   = uint8_t;
using u16  = uint16_t;
using u32  = uint32_t;
using u64  = uint64_t;
using u128 = unsigned long long int;

using i8_fast   = int_fast8_t;
using i16_fast  = int_fast16_t;
using i32_fast  = int_fast32_t;
using i64_fast  = int_fast64_t;
using u8_fast   = uint_fast8_t;
using u16_fast  = uint_fast16_t;
using u32_fast  = uint_fast32_t;
using u64_fast  = uint_fast64_t;

using isize_max = intmax_t;
using usize_max = uintmax_t;