# trunk-ignore-all(black)
# trunk-ignore-all(isort)
# --------------------------------------------------------------------------------
#                                 GENERATED FILE
# --------------------------------------------------------------------------------
# Filename: test.hlx
# Generation Date: 2024-03-11 23:27:01
# Generator: Helix Transpiler
# --------------------------------------------------------------------------------
# WARNING: This file is AUTO-GENERATED by the Helix Transpiler. Any modifications
# made directly to this file may be OVERWRITTEN during future compilations. To
# introduce changes, please modify the source files and recompile.
# --------------------------------------------------------------------------------
# Licensed under the Creative Commons Attribution 4.0 International License (CC BY 4.0)
# SPDX-License-Identifier: CC-BY-4.0
# License Details: https://creativecommons.org/licenses/by/4.0/
#
# By using this file, you agree to the Terms and Conditions of the License.
# --------------------------------------------------------------------------------
# Helix Version: 0.1.0-alpha.a
# Repository: https://github.com/kneorain/helix
# Documentation: https://kneorain.github.io/helix/
# For further assistance, contact the development team or refer to project documentation.
# --------------------------------------------------------------------------------
from __future__ import annotations # type: ignore
from beartype.door import is_bearable as is_typeof, is_subhint as is_sub_typeof # type: ignore
from beartype import beartype, BeartypeConf # type: ignore
###### from include.plum.plum import dispatch as overload_with_type_check
import os               # type: ignore
import sys              # type: ignore
import types            # type: ignore
sys.path.append(os.path.dirname(os.path.realpath("z:\\devolopment\\helix\\helix-lang\\helix.py")) + os.sep + ".helix")            # type: ignore
sys.path.append(os.path.dirname(os.path.realpath("z:\\devolopment\\helix\\helix-lang\\helix.py")))            # type: ignore
sys.path.append(os.path.dirname(os.path.realpath(os.getcwd())))                                     # type: ignore
# trunk-ignore(ruff/F401)
from include.core import ABC, Any, BuiltinFunctionType, C_For, Callable, DEFAULT_VALUE, DispatchError, Enum, FastMap, FunctionType, Iterator, Literal, NoReturn, NoneType, Optional, Self, T, Thread, Type, TypeVar, UnionType, __import_c__, __import_cpp__, __import_py__, __import_rs__, annotations, array, auto, beartype, dataclass, double, getcontext, hx__abstract_method, hx__async, hx__multi_method, hx_array, hx_bool, hx_bytes, hx_char, hx_double, hx_float, hx_int, hx_list, hx_map, hx_set, hx_string, hx_tuple, hx_unknown, hx_void, inspect, multimeta, panic, printf, ref, replace_primitives, scanf, sleep, std, string, subtype, unknown, void, wraps # type: ignore
# trunk-ignore(ruff/F401)
# trunk-ignore(ruff/F811)
from include.core import __import_c__, __import_cpp__, __import_py__, __import_rs__                 # type: ignore
import threading  # type: ignore
import functools  # type: ignore
__lock = threading.Lock()
__file__ = "Z:\\devolopment\\helix\\helix-lang\\syntax\\test.hlx"
# trunk-ignore(ruff/F821)
def exception_handler(exception_type: type[BaseException] | threading.ExceptHookArgs, exception: Optional[Exception] = None, tb: Optional[types.TracebackType] = None, debug_hook: bool = False, thread_error: bool = False):
    import traceback
    import linecache
    import os
    from include.core import _H_tokenize_line__
    from beartype.roar import BeartypeException
    print()
    thread_name = None
    if thread_error and exception_type is not None:
        thread_name = exception_type.thread.name  # type: ignore
        exception = exception_type.exc_value      # type: ignore
        tb = exception_type.exc_traceback         # type: ignore
        exception_type = exception_type.exc_type  # type: ignore
    stack = traceback.extract_tb(tb)
    stack = traceback.StackSummary([frame for frame in stack if not frame.filename.startswith("<@beartype(")])
    exception = TypeError(str(exception).replace("type hint", "type")) if isinstance(exception, BeartypeException) else exception
    current_exception = exception
    relevant_frames = []
    early_replacements = dict((v, k) for k, v in {'...': 'None', 'true': 'True', 'false': 'False', 'null': 'None', 'none': 'None', '&&': 'and', '||': 'or', '!': 'not', '===': 'is', '!==': 'is not', 'stop': 'break', '::': '.', 'new': '__init__', 'delete': '__del__', 'enter': '__enter__', 'exit': '__exit__', 'u8': 'hx_u8', 'u16': 'hx_u16', 'u32': 'hx_u32', 'u64': 'hx_u64', 'u128': 'hx_u128', 'i8': 'hx_i8', 'i16': 'hx_i16', 'i32': 'hx_i32', 'i64': 'hx_i64', 'i128': 'hx_i128', 'f32': 'hx_f32', 'f64': 'hx_f64', 'f128': 'hx_f128'}.items())
    # First loop: filter out irrelevant frames
    index = 0
    for frame in stack:
        filename = frame.filename
        line_no = frame.lineno
        if "_hlx" in os.path.basename(filename):
            filename = __file__
            line_no = int(open(frame.filename + ".lines", "r").readlines()[line_no-1]) # type: ignore
            if line_no == -1:
                continue
        # Check specific conditions to skip
        if (
            f"plum{os.sep}plum" in filename
        ):
            continue
        if (
            linecache.getline(filename, line_no-1).strip() == "def hx_internal_multi_method_decorator(*args, **kwargs):" # type: ignore
            and
            linecache.getline(filename, line_no-3).strip() == "def hx__multi_method(func: Callable) -> Callable:" # type: ignore
        ):
            continue
        relevant_frames.append((index, frame))
        index += 1
    if len(relevant_frames) > 1:
        __lock.acquire(blocking=True, timeout=1.2)
        for frame_info in relevant_frames:
            index, frame = frame_info
            filename = frame.filename
            line_no = frame.lineno
            if "_hlx" in os.path.basename(filename):
                filename = __file__
                line_no = int(open(frame.filename + ".lines", "r").readlines()[line_no-1]) # type: ignore
            # Attempt to find the marked part in the error message
            # see if the frame contains colno and end_colno
            marks = None
            if hasattr(frame, "colno") and hasattr(frame, "end_colno"): # type: ignore
                marks = list(_H_tokenize_line__(frame._line[frame.colno:frame.end_colno])) # type: ignore
            try:
                file_ext =  os.path.basename(filename).split('.')[1] # type: ignore
            except IndexError:
                file_ext = "py"
            if marks:
                panic(
                    current_exception,  # type: ignore
                    *marks,
                    file=filename,
                    line_no=line_no,  # type: ignore
                    multi_frame=True,
                    pos=0 if index == 0 else 1 if index < len(relevant_frames) - 1 else 2,
                    replacements=early_replacements,
                    follow_marked_order=True,
                    mark_start=frame.colno,
                    thread_name=thread_name,
                    lang=file_ext
                )
            else:
                panic(
                    current_exception,  # type: ignore
                    file=filename,
                    line_no=line_no,  # type: ignore
                    replacements=early_replacements,
                    multi_frame=True,
                    pos=0 if index == 0 else 1 if index < len(relevant_frames) - 1 else 2,
                    thread_name=thread_name,
                    lang=file_ext
                )
            current_exception = current_exception.__cause__ if current_exception.__cause__ else current_exception  # type: ignore
        else:
            __lock.release()
            exit(1)
    else:
        __lock.acquire(blocking=True, timeout=0.1)
        index, frame = relevant_frames[0]
        filename = frame.filename
        line_no = frame.lineno
        if "_hlx" in filename:
            filename = __file__
            line_no = int(open(frame.filename + ".lines", "r").readlines()[line_no-1]) # type: ignore
        # Attempt to find the marked part in the error message
        # see if the frame contains colno and end_colno
        marks = None
        if hasattr(frame, "colno") and hasattr(frame, "end_colno"):
            marks = list(_H_tokenize_line__(frame._line[frame.colno:frame.end_colno])) # type: ignore
        try:
            file_ext =  os.path.basename(filename).split('.')[1]
        except IndexError:
            file_ext = "py"
        if marks:
            panic(
                current_exception, # type: ignore
                *marks,
                file=filename,
                line_no=line_no, # type: ignore
                replacements=early_replacements,
                follow_marked_order=True,
                mark_start=frame.colno,
                thread_name=thread_name,
                lang=file_ext
            )
        else:
            panic(
                current_exception, # type: ignore
                file=filename,
                line_no=line_no, # type: ignore
                replacements=early_replacements,
                thread_name=thread_name,
                lang=file_ext
            )
        __lock.release()
        exit(1)
