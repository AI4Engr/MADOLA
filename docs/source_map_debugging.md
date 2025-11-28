# MADOLA Debugging Guide

This document describes the comprehensive debugging functionality in MADOLA, including source maps, interactive debugging, and step-by-step execution for .mda files.

## Overview

MADOLA provides professional-grade debugging capabilities for .mda files:

- **Interactive Step Debugging**: GDB-like step into, step over, step out commands
- **Breakpoint System**: Line, conditional, function, and variable breakpoints
- **Variable Inspection**: Real-time variable values and scope inspection
- **Precise Error Locations**: Errors show exact line and column numbers in source .mda files
- **Source Map Generation**: Standard JSON source maps for IDE integration
- **Stack Trace Support**: Full call stack with source positions
- **Debug CLI Tools**: Enhanced command-line debugging capabilities

## Interactive Debugging

### Starting the Interactive Debugger

```bash
# Start interactive debugging session
./dist/madola-debug example.mda

# Start with initial breakpoint at line 5
./dist/madola-debug example.mda --break 5
```

### Step Control Commands

#### Step Into (`step`, `s`)
Steps into function calls and expressions, allowing you to debug inside functions:

```bash
(madola-db) step
Stepping into...
>>> 7 | r := (-\beta + sqrt(\beta^2 - 4 * \alpha * \gamma)) / (2 * \alpha);
          ^
          | Now stepping into sqrt() function
```

**Use Case**: When you want to debug the implementation of function calls or complex expressions.

#### Step Over (`next`, `n`)
Steps over function calls, executing them but not stepping into their implementation:

```bash
(madola-db) next
Stepping over...
>>> 8 | print(r);
```

**Use Case**: When you want to execute a function but don't need to debug its internals.

#### Step Out (`finish`, `f`)
Steps out of the current function, continuing execution until it returns to the calling function:

```bash
(madola-db) finish
Stepping out...
>>> 9 | // Back in main function after sqrt() completed
```

**Use Case**: When you've finished debugging inside a function and want to return to the caller.

### Breakpoint Management

#### Line Breakpoints
Set breakpoints at specific line numbers:

```bash
# Basic line breakpoint
(madola-db) break 7
Breakpoint 1 set at example.mda:7:1

# Conditional breakpoint - only breaks when condition is true
(madola-db) break 10 if alpha > 0
Breakpoint 2 set at example.mda:10:1
```

#### Function Breakpoints
Break when entering specific functions:

```bash
(madola-db) break sqrt
Function breakpoint 3 set for function 'sqrt'
```

#### Variable Breakpoints
Break when a variable value changes:

```bash
(madola-db) break alpha
Variable breakpoint 4 set for variable 'alpha'
```

#### Managing Breakpoints

```bash
# List all breakpoints with their status
(madola-db) info breakpoints
=== Breakpoints ===
  1: example.mda:7 [enabled] (hit 0 times)
  2: example.mda:10 if alpha > 0 [enabled] (hit 0 times)
  3: function sqrt [enabled] (hit 0 times)

# Remove specific breakpoint
(madola-db) delete 1
Breakpoint 1 removed

# Disable/enable breakpoint
(madola-db) disable 2
(madola-db) enable 2
```

### Variable Inspection

#### Print Variable Values
```bash
# Print single variable
(madola-db) print alpha
alpha = 1

# Print expression
(madola-db) print alpha + beta
alpha + beta = -3
```

#### Show All Local Variables
```bash
(madola-db) locals
=== Local Variables ===
  alpha = 1
  beta = -4
  gamma = 4
  r = <undefined>
```

### Execution Control

```bash
# Continue normal execution until next breakpoint
(madola-db) continue
Continuing execution...

# Pause execution (also Ctrl+C)
(madola-db) pause
Execution paused

# Quit debugging session
(madola-db) quit
```

### Call Stack and Context

#### View Call Stack
```bash
(madola-db) backtrace
=== Call Stack ===
  #0: main() (example.mda:7:5)
  #1: quadratic_formula() (math.mda:15:12)
  #2: sqrt() (math.mda:45:8)
```

#### Show Current Location with Context
```bash
(madola-db) list
=== Current Location ===
example.mda:7:25

    5 | \alpha := 1;
    6 | \beta := -4;
>>> 7 | r := (-\beta + sqrt(\beta^2 - 4 * \alpha * \gamma)) / (2 * \alpha);
    8 | print(r);
    9 |
```

## Complete Interactive Session Example

