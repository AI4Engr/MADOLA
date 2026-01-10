@echo off
setlocal enabledelayedexpansion

REM MADOLA Development Script - Windows
REM IMPORTANT: Run this script from Windows Command Prompt (cmd.exe), NOT Git Bash
REM Git Bash has path translation issues that break MinGW-w64 compilation

REM Detect if running from Git Bash / MSYS
if defined MSYSTEM (
    echo.
    echo [WARNING] ========================================
    echo [WARNING] You are running from Git Bash/MSYS2
    echo [WARNING] This may cause build failures!
    echo [WARNING]
    echo [WARNING] Please run from Windows Command Prompt:
    echo [WARNING]   1. Open cmd.exe
    echo [WARNING]   2. cd C:\project\MADOLA
    echo [WARNING]   3. dev.bat build
    echo [WARNING] ========================================
    echo.
)

REM Check if CMake is available
cmake --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found. Please install CMake.
    exit /b 1
)

REM Function to find tree-sitter executable
REM First try local node_modules/.bin, then global
call :find_tree_sitter
if errorlevel 1 (
    set TREE_SITTER_NOT_FOUND=1
) else (
    set TREE_SITTER_NOT_FOUND=0
)

goto :main_commands

:find_tree_sitter
REM Check local node_modules/.bin first
if exist "node_modules\.bin\tree-sitter.cmd" (
    set "TREE_SITTER=node_modules\.bin\tree-sitter.cmd"
    exit /b 0
)
if exist "node_modules\.bin\tree-sitter" (
    set "TREE_SITTER=node_modules\.bin\tree-sitter"
    exit /b 0
)
REM Check global tree-sitter using where command (faster than running it)
where tree-sitter >nul 2>&1
if not errorlevel 1 (
    set "TREE_SITTER=tree-sitter"
    exit /b 0
)
exit /b 1

:main_commands
if "%1"=="" goto :show_help
if "%1"=="help" goto :show_help

if "%1"=="generate-grammar" (
    echo [INFO] Generating Tree-sitter parser from grammar...
    if !TREE_SITTER_NOT_FOUND! equ 1 (
        echo [ERROR] Tree-sitter CLI not found. Install: npm install tree-sitter-cli
        goto :eof
    )
    cd /d "%~dp0tree-sitter-madola"
    call %TREE_SITTER% generate --abi=latest
    if errorlevel 1 (
        echo [ERROR] Failed to generate Tree-sitter parser
        cd /d "%~dp0"
        goto :eof
    )
    cd /d "%~dp0"
    echo [INFO] Grammar generation complete
    goto :eof
)

if "%1"=="setup" (
    echo [INFO] Setting up MADOLA development environment...

    REM Initialize only the necessary submodules (non-recursive)
    echo [INFO] Initializing tree-sitter submodule...
    git submodule update --init --depth 1 vendor/tree-sitter

    echo [INFO] Initializing eigen submodule...
    git submodule update --init --depth 1 external/eigen

    echo [INFO] Initializing symengine submodule...
    git submodule update --init --depth 1 external/symengine

    REM Initialize Boost with shallow clone and sparse checkout for only needed libraries
    echo [INFO] Initializing Boost submodule ^(selective libraries only^)...
    git submodule update --init --depth 1 external/boost

    REM Initialize Boost library submodules (each Boost library is a submodule)
    echo [INFO] Initializing Boost library submodules ^(27 libraries^)...
    cd /d "%~dp0external\boost"
    git submodule update --init --depth 1 ^
        libs/multiprecision libs/config libs/assert libs/core ^
        libs/integer libs/mpl libs/preprocessor libs/static_assert ^
        libs/throw_exception libs/type_traits libs/predef libs/random ^
        libs/system libs/utility libs/move libs/detail libs/io ^
        libs/range libs/concept_check libs/iterator libs/function_types ^
        libs/fusion libs/optional libs/smart_ptr libs/tuple ^
        libs/typeof libs/array
    cd /d "%~dp0"

    if !TREE_SITTER_NOT_FOUND! equ 1 (
        echo [ERROR] Tree-sitter CLI not found. Install: npm install tree-sitter-cli
        goto :eof
    )
    cd /d "%~dp0tree-sitter-madola"
    call %TREE_SITTER% generate --abi=latest
    if errorlevel 1 (
        echo [ERROR] Failed to generate Tree-sitter parser
        cd /d "%~dp0"
        goto :eof
    )
    cd /d "%~dp0"
    echo [INFO] Setup complete - optimized submodule cloning saved ~1GB ^(131MB vs 1.2GB+^)
    goto :eof
)

