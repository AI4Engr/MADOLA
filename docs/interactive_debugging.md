# MADOLA Interactive Debugging Guide

## Step Debugging Commands

MADOLA now supports full interactive debugging with GDB-like commands for stepping through .mda code execution.

### Starting the Debugger

```bash
# Start interactive debugging
./dist/madola-debug example.mda

# Start with initial breakpoint
./dist/madola-debug example.mda --break 5
```

### Step Control Commands

#### Step Into (`step`, `s`)
Steps into function calls and expressions:
```
(madola-db) step
Stepping into...
>>> 7 | r := (-\beta + sqrt(\beta^2 - 4 * \alpha * \gamma)) / (2 * \alpha);
```

#### Step Over (`next`, `n`)
Steps over function calls, staying at the same level:
```
(madola-db) next
Stepping over...
>>> 8 | print(r);
```

#### Step Out (`finish`, `f`)
Steps out of the current function to the calling level:
```
(madola-db) finish
Stepping out...
>>> 9 | // Back to caller
```

### Breakpoint Commands

#### Set Line Breakpoint
```bash
(madola-db) break 5
Breakpoint 1 set at example.mda:5:1

# Conditional breakpoint
(madola-db) break 7 if alpha > 0
Breakpoint 2 set at example.mda:7:1
```

#### Function Breakpoints
```bash
(madola-db) break sqrt
Function breakpoint 3 set for function 'sqrt'
```

#### Variable Breakpoints
```bash
(madola-db) break alpha
Variable breakpoint 4 set for variable 'alpha'
```

#### Manage Breakpoints
```bash
# List all breakpoints
(madola-db) info breakpoints

# Remove breakpoint
(madola-db) delete 1

# Clear all breakpoints
(madola-db) clear
```

### Variable Inspection

#### Print Variable Value
```bash
(madola-db) print alpha
alpha = 1

(madola-db) print beta
beta = -4
```

#### Show All Variables
```bash
(madola-db) locals
=== Local Variables ===
  alpha = 1
  beta = -4
  gamma = 4
  r = <undefined>
```

### Execution Control

#### Continue Execution
```bash
(madola-db) continue
Continuing execution...
```

#### Pause Execution
```bash
(madola-db) pause  # (Ctrl+C also works)
Execution paused
```

### Call Stack & Context

#### Show Call Stack
```bash
(madola-db) backtrace
=== Call Stack ===
  #0: main() (example.mda:7:5)
  #1: sqrt() (math.mda:45:12)
```

#### Show Current Location
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

## Full Session Example

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

>>> 7 | r := (-\beta + sqrt(\beta^2 - 4 * \alpha * \gamma)) / (2 * \alpha);
          ^
          | Evaluating sqrt expression

(madola-db) print beta^2 - 4 * alpha * gamma
beta^2 - 4 * alpha * gamma = 0

(madola-db) next
Stepping over...

>>> 8 | print(r);

(madola-db) print r
r = 2

(madola-db) continue
Continuing execution...
2
Program execution completed.
```

## Advanced Features

### Conditional Breakpoints

```bash
# Break only when variable meets condition
(madola-db) break 10 if x > 5

# Break on specific iteration
(madola-db) break 15 if i == 3
```

### Expression Evaluation

```bash
# Evaluate expressions in current context
(madola-db) print alpha + beta
alpha + beta = -3

# Complex expressions
(madola-db) print sqrt(beta^2 - 4*alpha*gamma)
sqrt(beta^2 - 4*alpha*gamma) = 0
```

### Debugger Integration

The interactive debugger integrates with:

- **Source Maps**: Accurate position mapping
- **AST Nodes**: Step through language constructs
- **Variable Scope**: Proper variable visibility
- **Call Stack**: Function call tracking

### Programming Interface

```cpp
#include "src/core/debug/interactive_debugger.h"

// Create debugger
madola::debug::InteractiveDebugger debugger("file.mda", source);

// Set breakpoints programmatically
debugger.addBreakpoint(madola::SourceLocation(5, 1));
debugger.addFunctionBreakpoint("myFunction");

// Control execution
debugger.start(program);
debugger.stepInto();
debugger.stepOver();
debugger.stepOut();

// Inspect state
auto context = debugger.getCurrentContext();
auto value = debugger.getVariableValue("alpha");
```

### Event Callbacks

```cpp
// Set breakpoint callback
debugger.setOnBreakpoint([](const auto& bp) {
    std::cout << "Hit breakpoint " << bp.id << std::endl;
});

// Set step callback
debugger.setOnStep([](const auto& ctx) {
    std::cout << "Stepped to " << ctx.current_location.line << std::endl;
});

// Set variable change callback
debugger.setOnVariableChange([](const std::string& name, const std::string& value) {
    std::cout << name << " changed to " << value << std::endl;
});
```

## Command Reference

| Command | Shortcut | Description |
|---------|----------|-------------|
| `help` | `h` | Show command help |
| `continue` | `c` | Continue execution |
| `step` | `s` | Step into (enter functions) |
| `next` | `n` | Step over (skip function calls) |
| `finish` | `f` | Step out (exit current function) |
| `break <line>` | | Set line breakpoint |
| `break <line> if <cond>` | | Set conditional breakpoint |
| `delete <id>` | | Remove breakpoint |
| `info breakpoints` | | Show all breakpoints |
| `print <var>` | | Show variable value |
| `locals` | | Show all variables |
| `backtrace` | `bt` | Show call stack |
| `list` | `l` | Show current location |
| `quit` | `q` | Exit debugger |

The MADOLA interactive debugger provides professional-grade debugging capabilities comparable to GDB, making it easy to debug complex .mda mathematical programs with precise control and inspection.