```bash
$ ./dist/madola-debug example.mda

=== MADOLA Interactive Debugger ===
Type 'help' for commands, 'continue' to start execution

=== Current Location ===
example.mda:1:1

>>> 1 | @layout3x1
    2 | \alpha := 1;
    3 | \beta := -4;

(madola-db) break 7
Breakpoint 1 set at example.mda:7:1

(madola-db) break sqrt
Function breakpoint 2 set for function 'sqrt'

(madola-db) continue
Continuing execution...

Breakpoint 1 hit at example.mda:7:1

=== Current Location ===
example.mda:7:1

    5 | \alpha := 1;
    6 | \beta := -4;
>>> 7 | r := (-\beta + sqrt(\beta^2 - 4 * \alpha * \gamma)) / (2 * \alpha);
    8 | print(r);

(madola-db) locals
=== Local Variables ===
  alpha = 1
  beta = -4
  gamma = 4

(madola-db) step
Stepping into...

Breakpoint 2 hit at math.mda:45:1  # Function breakpoint triggered
=== Current Location ===
math.mda:45:1

>>> 45 | fn sqrt(x) {
     46 |     return x^0.5;
     47 | }

(madola-db) print x
x = 0

(madola-db) step
Stepping into...
>>> 46 |     return x^0.5;

(madola-db) finish
Stepping out...
>>> 7 | r := (-\beta + sqrt(\beta^2 - 4 * \alpha * \gamma)) / (2 * \alpha);
         ^                  ^
         |                  | sqrt() returned 0

(madola-db) print r
r = 2

(madola-db) next
Stepping over...
>>> 8 | print(r);

(madola-db) continue
Continuing execution...
2
Program execution completed.
```

## Architecture

### Core Components

1. **InteractiveDebugger** (`src/core/debug/interactive_debugger.h`)
   - Main debugger class with step control and breakpoint management
   - GDB-like command interface for interactive debugging
   - Support for step into, step over, step out operations

2. **SourceLocation** (`src/core/ast/ast.h`)
   - Tracks line, column, and byte offset positions
   - Used throughout AST nodes for position information

3. **SourceMapGenerator** (`src/core/debug/source_map.h`)
   - Generates standard JSON source maps
   - Maps generated output back to original .mda source
   - VLQ encoding support for compact mappings

4. **Enhanced AST Nodes** (`src/core/ast/ast.h`)
   - All AST nodes now include source position information
   - Constructors accept SourceLocation parameters

5. **DebuggingEvaluator** (`src/core/debug/interactive_debugger.h`)
   - Debug-aware evaluator that integrates with the interactive debugger
   - Hooks for step control and variable inspection

## Debugging Command Reference

| Command | Shortcut | Description |
|---------|----------|-------------|
| `help` | `h` | Show command help |
| `continue` | `c` | Continue execution until next breakpoint |
| `step` | `s` | **Step into** - Enter function calls and expressions |
| `next` | `n` | **Step over** - Execute but don't enter function calls |
| `finish` | `f` | **Step out** - Exit current function to caller |
| `break <line>` | | Set line breakpoint |
| `break <line> if <cond>` | | Set conditional breakpoint |
| `break <function>` | | Set function breakpoint |
| `delete <id>` | | Remove breakpoint by ID |
| `disable <id>` | | Disable breakpoint |
| `enable <id>` | | Enable breakpoint |
| `info breakpoints` | | Show all breakpoints with status |
| `print <var>` | | Show variable value or evaluate expression |
| `locals` | | Show all local variables in current scope |
| `backtrace` | `bt` | Show call stack with function names |
| `list` | `l` | Show current location with source context |
| `pause` | | Pause execution (also Ctrl+C) |
| `quit` | `q` | Exit debugger |

### Step Debugging Detailed Behavior

#### Step Into (`step`, `s`)
- **Function Calls**: Steps into the function body
- **Expressions**: Steps through sub-expressions
- **Complex Statements**: Breaks down into individual operations
- **Use When**: You want to debug the internals of function calls

```bash
# Before step into
>>> 7 | result := calculate(alpha, beta);

# After step into - now inside calculate() function
>>> 15 | fn calculate(a, b) {
     16 |     return a + b * 2;
```

#### Step Over (`next`, `n`)
- **Function Calls**: Executes the entire function without stepping into it
- **Statements**: Moves to the next statement at the same scope level
- **Use When**: You trust the function works and don't need to debug it

```bash
# Before step over
>>> 7 | result := calculate(alpha, beta);

# After step over - function executed, moved to next line
>>> 8 | print(result);
```

