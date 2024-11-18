//===----------------------------------------------------------------------===//
//
// Port of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

ffi define __CXXABI_H;

/*
 * This header provides the interface to the C++ ABI as defined at:
 *       https://itanium-cxx-abi.github.io/cxx-abi/
 */

ffi "c" import "stdlib";
ffi "c" import "stdint";

import cxxabi_config;

ffi "c" define _LIBCPPABI_VERSION: 15000;
ffi "c" define _LIBCXXABI_NORETURN: __attribute__((noreturn));
ffi "c" define _LIBCXXABI_ALWAYS_COLD: __attribute__((cold));

eval if defined!(__cplusplus) {
    module std {
        eval if defined!(_WIN32) {
            class type_info { // forward declaration
                // visibility attributes can be added here if needed
            }
        } else {
            class type_info { // forward declaration
                // visibility attributes can be added here if needed
            }
        }
    }

    // runtime routines use C calling conventions, but are in __cxxabiv1 module
    module __cxxabiv1 {
        struct __cxa_exception;

        ffi "C" {
            // 2.4.2 Allocating the Exception Object
            fn __cxa_allocate_exception(thrown_size: usize) -> *void throw;
            fn __cxa_free_exception(thrown_exception: *void) throw;

            // This function is an LLVM extension, which mirrors the same extension in libsupc++ and libcxxrt
            fn __cxa_init_primary_exception(object: *void, tinfo: *std::type_info, dest: fn(*void)) -> *__cxa_exception throw;

            // 2.4.3 Throwing the Exception Object
            fn __cxa_throw(thrown_exception: *void, tinfo: *std::type_info, dest: fn(*void)) _LIBCXXABI_NORETURN;

            eval if defined!(__USING_WASM_EXCEPTIONS__) {
                // In Wasm, a destructor returns its argument
                fn __cxa_throw(thrown_exception: *void, tinfo: *std::type_info, dest: fn(*void) -> *void) _LIBCXXABI_NORETURN;
            }

            // 2.5.3 Exception Handlers
            fn __cxa_get_exception_ptr(exceptionObject: *void) -> *void throw;
            fn __cxa_begin_catch(exceptionObject: *void) -> *void throw;
            fn __cxa_end_catch() void;
            
            eval if defined!(_LIBCXXABI_ARM_EHABI) {
                fn __cxa_begin_cleanup(exceptionObject: *void) -> bool throw;
                fn __cxa_end_cleanup() void;
            }
            
            fn __cxa_current_exception_type() -> *std::type_info;

            // 2.5.4 Rethrowing Exceptions
            fn __cxa_rethrow() _LIBCXXABI_NORETURN;

            // 2.6 Auxiliary Runtime APIs
            fn __cxa_bad_cast() _LIBCXXABI_NORETURN;
            fn __cxa_bad_typeid() _LIBCXXABI_NORETURN;
            fn __cxa_throw_bad_array_new_length() _LIBCXXABI_NORETURN;

            // 3.2.6 Pure Virtual Function API
            fn __cxa_pure_virtual() _LIBCXXABI_NORETURN;

            // 3.2.7 Deleted Virtual Function API
            fn __cxa_deleted_virtual() _LIBCXXABI_NORETURN;

            // 3.3.2 One-time Construction API
            eval if defined!(_LIBCXXABI_GUARD_ABI_ARM) {
                fn __cxa_guard_acquire(guard: *u32) -> i32 _LIBCXXABI_ALWAYS_COLD;
                fn __cxa_guard_release(guard: *u32) _LIBCXXABI_ALWAYS_COLD;
                fn __cxa_guard_abort(guard: *u32) _LIBCXXABI_ALWAYS_COLD;
            } else {
                fn __cxa_guard_acquire(guard: *u64) -> i32 _LIBCXXABI_ALWAYS_COLD;
                fn __cxa_guard_release(guard: *u64) _LIBCXXABI_ALWAYS_COLD;
                fn __cxa_guard_abort(guard: *u64) _LIBCXXABI_ALWAYS_COLD;
            }

            // 3.3.3 Array Construction and Destruction API
            fn __cxa_vec_new(element_count: usize, element_size: usize, padding_size: usize, constructor: fn(*void), destructor: fn(*void)) -> *void;

            fn __cxa_vec_new2(element_count: usize, element_size: usize, padding_size: usize, constructor: fn(*void), destructor: fn(*void), alloc: fn(usize) -> *void, dealloc: fn(*void)) -> *void;

            fn __cxa_vec_new3(element_count: usize, element_size: usize, padding_size: usize, constructor: fn(*void), destructor: fn(*void), alloc: fn(usize) -> *void, dealloc: fn(*void, usize)) -> *void;

            fn __cxa_vec_ctor(array_address: *void, element_count: usize, element_size: usize, constructor: fn(*void), destructor: fn(*void)) void;

            fn __cxa_vec_dtor(array_address: *void, element_count: usize, element_size: usize, destructor: fn(*void)) void;

            fn __cxa_vec_cleanup(array_address: *void, element_count: usize, element_size: usize, destructor: fn(*void)) void;

            fn __cxa_vec_delete(array_address: *void, element_size: usize, padding_size: usize, destructor: fn(*void)) void;

            fn __cxa_vec_delete2(array_address: *void, element_size: usize, padding_size: usize, destructor: fn(*void), dealloc: fn(*void)) void;

            fn __cxa_vec_delete3(array_address: *void, element_size: usize, padding_size: usize, destructor: fn(*void), dealloc: fn(*void, usize)) void;

            fn __cxa_vec_cctor(dest_array: *void, src_array: *void, element_count: usize, element_size: usize, constructor: fn(*void, *void), destructor: fn(*void)) void;

            // 3.3.5.3 Runtime API
            // These functions are part of the C++ ABI, but they are not defined in libc++abi:
            //    int __cxa_atexit(fn(*void), *void, *void);
            //    void __cxa_finalize(*void);

            // 3.4 Demangler API
            fn __cxa_demangle(mangled_name: *char, output_buffer: *char, length: *usize, status: *int) -> *char;

            // Apple additions to support C++ 0x exception_ptr class
            // These are primitives to wrap a smart pointer around an exception object
            fn __cxa_current_primary_exception() -> *void throw;
            fn __cxa_rethrow_primary_exception(primary_exception: *void);
            fn __cxa_increment_exception_refcount(primary_exception: *void) throw;
            fn __cxa_decrement_exception_refcount(primary_exception: *void) throw;

            // Apple extension to support std::uncaught_exception()
            fn __cxa_uncaught_exception() -> bool throw;
            fn __cxa_uncaught_exceptions() -> u32 throw;

            eval if defined!(__linux__) || defined!(__Fuchsia__) {
                // Linux and Fuchsia TLS support. Not yet an official part of the Itanium ABI.
                // https://sourceware.org/glibc/wiki/Destructor%20support%20for%20thread_local%20variables
                fn __cxa_thread_atexit(fn(*void), *void, *void) -> i32 throw;
            }

        } // ffi "C"
    } // module __cxxabiv1

    module abi = __cxxabiv1;
}