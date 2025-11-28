@echo off
REM Regression test runner for MADOLA (Windows - with parallel execution)
REM Usage: run_regression.bat [native|wasm|update] [update]
REM   run_regression.bat             - Run native tests
REM   run_regression.bat native      - Run native tests
REM   run_regression.bat wasm        - Run WASM tests
REM   run_regression.bat update      - Update native baselines
REM   run_regression.bat native update - Update native baselines
REM   run_regression.bat wasm update   - Update WASM baselines

setlocal enabledelayedexpansion

REM Configuration
set SCRIPT_DIR=%~dp0
set PROJECT_DIR=%SCRIPT_DIR%..
set FIXTURES_DIR=%SCRIPT_DIR%fixtures
set OUTPUT_DIR=%SCRIPT_DIR%results
set EXPECTED_DIR=%SCRIPT_DIR%expected
set DIFF_DIR=%SCRIPT_DIR%diff
set TEMP_DIR=%SCRIPT_DIR%temp

REM Parse arguments
set MODE=%1
set UPDATE=false

REM Check if first argument is "update" - if so, set mode to native and enable update
if "%1"=="update" (
    set MODE=native
    set UPDATE=true
) else (
    if "%MODE%"=="" set MODE=native
    if "%2"=="update" set UPDATE=true
)

REM Use separate expected baselines for WASM mode
if /I "%MODE%"=="wasm" set EXPECTED_DIR=%SCRIPT_DIR%expected_wasm

echo Running regression tests in %MODE% mode (parallel execution)...
if "%UPDATE%"=="true" echo Will update expected results after running tests

REM Ensure directories exist
if not exist "%OUTPUT_DIR%\evaluation" mkdir "%OUTPUT_DIR%\evaluation"
if not exist "%OUTPUT_DIR%\html" mkdir "%OUTPUT_DIR%\html"
if not exist "%EXPECTED_DIR%\evaluation" mkdir "%EXPECTED_DIR%\evaluation"
if not exist "%EXPECTED_DIR%\html" mkdir "%EXPECTED_DIR%\html"
if not exist "%DIFF_DIR%\evaluation" mkdir "%DIFF_DIR%\evaluation"
if not exist "%DIFF_DIR%\html" mkdir "%DIFF_DIR%\html"
if not exist "%TEMP_DIR%" mkdir "%TEMP_DIR%"

REM Clear previous output and diff
del /Q "%OUTPUT_DIR%\evaluation\*" 2>nul
del /Q "%OUTPUT_DIR%\html\*" 2>nul
del /Q "%DIFF_DIR%\evaluation\*" 2>nul
del /Q "%DIFF_DIR%\html\*" 2>nul
del /Q "%TEMP_DIR%\*.flag" 2>nul

REM Check if madola executable exists
if "%MODE%"=="native" (
    set MADOLA_CMD=%PROJECT_DIR%\dist\madola.exe
    if not exist "!MADOLA_CMD!" (
        echo Error: !MADOLA_CMD! not found. Run 'dev.bat build' first.
        exit /b 1
    )
) else if "%MODE%"=="wasm" (
    set MADOLA_RUNNER=%PROJECT_DIR%\madola_runner.js
    set MADOLA_JS=%PROJECT_DIR%\web\runtime\madola.js
    set MADOLA_WASM=%PROJECT_DIR%\web\runtime\madola.wasm
    if not exist "!MADOLA_JS!" (
        echo Error: WASM files not found. Run 'dev.bat wasm' first.
        exit /b 1
    )
    if not exist "!MADOLA_RUNNER!" (
        echo Error: madola_runner.js not found.
        exit /b 1
    )
) else (
    echo Error: Invalid mode '%MODE%'. Use 'native' or 'wasm'.
    exit /b 1
)

REM Run all test cases in parallel using start /b
set test_count=0
for %%f in ("%FIXTURES_DIR%\*.mda") do (
    set test_file=%%f
    set base_name=%%~nf
    
    echo Running test: !base_name!
    
    if "%MODE%"=="native" (
        REM Run native executable in background, always write flag
        start /b "" cmd /c "pushd "%FIXTURES_DIR%" && ( "%MADOLA_CMD%" "!base_name!.mda" > "%OUTPUT_DIR%\evaluation\!base_name!.txt" 2>&1 || ver > nul ) && ( "%MADOLA_CMD%" "!base_name!.mda" --html > "%OUTPUT_DIR%\html\!base_name!.html" 2>&1 || ver > nul ) & popd & echo done > "%TEMP_DIR%\!base_name!.flag""
    ) else (
        REM Run WASM version in background, always write flag
        start /b "" cmd /c "pushd "%PROJECT_DIR%" && ( node madola_runner.js "!test_file!" --output "%OUTPUT_DIR%\evaluation\!base_name!.txt" 2>&1 || ver > nul ) && ( node madola_runner.js "!test_file!" --html --output "%OUTPUT_DIR%\html\!base_name!.html" 2>&1 || ver > nul ) & popd & echo done > "%TEMP_DIR%\!base_name!.flag""
    )
    
    set /a test_count+=1
)

REM Wait for all background processes to complete by checking flag files
echo.
echo Waiting for all tests to complete...
:wait_loop
set completed=0
for %%f in ("%FIXTURES_DIR%\*.mda") do (
    set base_name=%%~nf
    if exist "%TEMP_DIR%\!base_name!.flag" (
        set /a completed+=1
    )
)

if !completed! lss %test_count% (
    timeout /t 1 /nobreak >nul
    goto wait_loop
)

echo Ran %test_count% test cases

REM Clean up temp directory
del /Q "%TEMP_DIR%\*.flag" 2>nul

REM Update expected results if requested
if "%UPDATE%"=="true" (
    echo Updating expected results...
    xcopy /Y "%OUTPUT_DIR%\evaluation\*" "%EXPECTED_DIR%\evaluation\" >nul 2>&1
    xcopy /Y "%OUTPUT_DIR%\html\*" "%EXPECTED_DIR%\html\" >nul 2>&1
    echo Expected results updated
    exit /b 0
)

REM Compare results
echo.
echo Comparing results...
set diff_count=0
set pass_count=0

for %%f in ("%OUTPUT_DIR%\evaluation\*.txt") do (
    set base_name=%%~nxf
    set output_file=%%f
    set expected_file=%EXPECTED_DIR%\evaluation\!base_name!
    
    if exist "!expected_file!" (
        fc "!expected_file!" "!output_file!" >nul 2>&1
        if errorlevel 1 (
            echo [FAIL] !base_name! (evaluation^)
            set /a diff_count+=1
        ) else (
            echo [PASS] !base_name! (evaluation^)
            set /a pass_count+=1
        )
    ) else (
        echo [NO BASELINE] !base_name! (evaluation^)
    )
)

for %%f in ("%OUTPUT_DIR%\html\*.html") do (
    set base_name=%%~nxf
    set output_file=%%f
    set expected_file=%EXPECTED_DIR%\html\!base_name!
    
    if exist "!expected_file!" (
        fc "!expected_file!" "!output_file!" >nul 2>&1
        if errorlevel 1 (
            echo [FAIL] !base_name! (html^)
            set /a diff_count+=1
        ) else (
            echo [PASS] !base_name! (html^)
            set /a pass_count+=1
        )
    ) else (
        echo [NO BASELINE] !base_name! (html^)
    )
)

echo.
echo Results: %pass_count% passed, %diff_count% failed

if %diff_count% gtr 0 (
    echo.
    echo Some tests failed. To update baselines, run: run_regression.bat %MODE% update
    exit /b 1
)

echo All tests passed!
exit /b 0
