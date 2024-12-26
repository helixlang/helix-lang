#ifndef __CXX_FLAGS_H__
#define __CXX_FLAGS_H__

#include <string_view>

namespace cxx::flags {
class CF {
  public:
    std::string_view gcc;
    std::string_view clang;
    std::string_view msvc;
    std::string_view mingw;
};

constexpr CF debugModeFlag{"-g -g3", "-g -g3", "/Zi", "-g -g3"};
constexpr CF warnAllFlag{"-Wall", "-Wall", "/W4", "-Wall"};
constexpr CF noWarningsFlag{"-w", "-w", "/w", "-w"};
constexpr CF stdC23Flag{"-std=c23", "-std=c23", "/std:c23", "-std:c23"};
constexpr CF stdCXX23Flag{"-std=c++23", "-std=c++23", "/std:c++20", "-std:c++23"};
constexpr CF optimizationLevel0{"-O0", "-O0", "/Od", "-O0"};
constexpr CF optimizationLevel1{"-O1", "-O1", "/O1", "-O1"};
constexpr CF optimizationLevel2{"-O2", "-O2", "/O2", "-O2"};
constexpr CF optimizationLevel3{"-O3", "-O3", "/Ox", "-O3"};
constexpr CF dryRunFlag{"-fsyntax-only", "-fsyntax-only", "/Zs", "-fsyntax-only"};
constexpr CF optimizationFast{"-Ofast", "-Ofast", "/Ox", "-Ofast"};
constexpr CF optimizationSize{"-Os", "-Os", "/O1", "-Os"};
constexpr CF linkStatic{"-static", "-static", "/MT", "-static"};
constexpr CF linkShared{"-shared", "-shared", "/LD", "-shared"};
constexpr CF positionIndependent{"-fPIC", "-fPIC", "/LD", "-fPIC"};
constexpr CF cxxStandardFlag{"-xc++", "-xc++", "/TP", "-xc++"};  // force c++ mode
constexpr CF noOptimization{"-fno-inline", "-fno-inline", "/Ob0", "-fno-inline"};
constexpr CF defineFlag{"-D", "-D", "/D", "-D"};
constexpr CF includeFlag{"-I", "-I", "/I", "-I"};
constexpr CF linkFlag{"-l", "-l", "/link", "-l"};
constexpr CF linkTimeOptimizationFlag{"-flto", "-flto", "/LTCG", "-flto"};
constexpr CF outputFlag{"-o", "-o", "/Fe:", "-o"};
constexpr CF inputFlag{"-i", "-i", "/Fi:", "-i"};
constexpr CF precompiledHeaderFlag{"-include", "-include", "/FI", "-include"};
constexpr CF preprocessorFlag{"-E", "-E", "/P", "-E"};
constexpr CF compileOnlyFlag{"-c", "-c", "/c", "-c"};
constexpr CF noEntryFlag{"-nostartfiles", "-nostartfiles", "/NOENTRY", "-nostartfiles"};
constexpr CF noDefaultLibrariesFlag{
    "-nodefaultlibs", "-nodefaultlibs", "/NODEFAULTLIB", "-nodefaultlibs"};
constexpr CF noCXXSTDLibrariesFlag{"-nostdlib++", "-nostdlib++", "/NOSTDLIB", "-nostdlib++"};
constexpr CF noCXXSTDIncludesFlag{"-nostdinc++", "-nostdinc++", "/NOSTDINC", "-nostdinc++"};
constexpr CF noBuiltinIncludesFlag{
    "-nobuiltininc", "-nobuiltininc", "/NOBUILTININC", "-nobuiltininc"};
constexpr CF linkLibCFlag{"-lc", "-lc", "/DEFAULTLIB:libc", "-lc"};
constexpr CF linkPathFlag{"-L", "-L", "/LIBPATH", "-L"};
constexpr CF linkDynamicFlag{"-Wl,-w,-rpath", "-Wl,-w,-rpath", "/LIBPATH", "-Wl,-w,-rpath"};
constexpr CF noDiagnosticsFlag{"-fno-diagnostics-show-option",
                               "-fno-diagnostics-show-option",
                               "/diagnostics:column",
                               "-fno-diagnostics-show-option"};
constexpr CF noDiagnosticsFixitFlag{"-fno-diagnostics-fixit-info",
                                    "-fno-diagnostics-fixit-info",
                                    "/diagnostics:caret",
                                    "-fno-diagnostics-fixit-info"};
constexpr CF noDiagnosticsShowLineNumbersFlag{"-fno-diagnostics-show-line-numbers",
                                              "-fno-diagnostics-show-line-numbers",
                                              "/diagnostics:caret",
                                              "-fno-diagnostics-show-line-numbers"};
constexpr CF noDiagnosticsShowOptionFlag{"-fno-diagnostics-show-option",
                                         "-fno-diagnostics-show-option",
                                         "/diagnostics:caret",
                                         "-fno-diagnostics-show-option"};
constexpr CF noColorDiagnosticsFlag{"-fno-color-diagnostics",
                                    "-fno-color-diagnostics",
                                    "/diagnostics:caret",
                                    "-fno-color-diagnostics"};
constexpr CF caretDiagnosticsMaxLinesFlag{"-fcaret-diagnostics-max-lines=0",
                                          "-fcaret-diagnostics-max-lines=0",
                                          "/diagnostics:caret",
                                          "-fcaret-diagnostics-max-lines=0"};
constexpr CF noElideTypeFlag{
    "-fno-elide-type", "-fno-elide-type", "/diagnostics:caret", "-fno-elide-type"};
constexpr CF noOmitFramePointerFlag{"-fno-omit-frame-pointer",
                                    "-fno-omit-frame-pointer",
                                    "/diagnostics:caret",
                                    "-fno-omit-frame-pointer"};
constexpr CF enableExceptionsFlag{"-fexceptions", "-fexceptions", "/EHsc", "-fexceptions"};

constexpr CF fullFilePathFlag{"-fdiagnostics-absolute-paths",
                              "-fdiagnostics-absolute-paths",
                              "/FC",
                              "-fdiagnostics-absolute-paths"};
constexpr CF showCaretsFlag{"-fshow-caret",
                           "-fshow-caret",
                           "/diagnostics:caret",
                           "-fshow-caret"};
constexpr CF noErrorReportingFlag{"-fno-error-report",
                                  "-fno-error-report",
                                  "/errorReport:None",
                                  "-fno-error-report"};
constexpr CF SanitizeFlag{"-fsanitize=all",
                                    "-fsanitize=all",
                                    "/fsanitize=address /RTCc /RTC1 /sdl /RTCu /RTCsu /RTCs",
                                    "-fsanitize=all"};
constexpr CF None {"", "", "", ""};
}  // namespace cxx::flags
#endif  // __CXX_FLAGS_H__