#### Step Out (`finish`, `f`)
- **Functions**: Continues execution until the current function returns
- **Scope**: Returns to the calling function
- **Use When**: You're done debugging inside a function and want to return to the caller

```bash
# Inside function
>>> 16 |     return a + b * 2;

# After step out - back to caller
>>> 8 | print(result);  // calculate() returned its value
```

## Non-Interactive Debugging

### Basic Source Map Debugging

```bash
# Enable debug mode
./dist/madola.exe example.mda --debug

# Generate source map
./dist/madola.exe example.mda --source-map

# Save source map to file
./dist/madola.exe example.mda --source-map --source-map-output example.mda.map
```

### Command Line Options

- `--debug`: Enable debug output showing AST structure and evaluation steps
- `--source-map`: Generate source map for the compilation
- `--source-map-output <file>`: Write source map to specified file
- `--help, -h`: Show help with all debugging options
- `--version, -v`: Show version information

### Error Reporting Examples

**Before (without source maps):**
```
Error: Undefined variable 'x'
```

**After (with source maps):**
```
Error in example.mda:7:5
Undefined variable 'x'

   5 | \alpha := 1;
   6 | \beta := -4;
   7 | x := \alpha + \beta;
     |     ^
   8 | print(x);
```

## Programming Interface

### Using SourceMapGenerator

```cpp
#include "src/core/debug/source_map.h"

// Create source map generator
auto source_map = std::make_shared<SourceMapGenerator>("example.mda");

// Add mappings during compilation
source_map->addMapping(generated_line, generated_col,
                      original_line, original_col, "variable_name");

// Or add from AST node
source_map->addMapping(generated_line, generated_col, ast_node, "symbol");

// Generate JSON source map
auto json_map = source_map->generateSourceMap("output.md");
```

### Enhanced Error Handling

```cpp
#include "src/core/debug/error_reporting.h"

try {
    // MADOLA evaluation code
} catch (const MadolaRuntimeError& e) {
    std::cout << e.formatError(debug_info) << std::endl;
}
```

### Stack Trace Support

```cpp
#include "src/core/debug/error_reporting.h"

StackTrace stack_trace;

// In function calls, add context
MADOLA_STACK_TRACE(stack_trace, source_location, "function_name", "description");

// Automatic cleanup when scope exits
```

## Source Map Format

MADOLA generates standard JSON source maps compatible with most debugging tools:

```json
{
  "version": 3,
  "file": "output.md",
  "sourceRoot": "",
  "sources": ["example.mda"],
  "sourcesContent": ["@layout3x1\\n\\alpha := 1;\\n..."],
  "names": ["alpha", "beta", "gamma"],
  "mappings": "AAAA,SAAS,CAAC,EAAE..."
}
```

### Mapping Fields

- `version`: Source map format version (always 3)
- `file`: Generated output file name
- `sources`: Array of original source files
- `sourcesContent`: Original source code content
- `names`: Symbol names for debugging
- `mappings`: VLQ-encoded position mappings

## IDE Integration

### VS Code Integration

1. Generate source map:
   ```bash
   ./dist/madola.exe example.mda --source-map-output example.mda.map
   ```

2. Configure VS Code to use source maps in `launch.json`:
   ```json
   {
     "type": "node",
     "request": "launch",
     "program": "madola.exe",
     "args": ["example.mda"],
     "sourceMaps": true
   }
   ```

### Other IDEs

Most modern IDEs and debuggers support source maps. Configure your debugger to:

1. Load the generated `.mda.map` file
2. Enable source map support
3. Set breakpoints in original .mda files

## Advanced Features

### Custom Debug Information

```cpp
// Create debug info with source content
DebugInfo debug_info("example.mda", source_content);

// Format errors with context lines
std::string error_msg = debug_info.formatError(
    "Runtime error message",
    SourceLocation(line, col),
    3  // context lines
);
```

### Source Map Validation

```cpp
// Validate generated source map
bool is_valid = DebugUtils::validateSourceMap(
    source_map,
    original_source,
    generated_output
);
```

### Debug Reports

```cpp
// Generate comprehensive debug report
std::string report = DebugUtils::createDebugReport(
    program_ast,
    source_map,
    source_content
);
```

## Build Integration

### CMake Integration

The debug features are integrated into the existing CMake build system:

```cmake
# Debug build with source maps
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_DEBUG_FEATURES=ON ..
make
```

### Dependencies

