#!/bin/bash

# Regression test runner for MADOLA (with parallel execution)
# Usage: ./run_regression.sh [native|wasm|update] [update]
#   ./run_regression.sh             - Run native tests
#   ./run_regression.sh native      - Run native tests
#   ./run_regression.sh wasm        - Run WASM tests
#   ./run_regression.sh update      - Run native tests and update baselines
#   ./run_regression.sh native update - Run native tests and update baselines
#   ./run_regression.sh wasm update   - Run WASM tests and update baselines

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
FIXTURES_DIR="$SCRIPT_DIR/fixtures"
OUTPUT_DIR="$SCRIPT_DIR/results"
EXPECTED_DIR="$SCRIPT_DIR/expected"
DIFF_DIR="$SCRIPT_DIR/diff"

# Function to find node executable (handles Windows paths in Git Bash)
find_node() {
    # First try standard node command
    if command -v node >/dev/null 2>&1; then
        echo "node"
        return 0
    fi

    # Try common Windows installation paths
    if [ -f "/c/Program Files/nodejs/node.exe" ]; then
        echo "/c/Program Files/nodejs/node.exe"
        return 0
    elif [ -f "/c/Program Files (x86)/nodejs/node.exe" ]; then
        echo "/c/Program Files (x86)/nodejs/node.exe"
        return 0
    fi

    # Try to find via Windows where command (if running in Git Bash on Windows)
    if command -v cmd.exe >/dev/null 2>&1; then
        local node_path=$(cmd.exe /c "where node.exe 2>nul" | head -1 | tr -d '\r')
        if [ -n "$node_path" ]; then
            # Convert Windows path to Unix-style path for Git Bash
            node_path=$(echo "$node_path" | sed 's|\\|/|g' | sed 's|^\([A-Za-z]\):|/\L\1|')
            echo "$node_path"
            return 0
        fi
    fi

    return 1
}

# Set NODE_CMD for WASM tests
NODE_CMD=""
if [ "${1:-native}" = "wasm" ]; then
    NODE_CMD=$(find_node) || {
        echo "Error: Node.js not found. Install Node.js to run WASM regression tests."
        exit 1
    }
fi

# Parse arguments
MODE="${1:-native}"
UPDATE=false

# Check if first argument is "update" - if so, set mode to native and enable update
if [ "$1" = "update" ]; then
    MODE="native"
    UPDATE=true
elif [ "$2" = "update" ]; then
    UPDATE=true
fi

# Use separate expected baselines for WASM mode
if [ "$MODE" = "wasm" ]; then
    EXPECTED_DIR="$SCRIPT_DIR/expected_wasm"
fi

# Validate mode
if [ "$MODE" != "native" ] && [ "$MODE" != "wasm" ]; then
    echo "Error: Invalid mode '$MODE'. Use 'native', 'wasm', or 'update'."
    exit 1
fi

echo "Running regression tests in $MODE mode (parallel execution)..."
if [ "$UPDATE" = true ]; then
    echo "Will update expected results after running tests"
fi

# Ensure directories exist
mkdir -p "$OUTPUT_DIR/evaluation" "$OUTPUT_DIR/html"
mkdir -p "$EXPECTED_DIR/evaluation" "$EXPECTED_DIR/html"
mkdir -p "$DIFF_DIR/evaluation" "$DIFF_DIR/html"