sys.excepthook = exception_handler  # type: ignore
threading.excepthook = functools.partial(exception_handler, thread_error=True)
sys.argv = ["Z:\\devolopment\\helix\\helix-lang\\helix.py", "Z:\\devolopment\\helix\\helix-lang\\syntax\\test.hlx"] + list(sys.argv)[2:]
del os, threading, functools
overload_with_type_check = beartype(conf=BeartypeConf(is_color=False))   # type: ignore
class C_cout():
    def __init__(self: Any, __val: 'C_cout'):
            raise NotImplementedError("Define an __init__ method for this class with the function signature new(self: Any, inst_class: 'C_cout')")
    @overload_with_type_check
    def __lshift__(self: Any, a: str | hx_string):
        print ( a )
@overload_with_type_check
def main(argv: list[str | hx_string] | hx_list[str | hx_string]):
    a: int | hx_int = int("12")
    add ( 123 , 123 )
    print ( add . join ( ) )
    do_something ( )
    for i in C_For(i = int(0)).set_con('i < 10').set_inc('i ++'):
        printf ( "doing something else eeeee: %d" , i )
    del i
    do_something . join ( )
    print ( "done" )
    return 0
@hx__async
def add(a: int | hx_int, b: int | hx_int) -> int:
    printf ( "adding: %d + %d" , a , b )
    return a + b
@overload_with_type_check
def subtract(a: int | hx_int, b: int | hx_int) -> int:
    return a - b
@hx__async
def do_something():
    for i in C_For(i = int(0)).set_con('i < 10').set_inc('i ++'):
        printf ( "doing something: %d" , i )
    del i
@overload_with_type_check
def a_cursed_fucntion(a: int | hx_int) -> FunctionType:
    printf ( "new something: %d" , a )
    return a_cursed_fucntion
if __name__ == "__main__":
    try:
        main() # type: ignore
    except (KeyError, TypeError):
        main(sys.argv)