The source map functionality requires:

- **nlohmann/json**: For JSON source map generation
- **Tree-sitter**: For precise position tracking (already integrated)

## Performance Considerations

### Source Map Generation

- Source maps add ~10-15% compilation overhead
- Memory usage increases by ~5-10% for position tracking
- Generated source map files are typically 2-3x source file size

### Debug Mode

- Debug mode adds detailed logging and validation
- Recommended for development only
- Production builds should disable debug features

## Best Practices

### Interactive Debugging Workflow

1. **Start with Line Breakpoints**: Set breakpoints at key locations before running
   ```bash
   (madola-db) break 10    # Set breakpoint at suspicious line
   (madola-db) continue    # Run until breakpoint
   ```

2. **Use Step Into for Function Debugging**: When you hit a function call, step into it to debug internals
   ```bash
   (madola-db) step        # Enter the function
   (madola-db) locals      # Check local variables
   ```

3. **Use Step Over for Trusted Code**: Skip over functions you know work correctly
   ```bash
   (madola-db) next        # Execute function without stepping into it
   ```

4. **Use Step Out to Exit Functions**: When you're done debugging inside a function
   ```bash
   (madola-db) finish      # Return to the calling function
   ```

5. **Combine Breakpoint Types**: Use different breakpoint types for comprehensive debugging
   ```bash
   (madola-db) break 15              # Line breakpoint
   (madola-db) break myFunc          # Function breakpoint
   (madola-db) break x               # Variable breakpoint
   (madola-db) break 20 if x > 5     # Conditional breakpoint
   ```

### Debugging Complex Mathematical Expressions

For complex mathematical expressions like:
```
r := (-\beta + sqrt(\beta^2 - 4 * \alpha * \gamma)) / (2 * \alpha);
```

**Debugging Strategy:**
1. Set breakpoint at the line
2. Step into to see each sub-expression evaluation
3. Print intermediate values
4. Verify mathematical correctness

```bash
(madola-db) break 7
(madola-db) continue
(madola-db) print beta^2 - 4 * alpha * gamma    # Check discriminant
(madola-db) step                                  # Step into sqrt()
(madola-db) print x                              # Check sqrt argument
(madola-db) finish                               # Return from sqrt
(madola-db) print r                              # Check final result
```

### During Development

1. **Always enable interactive debugging** for development builds
2. **Set function breakpoints** for key mathematical functions
3. **Use variable breakpoints** to track state changes
4. **Generate source maps** for IDE integration

### Error Handling

```cpp
// Good: Provide source location context
throw MadolaRuntimeError("Variable not found",
                        node.getStartPosition(),
                        source_file);

// Bad: Generic error without context
throw std::runtime_error("Variable not found");
```

### Source Map Organization

```
project/
├── src/
│   └── example.mda
├── build/
│   └── example.mda.map    # Source map
└── dist/
    └── example.md         # Generated output
```

## Troubleshooting

### Common Issues

**Source map not found:**
- Ensure source map is generated with `--source-map-output`
- Check file permissions and paths

**Incorrect line numbers:**
- Verify Tree-sitter positions are captured correctly
- Check for off-by-one errors in position mapping

**Performance issues:**
- Disable debug mode in production
- Consider selective source map generation for large files

### Debug Output

Enable verbose debugging with:
```bash
./dist/madola.exe example.mda --debug --source-map 2>&1 | tee debug.log
```

## Examples

### Basic Example

File: `example.mda`
```
@layout3x1
\alpha := 1;
\beta := -4;
\gamma := 4;

r := (-\beta + sqrt(\beta^2 - 4 * \alpha * \gamma)) / (2 * \alpha);
print(r);
```

Debug command:
```bash
./dist/madola.exe example.mda --debug --source-map-output example.mda.map
```

Output with source map:
```
=== DEBUG: AST Structure ===
Program [1:1-7:10]
  LayoutDirective [1:1-1:10]
  AssignmentStatement [2:1-2:13]
  AssignmentStatement [3:1-3:14]
  AssignmentStatement [4:1-4:16]
  AssignmentStatement [6:1-6:64]
  PrintStatement [7:1-7:9]

=== SOURCE MAP ===
{
  "version": 3,
  "sources": ["example.mda"],
  "mappings": "AAAA;AACA,QAAQ;AACR,QAAQ;AACR,QAAQ;AACA;AACR,MAAM"
}
```

This comprehensive source map debugging system enables precise debugging of MADOLA files with full IDE integration and enhanced error reporting.