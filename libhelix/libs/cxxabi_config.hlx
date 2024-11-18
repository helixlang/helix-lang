//===----------------------------------------------------------------------===//
//
// Port of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

eval if !defined!(____CXXABI_CONFIG_H) {
    define ____CXXABI_CONFIG_H;

    eval if defined(__arm__) && !defined(__USING_SJLJ_EXCEPTIONS__) && !defined(__ARM_DWARF_EH__) && !defined(__SEH__) {
        ffi define _LIBCXXABI_ARM_EHABI;
    }

    eval if !defined(__has_attribute) {
        ffi define __has_attribute!(_attribute_): 0;
    }

    eval if defined(__clang__) {
        ffi define _LIBCXXABI_COMPILER_CLANG;
    } else if !defined(__apple_build_version__) {
        ffi define _LIBCXXABI_CLANG_VER: (__clang_major__ * 100 + __clang_minor__);
    } else if defined(__GNUC__) {
        ffi define _LIBCXXABI_COMPILER_GCC;
    } else if defined(_MSC_VER) {
        ffi define _LIBCXXABI_COMPILER_MSVC;
    } else if defined(__IBMCPP__) {
        ffi define _LIBCXXABI_COMPILER_IBM;
    }

    eval if defined(_WIN32) {
        eval if defined(_LIBCXXABI_DISABLE_VISIBILITY_ANNOTATIONS) || (defined(__MINGW32__) && !defined(_LIBCXXABI_BUILDING_LIBRARY)) {
            ffi define _LIBCXXABI_HIDDEN;
            ffi define _LIBCXXABI_DATA_VIS;
            ffi define _LIBCXXABI_FUNC_VIS;
            ffi define _LIBCXXABI_TYPE_VIS;
        } else if defined(_LIBCXXABI_BUILDING_LIBRARY) {
            ffi define _LIBCXXABI_HIDDEN;
            ffi define _LIBCXXABI_DATA_VIS: __declspec(dllexport);
            ffi define _LIBCXXABI_FUNC_VIS: __declspec(dllexport);
            ffi define _LIBCXXABI_TYPE_VIS: __declspec(dllexport);
        } else {
            ffi define _LIBCXXABI_HIDDEN;
            ffi define _LIBCXXABI_DATA_VIS: __declspec(dllimport);
            ffi define _LIBCXXABI_FUNC_VIS: __declspec(dllimport);
            ffi define _LIBCXXABI_TYPE_VIS: __declspec(dllimport);
        }
    } else {
        eval if !defined(_LIBCXXABI_DISABLE_VISIBILITY_ANNOTATIONS) {
            ffi define _LIBCXXABI_HIDDEN: __attribute__((__visibility__("hidden")));
            ffi define _LIBCXXABI_DATA_VIS: __attribute__((__visibility__("default")));
            ffi define _LIBCXXABI_FUNC_VIS: __attribute__((__visibility__("default")));
        } eval if __has_attribute(__type_visibility__) {
            ffi define _LIBCXXABI_TYPE_VIS: __attribute__((__type_visibility__("default")));
        } else {
            ffi define _LIBCXXABI_TYPE_VIS: __attribute__((__visibility__("default")));
        }
    }
} else {
    ffi define _LIBCXXABI_HIDDEN;
    ffi define _LIBCXXABI_DATA_VIS;
    ffi define _LIBCXXABI_FUNC_VIS;
    ffi define _LIBCXXABI_TYPE_VIS;

    eval if defined(_LIBCXXABI_COMPILER_MSVC) {
        ffi define _LIBCXXABI_WEAK;
    } else {
        ffi define _LIBCXXABI_WEAK: __attribute__((__weak__));
    }

    eval if defined(__clang__) {
        ffi define _LIBCXXABI_COMPILER_CLANG;
    } else if defined(__GNUC__) {
        ffi define _LIBCXXABI_COMPILER_GCC;
    }

    eval if __has_attribute(__no_sanitize__) && defined(_LIBCXXABI_COMPILER_CLANG) {
        ffi define _LIBCXXABI_NO_CFI: __attribute__((__no_sanitize__("cfi")));
    } else {
        ffi define _LIBCXXABI_NO_CFI;
    }

    // wasm32 follows the arm32 ABI convention of using 32-bit guard.
    eval if defined(__arm__) || defined(__wasm32__) || defined(__ARM64_ARCH_8_32__) {
        ffi define _LIBCXXABI_GUARD_ABI_ARM;
    }

    eval if defined(_LIBCXXABI_COMPILER_CLANG) {
        eval if !__has_feature(cxx_exceptions) {
            ffi define _LIBCXXABI_NO_EXCEPTIONS;
        }
    } else if defined(_LIBCXXABI_COMPILER_GCC) && !defined(__EXCEPTIONS) {
        ffi define _LIBCXXABI_NO_EXCEPTIONS;
    }

    eval if defined(_WIN32) {
        ffi define _LIBCXXABI_DTOR_FUNC: __thiscall;
    } else {
        ffi define _LIBCXXABI_DTOR_FUNC;
    }
} // ____CXXABI_CONFIG_H