if "%1"=="build" (
    echo [INFO] Building MADOLA...

    REM Clean dist executables first
    if not exist dist mkdir dist
    if exist dist\madola.exe del /q dist\madola.exe
    if exist dist\madola del /q dist\madola
    if exist dist\madola-debug.exe del /q dist\madola-debug.exe
    if exist dist\madola-debug del /q dist\madola-debug

    REM Generate parser from grammar if needed
    if !TREE_SITTER_NOT_FOUND! equ 1 (
        echo [ERROR] Tree-sitter CLI not found. Install: npm install tree-sitter-cli
        goto :eof
    )

    REM Check if parser needs regeneration (grammar.js newer than parser.c or parser.c missing)
    set NEED_GENERATE=0
    if not exist "tree-sitter-madola\src\parser.c" set NEED_GENERATE=1
    if exist "tree-sitter-madola\grammar.js" if exist "tree-sitter-madola\src\parser.c" (
        for %%A in ("tree-sitter-madola\grammar.js") do set GRAMMAR_TIME=%%~tA
        for %%B in ("tree-sitter-madola\src\parser.c") do set PARSER_TIME=%%~tB
        if "!GRAMMAR_TIME!" GTR "!PARSER_TIME!" set NEED_GENERATE=1
    )

    if !NEED_GENERATE! equ 1 (
        echo [INFO] Regenerating Tree-sitter parser from grammar.js...
        cd /d "%~dp0tree-sitter-madola"
        call %TREE_SITTER% generate
        if errorlevel 1 (
            echo [ERROR] Failed to generate Tree-sitter parser
            cd /d "%~dp0"
            goto :eof
        )
        cd /d "%~dp0"
        echo [INFO] Parser generation complete
    ) else (
        echo [INFO] Parser is up to date, skipping regeneration
    )

    if not exist build mkdir build
    echo [INFO] Configuring CMake build...
    cd /d "%~dp0build"

    REM Try to use MinGW-w64 if available, otherwise fall back to system compiler
    where /q C:\apps\msys64\mingw64\bin\gcc.exe
    if not errorlevel 1 (
        echo [INFO] Using MinGW-w64 compiler
        cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=C:/apps/msys64/mingw64/bin/gcc.exe -DCMAKE_CXX_COMPILER=C:/apps/msys64/mingw64/bin/g++.exe
    ) else (
        echo [INFO] Using system compiler
        cmake .. -DCMAKE_BUILD_TYPE=Debug
    )
    if errorlevel 1 (
        echo [ERROR] CMake configuration failed
        cd /d "%~dp0"
        goto :eof
    )

    echo [INFO] Building project...
    cmake --build . --parallel
    if errorlevel 1 (
        echo [ERROR] Build failed
        cd /d "%~dp0"
        goto :eof
    )
    cd /d "%~dp0"
    echo [INFO] Build successful

    REM Copy to dist/
    if not exist dist mkdir dist
    if exist build\madola.exe copy build\madola.exe dist\
    if exist build\madola copy build\madola dist\
    if exist build\madola-debug.exe copy build\madola-debug.exe dist\
    if exist build\madola-debug copy build\madola-debug dist\
    echo [INFO] Build complete

    REM Show available executables
    echo [INFO] Built executables:
    if exist dist\madola.exe echo    - dist\madola.exe ^(main compiler^)
    if exist dist\madola echo    - dist\madola ^(main compiler^)
    if exist dist\madola-debug.exe echo    - dist\madola-debug.exe ^(interactive debugger^)
    if exist dist\madola-debug echo    - dist\madola-debug ^(interactive debugger^)
    goto :eof
)

