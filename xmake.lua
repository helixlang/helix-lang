set_project    ("helix-lang")
set_version    ("0.0.1-alpha-51b", { soname = true })
set_description("The Helix Compiler. Python's Simplicity, Rust inspired Syntax, and C++'s Power")

add_rules("mode.debug", "mode.release")

local abi
local runtime

function install_llvm_clang(package)
    local gray = "\027[1;30m"
    print(gray .. "installing llvm/clang will take anywhere between 10 minutes to a few hours if you want to see progress re-run with -v" .. "\027[0m")

	-- Set the number of Cores to use for parallel compilation
	local threads_avaliable = os.cpuinfo("ncpu") - 3
	local configs = {}

	table.insert(configs, "-DLLVM_ENABLE_PROJECTS=clang")
	table.insert(configs, "-DCMAKE_BUILD_TYPE=Release")                      -- always build in release mode

	table.insert(configs, "-DLLVM_ENABLE_ZSTD=ON")                           -- disable ZSTD support
	table.insert(configs, "-DLLVM_ENABLE_ZLIB=ON")                           -- disable ZLIB support

	table.insert(configs, "-DLLVM_ENABLE_RTTI=ON")                           -- enable RTTI for dynamic_cast and typeid
	table.insert(configs, "-DLLVM_ENABLE_BENCHMARKS=OFF")                    -- turn off benchmarks
	table.insert(configs, "-DLLVM_INCLUDE_BENCHMARKS=OFF")                   -- exclude benchmarks
	table.insert(configs, "-DLLVM_INCLUDE_TESTS=OFF")                        -- exclude tests
	table.insert(configs, "-DLLVM_INCLUDE_DOCS=ON")                          -- include documentation
	table.insert(configs, "-DLLVM_INCLUDE_EXAMPLES=ON")                      -- exclude examples
	table.insert(configs, "-DLLVM_PARALLEL_LINK_JOBS=" .. threads_avaliable) -- parallelize

	import("package.tools.cmake").install(package, configs, { cmake_generator = "Ninja" })
end

function get_abi()
    if os.host() == "windows"
    then
        return "msvc"
    elseif os.host() == "linux"
    then
        return "gcc"
    elseif os.host() == "macosx"
    then
        return "llvm"
    end
	return "" -- error?
end

function get_runtime(abi)
	if abi == "llvm"
	then
		return "c++_static"
	elseif abi == "gcc"
	then
		return "stdc++_static"
	elseif abi == "msvc"
	then
		return "MT"
	end
	return "" -- error?
end

function get_cxx_standard(abi)
	if abi == "llvm"
	then
		return "c++23"
	elseif abi == "gcc"
	then
		return "c++23"
	elseif abi == "msvc"
	then
		return "c++latest"
	end
	return "" -- error?
end



function setup_windows()
	add_rules("plugin.vsxmake.autoupdate")

	add_syslinks("ntdll", "version")
    -- add /EHsc and /RTC1 flags
    set_policy("check.auto_ignore_flags", false)
    add_cxflags("/EHsc")
	add_includedirs(".\\libs\\llvm-18.1.9-src\\llvm\\include")
	add_linkdirs(".\\libs\\llvm-18.1.9-src\\llvm\\lib")
	add_includedirs(".\\libs\\llvm-18.1.9-src\\clang\\include")
	add_linkdirs(".\\libs\\llvm-18.1.9-src\\clang\\lib")

end

function setup_linux()
	if os.exists("/home/linuxbrew/.linuxbrew/include") and os.exists("/home/linuxbrew/.linuxbrew/lib") then
		add_includedirs("/home/linuxbrew/.linuxbrew/include")
		add_linkdirs("/home/linuxbrew/.linuxbrew/lib")
	end

	-- check if perl is is installed

	-- it only works with set alternatives
	-- local ld = os.getenv("LD");

	-- if ld == nil then
	-- 	print("The `LD` environment variable must contain the path to the lld executable")
	-- elseif not (path.filename(ld) == "ld.lld") then
	-- 	print("The `LD` environment variable must contain the path to the lld executable, it contains: ", ld)
	-- end
	-- use with lld os.runv("echo", {"hello", "xmake!"})
	--setenv({ LD = os.getenv("HELIX_LLD") })
