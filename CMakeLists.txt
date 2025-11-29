cmake_minimum_required(VERSION 3.16)
project(MADOLA VERSION 0.1.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build optimizations
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # Generate compile_commands.json for IDE integration

# Enable parallel builds by default
if(NOT DEFINED CMAKE_BUILD_PARALLEL_LEVEL)
    include(ProcessorCount)
    ProcessorCount(N)
    if(NOT N EQUAL 0)
        set(CMAKE_BUILD_PARALLEL_LEVEL ${N})
        message(STATUS "Setting parallel build jobs to ${N}")
    endif()
endif()

# Enable ccache if available (speeds up rebuilds)
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Using ccache for faster rebuilds: ${CCACHE_PROGRAM}")
endif()

# Enable sccache as fallback if ccache not found
if(NOT CCACHE_PROGRAM)
    find_program(SCCACHE_PROGRAM sccache)
    if(SCCACHE_PROGRAM)
        set(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
        message(STATUS "Using sccache for faster rebuilds: ${SCCACHE_PROGRAM}")
    endif()
endif()

# Unity builds option (combines source files to reduce compile time)
option(USE_UNITY_BUILD "Enable unity builds for faster compilation" OFF)

# Precompiled headers option
option(USE_PRECOMPILED_HEADERS "Use precompiled headers" ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Compiler flags
if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /W4")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
    set(CMAKE_C_FLAGS_DEBUG "/Od /Zi /W4")
    set(CMAKE_C_FLAGS_RELEASE "/O2 /DNDEBUG")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

# Include directories (using target-specific includes instead of global)
# Global includes removed - now using target_include_directories for better dependency management

# Find Node.js executable
find_program(NODE_EXECUTABLE NAMES node nodejs REQUIRED)
message(STATUS "Found Node.js: ${NODE_EXECUTABLE}")

# Generate embedded CSS header from CSS file
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/src/core/generator/embedded_css.h
    COMMAND ${NODE_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/generate_css_header.js
            ${CMAKE_SOURCE_DIR}/web/css/madola-output.css
            ${CMAKE_SOURCE_DIR}/src/core/generator/embedded_css.h
    DEPENDS ${CMAKE_SOURCE_DIR}/web/css/madola-output.css
    COMMENT "Generating embedded CSS header"
)

# Add generated header as a dependency
add_custom_target(generate_css_header
    DEPENDS ${CMAKE_SOURCE_DIR}/src/core/generator/embedded_css.h
)

# Source files
set(CORE_SOURCES
    src/core/evaluator/evaluator.cpp
    src/core/evaluator/evaluator_environment.cpp
    src/core/evaluator/evaluator_expressions.cpp
    src/core/evaluator/evaluator_matrix.cpp
    src/core/evaluator/evaluator_functions.cpp
    src/core/generator/markdown_formatter.cpp
    src/core/generator/html_formatter_helpers.cpp
    src/core/generator/html_formatter_content.cpp
    src/core/generator/html_formatter_methods.cpp
    src/core/generator/cpp_generator.cpp
    src/core/generator/wasm_generator.cpp
    src/core/generator/wasm_addon_loader.cpp
    src/core/generator/unit_system.cpp
)

# Optional Tree-sitter support
option(WITH_TREE_SITTER "Build with Tree-sitter support" ON)

# Optional SymEngine support
option(WITH_SYMENGINE "Build with SymEngine symbolic computation support" ON)

# Optional debug features
option(WITH_DEBUG_FEATURES "Build with debug features" ON)

if(WITH_TREE_SITTER)
    # Add AST builder to core sources when Tree-sitter is enabled
    list(APPEND CORE_SOURCES
        src/core/ast/ast_builder_statements.cpp
        src/core/ast/ast_builder_expressions.cpp
    )

    # Tree-sitter sources (when available)
    set(TREE_SITTER_SOURCES
        vendor/tree-sitter/lib/src/lib.c
    )

    set(MADOLA_GRAMMAR_SOURCES
        tree-sitter-madola/src/parser.c
    )
endif()

# SymEngine configuration
if(WITH_SYMENGINE)
    # Add symbolic evaluator to core sources
    list(APPEND CORE_SOURCES
        src/core/evaluator/evaluator_symbolic.cpp
    )
endif()

if(WITH_SYMENGINE)

    # Configure SymEngine with minimal dependencies
    set(BUILD_TESTS OFF CACHE BOOL "Build SymEngine tests" FORCE)
    set(BUILD_BENCHMARKS OFF CACHE BOOL "Build SymEngine benchmarks" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

    # Configure integer class based on target platform
    # NOTE: For production Tauri/server builds, you can enable GMP for better performance:
    #   - Uncomment the GMP section below for native builds
    #   - Install GMP: pacman -S mingw-w64-x86_64-gmp (MSYS2) or apt-get install libgmp-dev (Linux)
    #   - GMP provides faster arbitrary-precision arithmetic than Boost.Multiprecision
    #   - Other optional dependencies for advanced features:
    #     * MPFR (arbitrary-precision floating-point): -DWITH_MPFR=ON
    #     * MPC (complex numbers): -DWITH_MPC=ON
    #     * ARB (ball arithmetic): -DWITH_ARB=ON

    if(EMSCRIPTEN)
        # WASM builds: Use Boost.Multiprecision (header-only, no linking required)
        set(INTEGER_CLASS "boostmp" CACHE STRING "Integer class for SymEngine" FORCE)
        set(WITH_BOOST ON CACHE BOOL "Build with Boost (header-only for boostmp)" FORCE)

        # Set Boost paths for SymEngine to find (header-only, no CMake config needed)
        set(BOOST_ROOT "${CMAKE_SOURCE_DIR}/external/boost" CACHE PATH "Boost root directory")
        set(Boost_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/boost" CACHE PATH "Boost include directory")
        set(Boost_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/external/boost" CACHE PATH "Boost include directories")
        set(Boost_FOUND TRUE CACHE BOOL "Boost found")
        set(Boost_LIBRARIES "" CACHE STRING "Boost libraries (header-only)")
        set(Boost_NO_SYSTEM_PATHS ON CACHE BOOL "Do not search system for Boost")
        set(Boost_NO_BOOST_CMAKE ON CACHE BOOL "Disable BoostConfig.cmake search")

        # For header-only Boost, we just need the include directory
        include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/external/boost")

        message(STATUS "SymEngine for WASM: Using Boost.Multiprecision (header-only)")
    else()
        # Native builds: Currently using Boost.Multiprecision for consistency
        # TODO: For production Tauri/server builds, enable GMP for better performance:
        # set(INTEGER_CLASS "gmp" CACHE STRING "Integer class for SymEngine" FORCE)

        # For now, use Boost.Multiprecision (same as WASM)
        set(INTEGER_CLASS "boostmp" CACHE STRING "Integer class for SymEngine" FORCE)
        set(WITH_BOOST ON CACHE BOOL "Build with Boost (header-only for boostmp)" FORCE)

        # Set Boost paths for native builds
        set(BOOST_ROOT "${CMAKE_SOURCE_DIR}/external/boost" CACHE PATH "Boost root directory")
        set(Boost_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/boost" CACHE PATH "Boost include directory")
        set(Boost_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/external/boost" CACHE PATH "Boost include directories")
        set(Boost_FOUND TRUE CACHE BOOL "Boost found")
        set(Boost_LIBRARIES "" CACHE STRING "Boost libraries (header-only)")
        set(Boost_NO_SYSTEM_PATHS ON CACHE BOOL "Do not search system for Boost")
        set(Boost_NO_BOOST_CMAKE ON CACHE BOOL "Disable BoostConfig.cmake search")

        include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/external/boost")

        message(STATUS "SymEngine for Native: Using Boost.Multiprecision (header-only)")
    endif()

    # Disable all other external dependencies
    set(WITH_MPFR OFF CACHE BOOL "Build with MPFR" FORCE)
    set(WITH_MPC OFF CACHE BOOL "Build with MPC" FORCE)
    set(WITH_ARB OFF CACHE BOOL "Build with ARB" FORCE)
    set(WITH_FLINT OFF CACHE BOOL "Build with FLINT" FORCE)
    set(WITH_SYMENGINE_VECTORIZER OFF CACHE BOOL "Build with vectorizer" FORCE)
    set(WITH_LLVM OFF CACHE BOOL "Build with LLVM" FORCE)
    set(WITH_BFD OFF CACHE BOOL "Build with BFD" FORCE)
    set(WITH_COTIRE OFF CACHE BOOL "Build with cotire" FORCE)
    set(WITH_SYMENGINE_THREAD_SAFE OFF CACHE BOOL "Thread safe" FORCE)
    set(WITH_SYMENGINE_RCP OFF CACHE BOOL "Use RCP" FORCE)

    # Add SymEngine subdirectory
    if(EXISTS "${CMAKE_SOURCE_DIR}/external/symengine/CMakeLists.txt")
        # For builds with Boost, we need to bypass find_package(Boost) since we're using header-only
        if(WITH_BOOST)
            # Set Boost variables before SymEngine tries to find it
            set(Boost_FOUND TRUE PARENT_SCOPE)
            set(Boost_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/external/boost" PARENT_SCOPE)
            set(Boost_LIBRARIES "" PARENT_SCOPE)

            # Also set them in current scope
            set(Boost_FOUND TRUE)
            set(Boost_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/external/boost")
            set(Boost_LIBRARIES "")

            # Create a FindBoost.cmake override with correct include paths
            # Boost modular structure has headers in libs/*/include
            set(BOOST_INCLUDE_PATHS
                "${CMAKE_SOURCE_DIR}/external/boost/libs/multiprecision/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/config/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/assert/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/core/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/integer/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/mpl/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/preprocessor/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/static_assert/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/throw_exception/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/type_traits/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/predef/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/random/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/system/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/utility/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/move/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/detail/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/io/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/range/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/concept_check/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/iterator/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/function_types/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/fusion/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/optional/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/smart_ptr/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/tuple/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/typeof/include"
                "${CMAKE_SOURCE_DIR}/external/boost/libs/array/include"
            )
            string(REPLACE ";" "\" \"" BOOST_INCLUDE_PATHS_STR "${BOOST_INCLUDE_PATHS}")

            file(WRITE "${CMAKE_BINARY_DIR}/FindBoost.cmake" "
                set(Boost_FOUND TRUE)
                set(Boost_INCLUDE_DIRS \"${BOOST_INCLUDE_PATHS_STR}\")
                set(Boost_LIBRARIES \"\")
                set(HAVE_SYMENGINE_BOOST yes)
            ")
            list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_BINARY_DIR}")
        endif()

        add_subdirectory(external/symengine EXCLUDE_FROM_ALL)
    endif()
endif()

# Debug sources
if(WITH_DEBUG_FEATURES AND WITH_TREE_SITTER)
    set(DEBUG_SOURCES
        src/core/debug/source_map.cpp
        src/core/debug/error_reporting.cpp
        src/core/debug/interactive_debugger.cpp
        src/core/debug/debug_cli.cpp
    )
endif()

# Object library for shared compilation (avoids recompiling for multiple targets)
add_library(madola_core_obj OBJECT ${CORE_SOURCES})
add_dependencies(madola_core_obj generate_css_header)

# Target-specific include directories (better than global include_directories)
target_include_directories(madola_core_obj PUBLIC
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/external/eigen
)

# Add SymEngine include directories if enabled
if(WITH_SYMENGINE)
    target_include_directories(madola_core_obj PUBLIC
        ${CMAKE_SOURCE_DIR}/external/symengine
        ${CMAKE_SOURCE_DIR}/external/symengine/symengine
        ${CMAKE_BINARY_DIR}/external/symengine
        ${CMAKE_BINARY_DIR}/external/symengine/symengine
        ${CMAKE_BINARY_DIR}/external/symengine/symengine/utilities/teuchos
    )

    # For builds with Boost, add Boost include paths to madola_core_obj
    if(WITH_BOOST)
        target_include_directories(madola_core_obj SYSTEM PUBLIC
            ${CMAKE_SOURCE_DIR}/external/boost/libs/multiprecision/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/config/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/assert/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/core/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/integer/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/mpl/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/preprocessor/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/static_assert/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/throw_exception/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/type_traits/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/predef/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/random/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/system/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/utility/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/move/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/detail/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/io/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/range/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/concept_check/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/iterator/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/function_types/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/fusion/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/optional/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/smart_ptr/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/tuple/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/typeof/include
            ${CMAKE_SOURCE_DIR}/external/boost/libs/array/include
        )
    endif()
endif()

target_include_directories(madola_core_obj PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/tree-sitter/lib/include
    ${CMAKE_SOURCE_DIR}/vendor/tree-sitter/lib/src
    ${CMAKE_SOURCE_DIR}/tree-sitter-madola/src
)

# Static library using object library (for linking)
add_library(madola_core STATIC $<TARGET_OBJECTS:madola_core_obj>)

# Enable precompiled headers for faster compilation (apply to object library)
if(USE_PRECOMPILED_HEADERS AND NOT EMSCRIPTEN)
    target_precompile_headers(madola_core_obj PRIVATE
        <memory>
        <string>
        <vector>
        <map>
        <unordered_map>
        <stdexcept>
        <sstream>
        <cmath>
        <algorithm>
    )
endif()

# Enable unity builds if requested (apply to object library)
if(USE_UNITY_BUILD)
    set_target_properties(madola_core_obj PROPERTIES UNITY_BUILD ON)
    set_target_properties(madola_core_obj PROPERTIES UNITY_BUILD_BATCH_SIZE 8)
endif()

# Main executable (only for native builds)
if(NOT EMSCRIPTEN)
    add_executable(madola src/main.cpp)
    target_link_libraries(madola madola_core)

    # Debug executable (only if debug features are enabled)
    if(WITH_DEBUG_FEATURES AND WITH_TREE_SITTER)
        # Check if nlohmann/json is available (required for source maps)
        find_package(nlohmann_json QUIET)
        if(nlohmann_json_FOUND)
            add_library(madola_debug_lib STATIC ${DEBUG_SOURCES})
            target_link_libraries(madola_debug_lib nlohmann_json::nlohmann_json madola_core)
            target_include_directories(madola_debug_lib PRIVATE
                ${CMAKE_SOURCE_DIR}/src/core
                ${CMAKE_SOURCE_DIR}/vendor/tree-sitter/lib/include
            )

            add_executable(madola-debug src/debug_main.cpp)
            target_link_libraries(madola-debug madola_core madola_debug_lib)
            target_compile_definitions(madola-debug PRIVATE WITH_DEBUG_FEATURES)
        else()
            message(WARNING "nlohmann/json not found. Debug features will be limited.")
            add_library(madola_debug_lib STATIC ${DEBUG_SOURCES})
            target_link_libraries(madola_debug_lib madola_core)
            target_include_directories(madola_debug_lib PRIVATE
                ${CMAKE_SOURCE_DIR}/src/core
                ${CMAKE_SOURCE_DIR}/vendor/tree-sitter/lib/include
            )

            add_executable(madola-debug src/debug_main.cpp)
            target_link_libraries(madola-debug madola_core madola_debug_lib)
            target_compile_definitions(madola-debug PRIVATE WITH_DEBUG_FEATURES)
        endif()
    endif()
endif()

# WASM interface library (for Emscripten)
if(EMSCRIPTEN)
    add_executable(madola_wasm src/wasm_interface.cpp)
    target_link_libraries(madola_wasm madola_core)

    # Enable native WASM exceptions so C++ throw/catch works in Emscripten builds
    target_compile_options(madola_core_obj PRIVATE -fwasm-exceptions)
    target_compile_options(madola_core PRIVATE -fwasm-exceptions)
    target_compile_options(madola_wasm PRIVATE -fwasm-exceptions)

    # WASM-specific flags
    set_target_properties(madola_wasm PROPERTIES
        LINK_FLAGS "\
            -O2 \
            -s WASM=1 \
            -s NO_EXIT_RUNTIME=0 \
            -s NODEJS_CATCH_EXIT=0 \
            -s NODEJS_CATCH_REJECTION=0 \
            -s MODULARIZE=1 \
            -s EXPORT_NAME=MADOLA \
            -s EXPORTED_FUNCTIONS=[_parse_madola,_evaluate_madola,_format_madola,_format_madola_html,_free_result] \
            -s EXPORTED_RUNTIME_METHODS=[ccall,UTF8ToString] \
            -s ALLOW_MEMORY_GROWTH=1 \
            -s INITIAL_MEMORY=67108864 \
            -s STACK_SIZE=8388608 \
            -s ENVIRONMENT=web,node \
            -fwasm-exceptions \
            -s EXIT_RUNTIME=1"
        OUTPUT_NAME "madola"
        SUFFIX ".js"
    )
endif()

# Configure Tree-sitter libraries and linking after executables are defined
if(WITH_TREE_SITTER)
    # Check if Tree-sitter sources exist
    if(EXISTS "${CMAKE_SOURCE_DIR}/vendor/tree-sitter/lib/src/lib.c")
        add_library(tree_sitter STATIC ${TREE_SITTER_SOURCES})
        target_compile_definitions(tree_sitter PRIVATE TREE_SITTER_HIDE_SYMBOLS)

        # Set include directories for tree_sitter
        target_include_directories(tree_sitter PUBLIC
            ${CMAKE_SOURCE_DIR}/vendor/tree-sitter/lib/include
        )
        target_include_directories(tree_sitter PRIVATE
            ${CMAKE_SOURCE_DIR}/vendor/tree-sitter/lib/src
        )

        if(EXISTS "${CMAKE_SOURCE_DIR}/tree-sitter-madola/src/parser.c")
            add_library(madola_grammar STATIC ${MADOLA_GRAMMAR_SOURCES})

            # Set include directories for madola_grammar
            target_include_directories(madola_grammar PUBLIC
                ${CMAKE_SOURCE_DIR}/tree-sitter-madola/src
            )
            target_link_libraries(madola_grammar PUBLIC tree_sitter)

            # Link Tree-sitter to core library (propagates to all targets)
            target_link_libraries(madola_core PUBLIC tree_sitter madola_grammar)
            target_compile_definitions(madola_core_obj PUBLIC WITH_TREE_SITTER)

            # Link to specific targets
            if(NOT EMSCRIPTEN)
                target_compile_definitions(madola PRIVATE WITH_TREE_SITTER)

                # Link Tree-sitter to debug executable if it exists
                if(TARGET madola-debug)
                    target_compile_definitions(madola-debug PRIVATE WITH_TREE_SITTER)
                endif()
            endif()

            if(EMSCRIPTEN)
                target_compile_definitions(madola_wasm PRIVATE WITH_TREE_SITTER)
            endif()
        endif()
    endif()
endif()

# Configure SymEngine linking after executables are defined
if(WITH_SYMENGINE)
    if(TARGET symengine)
        # Link SymEngine to core library (propagates to all targets)
        target_link_libraries(madola_core PUBLIC symengine)
        target_compile_definitions(madola_core_obj PUBLIC WITH_SYMENGINE)

        # Link to specific targets
        if(NOT EMSCRIPTEN)
            target_compile_definitions(madola PRIVATE WITH_SYMENGINE)

            # Link SymEngine to debug executable if it exists
            if(TARGET madola-debug)
                target_compile_definitions(madola-debug PRIVATE WITH_SYMENGINE)
            endif()
        endif()

        if(EMSCRIPTEN)
            target_compile_definitions(madola_wasm PRIVATE WITH_SYMENGINE)
        endif()
    endif()
endif()

# Testing
if(NOT EMSCRIPTEN)
    enable_testing()
    # Use madola.exe for testing with test files (using absolute paths)
    add_test(NAME madola_test_example COMMAND madola ${CMAKE_SOURCE_DIR}/example.mda)

    # Custom target for running tests
    add_custom_target(run_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS madola
        COMMENT "Running all tests"
    )
endif()

# Install targets
if(EMSCRIPTEN)
    # No install targets for WASM builds
else()
    install(TARGETS madola
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
endif()

# Print configuration summary
message(STATUS "==============================================")
message(STATUS "MADOLA Build Configuration")
message(STATUS "==============================================")
message(STATUS "  Build Type:            ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard:          ${CMAKE_CXX_STANDARD}")
message(STATUS "  Eigen Support:         Enabled (external/eigen)")
message(STATUS "  Tree-sitter Support:   ${WITH_TREE_SITTER}")
message(STATUS "  SymEngine Support:     ${WITH_SYMENGINE}")
message(STATUS "  Unity Build:           ${USE_UNITY_BUILD}")
message(STATUS "  Precompiled Headers:   ${USE_PRECOMPILED_HEADERS}")
if(CCACHE_PROGRAM)
message(STATUS "  Build Cache (ccache):  Enabled")
else()
message(STATUS "  Build Cache (ccache):  Not found")
endif()
if(EMSCRIPTEN)
message(STATUS "  Target:                WASM (Emscripten)")
else()
message(STATUS "  Target:                Native")
endif()
message(STATUS "  Parallel Jobs:         ${CMAKE_BUILD_PARALLEL_LEVEL}")
message(STATUS "==============================================")
