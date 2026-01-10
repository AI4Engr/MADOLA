#!/bin/bash

# MADOLA Development Script - Simplified

set -e

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

info() { echo -e "${GREEN}[INFO]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }

# Check dependencies
command -v cmake >/dev/null || error "CMake not found. Please install CMake."

# Function to find tree-sitter executable
find_tree_sitter() {
    # First try to set up environment for local node_modules/.bin
    if [ -f "node_modules/.bin/tree-sitter" ]; then
        # Add local node_modules/.bin to PATH and use tree-sitter
        local project_root="$(pwd)"
        export PATH="${project_root}/node_modules/.bin:$PATH"
        echo "tree-sitter"
    # Then try global tree-sitter
    elif command -v tree-sitter >/dev/null; then
        echo "tree-sitter"
    else
        return 1
    fi
}

# Main commands
case "${1:-help}" in
    "generate-grammar")
        info "Generating Tree-sitter parser from grammar..."
        TREE_SITTER=$(find_tree_sitter) || error "Tree-sitter CLI not found. Install: npm install tree-sitter-cli"
        cd tree-sitter-madola && $TREE_SITTER generate --abi=latest && cd ..
        info "✅ Grammar generation complete"
        ;;

    "setup")
        info "Setting up MADOLA development environment..."

        # Initialize only the necessary submodules (non-recursive)
        info "Initializing tree-sitter submodule..."
        git submodule update --init --depth 1 vendor/tree-sitter

        info "Initializing eigen submodule..."
        git submodule update --init --depth 1 external/eigen

        info "Initializing symengine submodule..."
        git submodule update --init --depth 1 external/symengine

        # Initialize Boost with shallow clone and sparse checkout for only needed libraries
        info "Initializing Boost submodule (selective libraries only)..."
        git submodule update --init --depth 1 external/boost

        # Initialize Boost library submodules (each Boost library is a submodule)
        info "Initializing Boost library submodules (27 libraries)..."
        cd external/boost
        git submodule update --init --depth 1 \
            libs/multiprecision libs/config libs/assert libs/core \
            libs/integer libs/mpl libs/preprocessor libs/static_assert \
            libs/throw_exception libs/type_traits libs/predef libs/random \
            libs/system libs/utility libs/move libs/detail libs/io \
            libs/range libs/concept_check libs/iterator libs/function_types \
            libs/fusion libs/optional libs/smart_ptr libs/tuple \
            libs/typeof libs/array
        cd ../..

        TREE_SITTER=$(find_tree_sitter) || error "Tree-sitter CLI not found. Install: npm install tree-sitter-cli"
        cd tree-sitter-madola && $TREE_SITTER generate && cd ..
        info "✅ Setup complete - optimized submodule cloning saved ~1GB (131MB vs 1.2GB+)"
        ;;

    "build")
        info "Building MADOLA..."

        # Clean dist executables first
        mkdir -p dist
        [ -f dist/madola ] && rm dist/madola
        [ -f dist/madola.exe ] && rm dist/madola.exe
        [ -f dist/madola-debug ] && rm dist/madola-debug
        [ -f dist/madola-debug.exe ] && rm dist/madola-debug.exe

        # Generate parser from grammar if needed
        TREE_SITTER=$(find_tree_sitter) || error "Tree-sitter CLI not found. Install: npm install tree-sitter-cli"

        # Check if parser needs regeneration (grammar.js newer than parser.c or parser.c missing)
        if [ ! -f "tree-sitter-madola/src/parser.c" ] || [ "tree-sitter-madola/grammar.js" -nt "tree-sitter-madola/src/parser.c" ]; then
            info "Regenerating Tree-sitter parser from grammar.js..."
            cd tree-sitter-madola && $TREE_SITTER generate && cd ..
            info "✅ Parser generation complete"
        else
            info "Parser is up to date, skipping regeneration"
        fi

        mkdir -p build
        cd build
        cmake .. -DCMAKE_BUILD_TYPE=Debug
        cmake --build . --parallel
        cd ..

        # Copy to dist/
        mkdir -p dist
        [ -f build/madola ] && cp build/madola dist/
        [ -f build/madola.exe ] && cp build/madola.exe dist/
        [ -f build/madola-debug ] && cp build/madola-debug dist/
        [ -f build/madola-debug.exe ] && cp build/madola-debug.exe dist/
        info "✅ Build complete"

        # Show available executables
        info "📁 Built executables:"
        [ -f dist/madola.exe ] && info "   • dist/madola.exe (main compiler)"
        [ -f dist/madola ] && info "   • dist/madola (main compiler)"
        [ -f dist/madola-debug.exe ] && info "   • dist/madola-debug.exe (interactive debugger)"
        [ -f dist/madola-debug ] && info "   • dist/madola-debug (interactive debugger)"
        ;;

    "wasm")
        info "Building WASM with CMake + Emscripten..."
        command -v emcc >/dev/null || error "Emscripten not found. Install Emscripten."

        # Generate parser from grammar first if needed
        TREE_SITTER=$(find_tree_sitter) || error "Tree-sitter CLI not found. Install: npm install tree-sitter-cli"
        if [ ! -f "tree-sitter-madola/src/parser.c" ] || [ "tree-sitter-madola/grammar.js" -nt "tree-sitter-madola/src/parser.c" ]; then
            info "Regenerating Tree-sitter parser from grammar.js..."
            cd tree-sitter-madola && $TREE_SITTER generate && cd ..
        fi

        # Create build directory if it doesn't exist
        mkdir -p build_wasm
        cd build_wasm

        # Configure with emcmake
        emcmake cmake .. -DCMAKE_BUILD_TYPE=Release

        # Build with emmake
        emmake cmake --build . --parallel

        cd ..

        # Copy WASM output to web/runtime
        mkdir -p web/runtime
        [ -f build_wasm/madola.js ] && cp build_wasm/madola.js web/runtime/
        [ -f build_wasm/madola.wasm ] && cp build_wasm/madola.wasm web/runtime/

        info "✅ WASM build complete"
        info "📁 Output files:"
        info "   • web/runtime/madola.js"
        info "   • web/runtime/madola.wasm"
        ;;

    "debug")
        info "Starting interactive debugger..."
        [ ! -f dist/madola-debug ] && [ ! -f dist/madola-debug.exe ] && $0 build

        EXEC="dist/madola-debug"
        [ -f "dist/madola-debug.exe" ] && EXEC="dist/madola-debug.exe"

        FILE=${2:-example.mda}

        if [ ! -f "$FILE" ]; then
            error "File not found: $FILE"
        fi

        info "🐛 Starting interactive debugger for $FILE..."
        info "Type 'help' for debugging commands"
        ${EXEC} "$FILE" "${@:3}"
        ;;

    "test")
        info "Running tests..."
        [ ! -f dist/madola ] && [ ! -f dist/madola.exe ] && $0 build
        cd build && ctest --output-on-failure && cd ..
        ;;

    "run")
        [ ! -f dist/madola ] && [ ! -f dist/madola.exe ] && $0 build
        EXEC="dist/madola"
        [ -f "dist/madola.exe" ] && EXEC="dist/madola.exe"

        FILE=${2:-example.mda}
        MODE=${3:-eval}

        case "$MODE" in
            "eval"|"evaluate")
                info "Evaluating $FILE..."
                ${EXEC} "$FILE"
                ;;
            "md"|"markdown")
                info "Creating markdown from $FILE..."
                OUTPUT_FILE="${FILE%.mda}.md"
                ${EXEC} "$FILE" --format > "$OUTPUT_FILE" 2>&1
                info "✅ Markdown saved to $OUTPUT_FILE"
                ;;
            "both")
                info "Evaluating $FILE..."
                ${EXEC} "$FILE"
                echo ""
                info "Creating markdown from $FILE..."
                OUTPUT_FILE="${FILE%.mda}.md"
                ${EXEC} "$FILE" --format > "$OUTPUT_FILE"
                info "✅ Markdown saved to $OUTPUT_FILE"
                ;;
            *)
                error "Invalid mode: $MODE. Use 'eval', 'markdown', or 'both'"
                ;;
        esac
        ;;

    "regression")
        SUBCOMMAND=${2:-native}

        case "$SUBCOMMAND" in
            "native")
                info "Running native regression tests..."
                [ -f regression/run_regression.sh ] || error "Regression script not found"
                [ ! -f dist/madola ] && [ ! -f dist/madola.exe ] && $0 build
                ./regression/run_regression.sh native
                ;;
            "wasm")
                info "Running WASM regression tests..."
                [ -f regression/run_regression.sh ] || error "Regression script not found"
                [ ! -f web/runtime/madola.js ] && $0 wasm
                ./regression/run_regression.sh wasm
                ;;
            *)
                error "Invalid regression mode: $SUBCOMMAND. Usage: ./dev.sh regression [native|wasm]"
                ;;
        esac
        ;;

    "serve")
        info "Starting demo server..."
        [ ! -f web/runtime/madola.js ] && $0 wasm

        # WASM files are already in web/runtime/ - no copying needed
        info "WASM files ready in web/runtime/"
        info "Function libraries ready in web/trove/"

        command -v node >/dev/null || error "Node.js not found. Install Node.js to run demo server."
        info "🚀 Starting server at http://localhost:8080"
        node web/serve.js
        ;;

    "clean")
        info "Cleaning..."
        rm -rf build/ build_wasm/ dist/ web/runtime/
        ;;

    "help"|*)
        echo "MADOLA Development Commands:"
        echo ""
        echo "  generate-grammar - Generate Tree-sitter parser from grammar"
        echo "  setup      - Initialize submodules and generate grammar"
        echo "  build      - Build native executable and interactive debugger"
        echo "  debug [file] [options] - Start interactive debugger (default: example.mda)"
        echo "  wasm       - Build WASM version using CMake + Emscripten"
        echo "  test       - Run test suite"
        echo "  run [file] [mode] - Run with file (default: example.mda)"
        echo "                      Modes: eval (default), markdown, both"
        echo "  regression [native|wasm] - Run regression tests (default: native)"
        echo "  serve      - Start demo server at http://localhost:8080"
        echo "  clean      - Clean build artifacts"
        echo "  help       - Show this help"
        echo ""
        echo "Quick start:"
        echo "  ./dev.sh setup    # First time setup"
        echo "  ./dev.sh build    # Build and test"
        echo "  ./dev.sh wasm     # Build for web"
        echo "  ./dev.sh serve    # Start web demo"
        echo ""
        echo "Debugging:"
        echo "  ./dev.sh debug example.mda          # Start interactive debugger"
        echo "  ./dev.sh debug example.mda --break 5 # Start with breakpoint at line 5"
        echo ""
        echo "Run examples:"
        echo "  ./dev.sh run test.mda          # Evaluate test.mda"
        echo "  ./dev.sh run test.mda markdown # Create test.md"
        echo "  ./dev.sh run test.mda both     # Both evaluate and create markdown"
        ;;
esac