end

function setup_macos()
	if os.exists("/opt/homebrew/include") and os.exists("/opt/homebrew/lib") then
		add_includedirs("/opt/homebrew/include")
		add_linkdirs("/opt/homebrew/lib")
	end
end

function setup_debug()
	set_symbols ("debug") -- Generate debug symbols
	set_optimize("none")  -- Disable optimization
	add_defines ("DEBUG") -- Define DEBUG macro
    set_warnings("all", "extra")
end

function setup_release()

	set_symbols ("hidden")     -- Hide symbols
	set_optimize("aggressive") -- Enable maximum optimization
	add_defines ("NDEBUG")     -- Define NDEBUG macro
    set_warnings("none")
end

local function setup_build_folder()
	set_targetdir("$(buildir)/$(mode)/$(arch)-" .. abi .. "-$(os)/bin")
	set_objectdir("$(buildir)/.resolver")
	set_dependir ("$(buildir)/.shared")
end

local function setup_env()
	if os.host() == "windows"
	then
		setup_windows()
	elseif os.host() == "macosx"
	then
		setup_macos()
	elseif os.host() == "linux"
	then
		setup_linux()
	end

	-- Setup build mode specific settings
	if is_mode("debug")
	then
		setup_debug()
	else
		setup_release()
	end

	set_policy("build.across_targets_in_parallel", true) -- optimization

    -- Set the C++ Standard
	set_languages(cxx_standard)

    -- Set the runtime
    set_runtimes(runtime)
end

local function helix_src_setup()
	-- Include dir
	add_includedirs("source")

	-- Add source fikes
	add_files("source/**.cc") -- add all files in the source directory
    -- add_files("source/**.hlx") -- compile all helix files

	-- Header files
	add_headerfiles("source/**.hh") -- add all headers in the source directory
	add_headerfiles("source/**.def")
	add_headerfiles("source/**.inc")
    
    add_headerfiles("lib-helix/core/include/**.h") -- add all headers in the lib-helix/core/include directory
    add_includedirs("lib-helix/core/include") -- add all headers in the lib-helix/core/include directory

    -- libs
    add_includedirs("libs") -- add all files in the neo-json directory

	add_headerfiles("libs/neo-panic/**.hh")  -- add all files in the neo-panic directory
	add_files("libs/neo-panic/**.cc")        -- add all files in the neo-panic directory
    
    add_headerfiles("libs/neo-json/**.hh")    -- add all files in the neo-json directory
    add_headerfiles("libs/glaze-json/**.hpp") -- add all files in glaze-json directory
	add_headerfiles("libs/neo-pprint/**.hh")  -- add all files in the neo-pprint directory
	add_headerfiles("libs/neo-types/**.hh")   -- add all files in the neo-types directory
	add_headerfiles("libs/taywee-args/**.hh") -- add all files in the taywee-args directory
end

function sleep(seconds)
    local start = os.mclock()
    while os.mclock() - start < seconds do end
end