if "%1"=="wasm" (
    echo [INFO] Building WASM with CMake + Emscripten...
    emcc --version >nul 2>&1
    if errorlevel 1 (
        echo [ERROR] Emscripten not found. Install Emscripten.
        goto :eof
    )

    REM Generate parser from grammar first
    if !TREE_SITTER_NOT_FOUND! equ 1 (
        echo [ERROR] Tree-sitter CLI not found. Install: npm install tree-sitter-cli
        goto :eof
    )
    cd tree-sitter-madola
    %TREE_SITTER% generate
    cd ..

    REM Create build directory if it doesn't exist
    if not exist build_wasm mkdir build_wasm
    cd build_wasm

    REM Configure with emcmake
    call emcmake cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release

    REM Build with emmake
    call emmake cmake --build . --parallel

    cd ..

    REM Copy WASM output to web\runtime
    if not exist web\runtime mkdir web\runtime
    if exist build_wasm\madola.js copy /Y build_wasm\madola.js web\runtime\
    if exist build_wasm\madola.wasm copy /Y build_wasm\madola.wasm web\runtime\

    echo [INFO] WASM build complete
    echo [INFO] Output files:
    echo    - web\runtime\madola.js
    echo    - web\runtime\madola.wasm
    goto :eof
)

if "%1"=="debug" (
    echo [INFO] Starting interactive debugger...
    if not exist dist\madola-debug.exe if not exist dist\madola-debug call %0 build

    set EXEC=dist\madola-debug
    if exist dist\madola-debug.exe set EXEC=dist\madola-debug.exe

    set FILE=%2
    if "%FILE%"=="" set FILE=example.mda

    if not exist !FILE! (
        echo [ERROR] File not found: !FILE!
        exit /b 1
    )

    echo [INFO] Starting interactive debugger for !FILE!...
    echo [INFO] Type 'help' for debugging commands
    !EXEC! !FILE! %3 %4 %5 %6 %7 %8 %9
    goto :eof
)

if "%1"=="test" (
    echo [INFO] Running tests...
    if not exist dist\madola.exe if not exist dist\madola call %0 build
    cd build
    ctest --output-on-failure
    cd ..
    goto :eof
)

if "%1"=="run" (
    if not exist dist\madola.exe if not exist dist\madola call %0 build
    set EXEC=dist\madola
    if exist dist\madola.exe set EXEC=dist\madola.exe

    set FILE=%2
    if "%FILE%"=="" set FILE=example.mda

    set MODE=%3
    if "%MODE%"=="" set MODE=eval

    if "!MODE!"=="eval" (
        echo [INFO] Evaluating !FILE!...
        !EXEC! !FILE!
    ) else if "!MODE!"=="evaluate" (
        echo [INFO] Evaluating !FILE!...
        !EXEC! !FILE!
    ) else if "!MODE!"=="md" (
        echo [INFO] Creating markdown from !FILE!...
        set OUTPUT_FILE=!FILE:.mda=.md!
        !EXEC! !FILE! --format > !OUTPUT_FILE! 2>&1
        echo [INFO] Markdown saved to !OUTPUT_FILE!
    ) else if "!MODE!"=="markdown" (
        echo [INFO] Creating markdown from !FILE!...
        set OUTPUT_FILE=!FILE:.mda=.md!
        !EXEC! !FILE! --format > !OUTPUT_FILE! 2>&1
        echo [INFO] Markdown saved to !OUTPUT_FILE!
    ) else if "!MODE!"=="both" (
        echo [INFO] Evaluating !FILE!...
        !EXEC! !FILE!
        echo.
        echo [INFO] Creating markdown from !FILE!...
        set OUTPUT_FILE=!FILE:.mda=.md!
        !EXEC! !FILE! --format > !OUTPUT_FILE!
        echo [INFO] Markdown saved to !OUTPUT_FILE!
    ) else (
        echo [ERROR] Invalid mode: !MODE!. Use 'eval', 'markdown', or 'both'
        exit /b 1
    )
    goto :eof
)