# Clear previous output and diff
rm -f "$OUTPUT_DIR/evaluation"/* "$OUTPUT_DIR/html"/* 2>/dev/null || true
rm -f "$DIFF_DIR/evaluation"/* "$DIFF_DIR/html"/* 2>/dev/null || true

# Check if madola executable exists
if [ "$MODE" = "native" ]; then
    # For Linux/WSL, use the build directory executable
    # For Windows, use the dist directory executable
    if [ -f "$PROJECT_DIR/build/madola" ]; then
        MADOLA_CMD="$PROJECT_DIR/build/madola"
    elif [ -f "$PROJECT_DIR/dist/madola" ]; then
        MADOLA_CMD="$PROJECT_DIR/dist/madola"
    elif [ -f "$PROJECT_DIR/dist/madola.exe" ]; then
        MADOLA_CMD="$PROJECT_DIR/dist/madola.exe"
    else
        echo "Error: No madola executable found. Run './dev.sh build' or 'dev.bat build' first."
        exit 1
    fi
elif [ "$MODE" = "wasm" ]; then
    MADOLA_JS="$PROJECT_DIR/web/runtime/madola.js"
    MADOLA_WASM="$PROJECT_DIR/web/runtime/madola.wasm"
    if [ ! -f "$MADOLA_JS" ] || [ ! -f "$MADOLA_WASM" ]; then
        echo "Warning: WASM files not found. Tests will fail but output files will be created."
        echo "Run 'dev.bat wasm' to build WASM files for proper testing."
    fi
else
    echo "Error: Invalid mode '$MODE'. Use 'native' or 'wasm'."
    exit 1
fi

# Function to normalize line endings and whitespace for cross-platform comparison
normalize_file() {
    local file="$1"
    if [ -f "$file" ]; then
        # Convert Windows line endings to Unix and remove trailing whitespace
        sed -e 's/\r$//' -e 's/[[:space:]]*$//' "$file"
    fi
}

# Function to run test case
run_test() {
    local test_file="$1"
    local base_name=$(basename "$test_file" .mda)

    echo "Running test: $base_name"

    if [ "$MODE" = "native" ]; then
        # Run native executable
        cd "$FIXTURES_DIR"
        "$MADOLA_CMD" "$base_name.mda" > "$OUTPUT_DIR/evaluation/$base_name.txt" 2>&1 || true
        "$MADOLA_CMD" "$base_name.mda" --html > "$OUTPUT_DIR/html/$base_name.html" 2>&1 || true
    else
        # Run WASM version using madola_runner.js
        cd "$PROJECT_DIR"
        # Run WASM evaluation (equivalent to native execution)
        "$NODE_CMD" madola_runner.js "$test_file" --output "$OUTPUT_DIR/evaluation/$base_name.txt" || true
        # Run WASM HTML formatting (equivalent to --html flag)
        "$NODE_CMD" madola_runner.js "$test_file" --html --output "$OUTPUT_DIR/html/$base_name.html" || true
    fi
}

# Run all test cases in parallel
if [ ! -d "$FIXTURES_DIR" ] || [ -z "$(ls -A "$FIXTURES_DIR" 2>/dev/null)" ]; then
    echo "No test fixtures found in $FIXTURES_DIR"
    echo "Create some .mda files in the fixtures directory first."
    exit 1
fi

test_count=0
pids=()

for test_file in "$FIXTURES_DIR"/*.mda; do
    if [ -f "$test_file" ]; then
        run_test "$test_file" &
        pids+=($!)
        test_count=$((test_count + 1))
    fi
done

# Wait for all background jobs to complete
for pid in "${pids[@]}"; do
    wait $pid
done

echo "Ran $test_count test cases"

# Update expected results if requested
if [ "$UPDATE" = true ]; then
    echo "Updating expected results..."
    cp "$OUTPUT_DIR/evaluation"/* "$EXPECTED_DIR/evaluation/" 2>/dev/null || true
    cp "$OUTPUT_DIR/html"/* "$EXPECTED_DIR/html/" 2>/dev/null || true
    echo "Expected results updated from output to expected"
    exit 0
fi

# Compare results and generate diffs
echo ""
echo "Comparing results..."
diff_count=0
pass_count=0

for output_file in "$OUTPUT_DIR/evaluation"/*.txt; do
    if [ -f "$output_file" ]; then
        base_name=$(basename "$output_file")
        expected_file="$EXPECTED_DIR/evaluation/$base_name"
        diff_file="$DIFF_DIR/evaluation/$base_name"

        if [ -f "$expected_file" ]; then
            # Normalize both files and compare
            if normalize_file "$expected_file" | diff -u - <(normalize_file "$output_file") > "$diff_file" 2>/dev/null; then
                rm "$diff_file"  # No differences
                echo "[PASS] $base_name (evaluation)"
                ((pass_count++)) || true
            else
                echo "[FAIL] $base_name (evaluation)"
                ((diff_count++)) || true
            fi
        else
            echo "[NO BASELINE] $base_name (evaluation)"
        fi
    fi
done

for output_file in "$OUTPUT_DIR/html"/*.html; do
    if [ -f "$output_file" ]; then
        base_name=$(basename "$output_file")
        expected_file="$EXPECTED_DIR/html/$base_name"
        diff_file="$DIFF_DIR/html/$base_name"

        if [ -f "$expected_file" ]; then
            # Normalize both files and compare
            if normalize_file "$expected_file" | diff -u - <(normalize_file "$output_file") > "$diff_file" 2>/dev/null; then
                rm "$diff_file"  # No differences
                echo "[PASS] $base_name (html)"
                ((pass_count++)) || true
            else
                echo "[FAIL] $base_name (html)"
                ((diff_count++)) || true
            fi
        else
            echo "[NO BASELINE] $base_name (html)"
        fi
    fi
done

echo ""
echo "Results: $pass_count passed, $diff_count failed"

if [ $diff_count -gt 0 ]; then
    echo ""
    echo "To see differences, check files in: $DIFF_DIR"
    echo "To update baselines, run: ./run_regression.sh $MODE update"
    exit 1
fi

echo "All tests passed!"