local function print_all_info(target)
    print("\n\n")

    local yellow = "\027[1;33m"
    local green = "\027[1;32m"
    local magenta = "\027[1;35m"
    local cyan = "\027[1;36m"
    local blue = "\027[1;34m"
    local reset = "\027[0m"

    if is_mode("release")
    then
        print(yellow .. "#===--------------------- " .. cyan .. "Welcome to Helix " .. yellow .. "---------------------===#")
        print(yellow .. "#                                                                  #")
        print(yellow .. "#  " .. reset .. "Thank you for using Helix, part of the Helix Project!" .. yellow .. "           #")
        print(yellow .. "#                                                                  #")
        print(yellow .. "#  " .. reset .. "Helix is licensed under the " .. magenta .. "Creative Commons Attribution 4.0" .. yellow .. "    #")
        print(yellow .. "#  " .. magenta .. "International (CC BY 4.0)." .. yellow .. "                                      #")
        print(yellow .. "#                                                                  #")
        print(yellow .. "#  " .. reset .. "This means you're free to use, modify, and distribute Helix," .. yellow .. "    #")
        print(yellow .. "#  " .. reset .. "even for commercial purposes, as long as you give proper" .. yellow .. "        #")
        print(yellow .. "#  " .. reset .. "credit and indicate if any changes were made. For more" .. yellow .. "          #")
        print(yellow .. "#  " .. reset .. "information, visit the link below." .. yellow .. "                              #")
        print(yellow .. "#  " .. cyan .. "https://creativecommons.org/licenses/by/4.0/" .. yellow .. "                    #")
        print(yellow .. "#                                                                  #")
        print(yellow .. "#  " .. magenta .. "SPDX-License-Identifier: " .. reset .. "CC-BY-4.0" .. yellow .. "                              #")
        print(yellow .. "#  " .. reset .. "Copyright (c) 2024 " .. cyan .. "Helix Project" .. yellow .. "                                #")
        print(yellow .. "#                                                                  #")
        print(yellow .. "#===------------------------------------------------------------===#" .. reset)

        print("\n\n")
    end

    if is_mode("debug")
    then
        print("skipping checks:")
    else
        print("starting checks (switch to debug mode to skip checks):")
    end

	if is_mode("debug")
	then
        sleep(math.random(1, 10))
        print("  mode: \027[1;31mdebug\027[0m")
        sleep(math.random(1, 10))
        print("  warnings: \027[1;31mall\027[0m")
        sleep(math.random(1, 10))
        print("  symbols: \027[1;31mall\027[0m")
    else
        sleep(math.random(1, 10))
        print("  mode: \027[1;32mrelease\027[0m")
        sleep(math.random(1, 10))
        print("  warnings: \027[1;33mless\027[0m")
        sleep(math.random(1, 10))
        print("  symbols: \027[1;33mnone\027[0m")
    end

    print("  os target: \027[1;33m" .. os.host() .. "\027[0m")
	sleep(math.random(1, 20))
    print("  build multi-core: \027[1;33mtrue\027[0m")
	sleep(math.random(1, 20))
    print("  language: \027[1;33mc++23\027[0m")
    sleep(math.random(1, 20))
    print("  abi: \027[1;33m" .. abi .. "\027[0m")
    sleep(math.random(1, 20))
    print("  runtime: \027[1;33m" .. runtime .. "\027[0m")
    sleep(math.random(1, 20))
    print("  target triple: \027[1;33m" .. path.basename(path.directory(path.directory(target:targetfile()))) .. "\027[0m")
    
    sleep(500)
    
    if is_mode("debug")
    then
        print("\n\n")
        print("starting build")
    else
        print("\n\n")
        print("checking for components:")

        sleep(500)

        -- loop thrugh all the folders along with thier subdirectories (do not include files)
        for _, dir in ipairs(os.dirs("source/**")) do
            -- Print directory name
            print("  - found \027[1;33m\"" .. path.filename(dir) .. "\"\027[0m")

            -- add a small delay between 0.1 - 0.3 seconds
            sleep(math.random(1, 10))

            -- loop through all files inside the directory
            for _, file in ipairs(os.files(dir .. "/**")) do
                print("  - found \027[1;33m\"" .. path.basename(path.filename(file)) .. "\"\027[0m")
                -- add a small delay between 0.1 - 0.3 seconds
                sleep(math.random(1, 10))
            end
        end

        print("\n\n")

        sleep(500)
        print("checks complete, starting build")
    end

    print("\n\n")
end

abi     = get_abi()
runtime = get_runtime(abi)
cxx_standard = get_cxx_standard(abi)