if "%1"=="regression" (
    set "SUBCOMMAND=%~2"
    if "!SUBCOMMAND!"=="" set "SUBCOMMAND=native"

    if "!SUBCOMMAND!"=="native" (
        echo [INFO] Running native regression tests...
        if not exist regression\run_regression.bat (
            echo [ERROR] Regression script not found
            exit /b 1
        )
        if not exist dist\madola.exe if not exist dist\madola call %0 build
        call regression\run_regression.bat native
    ) else if "!SUBCOMMAND!"=="wasm" (
        echo [INFO] Running WASM regression tests...
        if not exist regression\run_regression.bat (
            echo [ERROR] Regression script not found
            exit /b 1
        )
        if not exist web\runtime\madola.js call %0 wasm
        call regression\run_regression.bat wasm
    ) else if "!SUBCOMMAND!"=="update" (
        echo [INFO] Updating regression test expected results...
        if not exist regression\results (
            echo [ERROR] No regression results found. Run tests first.
            exit /b 1
        )
        echo [INFO] Copying results to expected...
        if exist regression\expected rmdir /s /q regression\expected
        xcopy /E /I /Y regression\results regression\expected
        echo [INFO] Expected results updated successfully
        echo [INFO] You can now commit the updated expected results
    ) else (
        echo [ERROR] Invalid regression mode: !SUBCOMMAND!
        echo [INFO] Usage: dev.bat regression [native^|wasm^|update]
        exit /b 1
    )
    goto :eof
)

if "%1"=="serve" (
    echo [INFO] Starting demo server...
    if not exist web\runtime\madola.js call %0 wasm

    REM WASM files are already in web/runtime/ - no copying needed
    echo [INFO] WASM files ready in web/runtime/
    echo [INFO] Function libraries ready in web/trove/

    node --version >nul 2>&1
    if errorlevel 1 (
        echo [ERROR] Node.js not found. Install Node.js to run demo server.
        exit /b 1
    )
    echo [INFO] Starting server at http://localhost:8080
    node web\serve.js
    goto :eof
)

if "%1"=="clean" (
    echo [INFO] Cleaning...
    if exist build rmdir /s /q build
    if exist build_wasm rmdir /s /q build_wasm
    if exist dist rmdir /s /q dist
    if exist web\runtime rmdir /s /q web\runtime
    goto :eof
)

:show_help
echo MADOLA Development Commands:
echo.
echo   generate-grammar - Generate Tree-sitter parser from grammar
echo   setup      - Initialize submodules and generate grammar
echo   build      - Build native executable and interactive debugger
echo   debug [file] [options] - Start interactive debugger ^(default: example.mda^)
echo   wasm       - Build WASM version using CMake + Emscripten
echo   test       - Run test suite
echo   run [file] [mode] - Run with file ^(default: example.mda^)
echo                       Modes: eval ^(default^), markdown, both
echo   regression [native^|wasm^|update] - Run regression tests ^(default: native^)
echo                                        update: Update expected results
echo   serve      - Start demo server at http://localhost:8080
echo   clean      - Clean build artifacts
echo   help       - Show this help
echo.
echo Quick start:
echo   dev.bat setup    # First time setup
echo   dev.bat build    # Build and test
echo   dev.bat wasm     # Build for web
echo   dev.bat serve    # Start web demo
echo.
echo Debugging:
echo   dev.bat debug example.mda          # Start interactive debugger
echo   dev.bat debug example.mda --break 5 # Start with breakpoint at line 5
echo.
echo Run examples:
echo   dev.bat run test.mda          # Evaluate test.mda
echo   dev.bat run test.mda markdown # Create test.md
echo   dev.bat run test.mda both     # Both evaluate and create markdown