-- Define the LLVM and Clang package
package("llvm-clang")
    add_deps("cmake", "ninja") -- , "zlib", "zstd" , "perl"
    set_sourcedir(path.join(os.scriptdir(), "libs", "llvm-18.1.9-src", "llvm"))
    on_install(install_llvm_clang)
package_end()

-- Requires
add_requires("zlib")
add_requires("zstd")
add_requires("llvm-clang")

-- Packages
add_packages("zlib")
add_packages("zstd")
add_packages("llvm-clang") -- link against the LLVM-Clang package

-- Global Links
add_links("zstd")

setup_build_folder()
setup_env()

helix_src_setup()

target("tests")
    set_kind("binary")
    add_files("tests/**.cc")        -- add all files in the tests directory
    add_includedirs("tests/lib")    -- add all libs in the tests dir

    remove_files("source/helix.cc") -- exclude main from the source directory
target_end()

target("helix") -- target config defined in the config seciton
    before_build(function (target)
        print_all_info(target)
    end)

    set_kind("binary")
    set_languages(cxx_standard)

    -- after build copy all the folders and files from lib-helix to the target directory
    after_build(function(target)
        -- Determine the target output directory
        local target_dir = path.directory(target:targetfile())

        -- Traverse the lib-helix folder to find all files
        for _, filepath in ipairs(os.files("lib-helix/**/*")) do
            -- Get the file extension and relative path
            local ext = path.extension(filepath)
            local relative_path = path.relative(filepath, "lib-helix")
            local target_path = path.join(target_dir, "..", relative_path)

            if ext == ".h" or ext == ".hh" then
                -- Process header files: prepend #line directive
                local content = io.readfile(filepath)
                local line_directive = string.format('#line 1 R"(%s)"\n', path.translate(path.absolute(filepath)))
                content = line_directive .. content

                -- Write the modified header to the target location
                io.writefile(target_path, content)
            else
                -- Copy other files without modification
                os.cp(filepath, target_path)
            end
        end

        -- Remove the README.md file
        os.rm(path.join(target_dir, "..", "README.md"))
    end)
target_end() -- empty target

-- rule("helix")
--   set_extensions(".hlx")
--   on_build_file(function (target, sourcefile)
--     -- Call Helix compiler to compile .hlx file to C++ or object file
--     os.run("helix %s", sourcefile)
--     -- Optionally link Helix output with C++ build
--   end)

target("helix-api")
    set_kind("static")
    set_targetdir("$(buildir)/$(mode)/$(arch)-" .. abi .. "-$(os)")

    after_build(function(target) -- make the helix library with all the appropriate header files
        -- determine the target output directory
        local target_dir = path.directory(target:targetfile())

        local include_path            = path.join(target_dir,       "include")
        local include_lib_path        = path.join(include_path,     "lib")
        local include_include_path    = path.join(include_path,     "include")
        local include_xmake_lua       = path.join(include_path,     "xmake.lua")
        local include_lib_target_file = path.join(include_lib_path, target:filename())

        -- create directories for library and headers
        os.mkdir(include_lib_path)
        os.mkdir(include_include_path)

        -- move the compiled library to the 'lib' folder and
        os.cp(target:targetfile(), include_lib_target_file)
        os.rm(target:targetfile())

        -- copy header files to the 'include' folder
        local headers = os.files("source/**.hh")

        for _, header in ipairs(headers) do
            local rel_path = path.relative(header, "source")
            os.mkdir(path.join(include_include_path, path.directory(rel_path)))
            os.cp(header, path.join(include_include_path, rel_path))
        end

        -- Write the xmake config file
        local file = io.open(include_xmake_lua, "w")

        file:write([[
        target("helix-include")
            set_kind("static")
            add_files("lib/*.a")
            add_includedirs("include")
            add_headerfiles("include/**.hh")
        ]])
        file:close()
    end)
target_end()

-- -fexperimental-new-constant-interpreter