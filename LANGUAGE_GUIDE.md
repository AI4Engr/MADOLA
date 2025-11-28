# MADOLA Language Guide

MADOLA (Math Domain Language) is a specialized language for mathematical computations with support for complex numbers, matrices, units, and symbolic math. This guide covers all language features with practical examples.

## Table of Contents

1. [Feature Overview](#feature-overview)
2. [Basic Syntax](#basic-syntax)
3. [Data Types](#data-types)
4. [Operators](#operators)
5. [Variables and Assignment](#variables-and-assignment)
6. [Control Flow](#control-flow)
7. [Functions](#functions)
8. [Built-in Functions](#built-in-functions)
9. [Arrays and Matrices](#arrays-and-matrices)
10. [Units](#units)
11. [Comments](#comments)
12. [Document Structure](#document-structure)
13. [Decorators](#decorators)
14. [Importing](#importing)
15. [Visualization and Graphing](#visualization-and-graphing)

---

## Feature Overview

MADOLA supports:

- **Variables & Assignment**: `x := 42;` with inline comments
- **Inline Comments**: `x := 42 -|comment after;` or `x := 42 |-comment before;` (controls comment position in output)
- **Array Assignment**: `x[i] := value;` with dynamic array creation
- **Column Vector Assignment**: `v[i;] := value;` auto-creates column vectors in loops
- **Mathematical Expressions**: `result := (a + b) * c;`
- **Complex Numbers**: `z := 1+2i;` supports addition, subtraction, multiplication, and division
- **Matrix Methods**:
  - `A.det()` - Matrix determinant (displays as |A|)
  - `A.inv()` - Matrix inverse (displays as A⁻¹)
  - `A.tr()` - Matrix trace
  - `A.T()` - Matrix transpose (displays as Aᵀ)
  - `A.eigenvalues()` - Matrix eigenvalues (returns vector)
  - `A.eigenvectors()` - Matrix eigenvectors (returns matrix with eigenvectors as columns)
- **Function Calls**: `pi := calcPi(1000);`
- **Mathematical Functions**: `math.sqrt()`, `math.abs()`, `math.sin()`, `math.cos()`, `math.tan()` - standard mathematical and trigonometric functions
- **Summation**: `math.summation(expression, variable, lower, upper)` - symbolic summation with LaTeX output
- **Imports**: `from math_lib import square, cube;` - imports functions from `.mda` files or WASM modules (searches current directory and `~/.madola/trove/`)
- **Print Statements**: `print(result);`
- **For Loops**: `for i in 1...10 { ... }`
- **While Loops**: `while (condition) { ... }` with support for single statement form
- **Break Statement**: `break;` exits for-loops and while-loops immediately
- **Function Declarations**: `fn square(x) { return x * x; }`
- **Piecewise Functions**: `f(x) := piecewise { (expr1, cond1), (expr2, otherwise) }`
- **Type Inspection**: `type(variable)` returns `"number"`, `"array"`, `"unit"`, `"complex"`, etc.
- **Visualization**: `graph(x, y, "title")` for 2D graphs, `graph_3d(...)` for 3D visualizations
- **Data Tables**: `table(headers, col1, col2, ...)` for formatted data tables
- **Physical Units**: `5 mm`, `3.5 kg`, `25 * m/s` - values with physical units

### Decorators

- **Document Structure**:
  - `@version 0.01` - version metadata (not shown in HTML reports)
  - `@h1[center] {text}`, `@h2 {text}`, `@h3 {text}`, `@h4 {text}` - HTML headings for reports
  - `@p {text}` - paragraphs with preserved whitespace and line breaks
- **Display Control**:
  - `@eval` - force evaluation and display of results
  - `@skip` - skips next line in output
- `@layout1x2`, `@layout1x3`, etc. - formats assignments in horizontal LaTeX array
  - `@resolveAlign` - show step-by-step equation solving with aligned equals signs
  - `@merge` - merge multiple statements into single output line
- **Code Generation**:
  - `@gen_cpp` - generates C++ files in `~/.madola/gen_cpp/`
  - `@gen_addon` - compiles functions to WASM and stores in `~/.madola/trove/`

### Comments

- **Single-line**: `// This is a comment`
- **Multi-line**: `/* This is a multi-line comment */`

---

## Basic Syntax

MADOLA uses a simple, mathematical syntax. Statements end with semicolons (`;`), and blocks use curly braces (`{}`).

```madola
// Simple variable assignment
\alpha_{u,1} := 42;

// Print output
print(\alpha_{u,1});
```

---

## Data Types

### Numbers

MADOLA supports integers and floating-point numbers:

```madola
integer := 42;
floating := 3.14159;
negative := -10;

print(integer);   // Output: 42
print(floating);  // Output: 3.14159
```

### Complex Numbers

Complex numbers use the `i` notation for the imaginary unit:

```madola
// Basic complex numbers
z1 := 1 + 2i;
z2 := 3 - 4i;
z3 := 5i;        // Pure imaginary
z4 := 7;         // Real number (0i)

print(z1);  // Output: 1 + 2i
print(z2);  // Output: 3 - 4i
```

#### Complex Arithmetic

```madola
a := 1 + 2i;
b := 3 - 4i;

// Addition
sum := a + b;
print(sum);  // Output: 4 - 2i

// Subtraction
diff := a - b;
print(diff);  // Output: -2 + 6i

// Multiplication
product := a * b;
print(product);  // Output: 11 - 2i
// Calculation: (1+2i)(3-4i) = 3 - 4i + 6i - 8i² = 3 + 2i + 8 = 11 + 2i

// Division
quotient := a / b;
print(quotient);  // Output: -0.2 + 0.4i
// Calculation: (1+2i)/(3-4i) = (1+2i)(3+4i)/25 = (-5+10i)/25
```


### Strings

Strings are enclosed in double quotes:

```madola
message := "Hello, MADOLA!";
print(message);
```

### Arrays and Vectors

```madola
// Row vector
row := [1, 2, 3];

// Column vector
col := [1; 2; 3];

// Access elements (0-indexed)
first := row[0];
print(first);  // Output: 1
```

### Matrices

```madola
// 2x3 matrix
A := [1, 2, 3;
      4, 5, 6];

// 3x3 identity-like matrix
I := [1, 0, 0;
      0, 1, 0;
      0, 0, 1];
```

---

## Operators

### Arithmetic Operators

```madola
a := 10;
b := 3;

sum := a + b;        // 13
difference := a - b; // 7
product := a * b;    // 30
quotient := a / b;   // 3.333...
power := a ^ b;      // 1000
```

### Comparison Operators

```madola
x := 5;
y := 10;

x < y;   // true (1)
x > y;   // false (0)
x <= 5;  // true (1)
x >= 10; // false (0)
x == 5;  // true (1)
x != y;  // true (1)
```

### Logical Operators

```madola
a := 1;  // true
b := 0;  // false

result1 := a && b;  // false (0) - AND
result2 := a || b;  // true (1)  - OR
result3 := !a;      // false (0) - NOT

// Combined with comparisons
x := 7;
inRange := (x > 5) && (x < 10);  // true (1)
```

---

## Variables and Assignment

### Basic Assignment

```madola
x := 42;
y := x + 10;
```

### Assignment with Inline Comments

```madola
// Comment before value
mass := 75 |-The mass of the object 75;

// Comment after value
speed := 25 -|meters per second;

print(mass);   // Output: 75
print(speed);  // Output: 25
```

### Array Element Assignment

```madola
arr := [1, 2, 3, 4, 5];
arr[2] := 99;
print(arr[2]);  // Output: 99
```

### Column Vector Assignment

Create column vectors dynamically in loops using the `[i;]` syntax:

```madola
// Auto-create column vector
for i in 0...4 {
    v[i;] := i * 2;
}
// Result: v = [0; 2; 4; 6; 8] (column vector)

// Compare with row vector
for i in 0...4 {
    r[i] := i * 2;
}
// Result: r = [0, 2, 4, 6, 8] (row vector)
```

**Use cases:**
- Matrix column construction
- Vector operations requiring column format
- Linear algebra computations

---

## Control Flow

### If/Else Statements

```madola
fn checkSign(x) {
    if (x > 0) {
        return 1;
    } else if (x < 0) {
        return -1;
    } else {
        return 0;
    }
}

result := checkSign(-5);
print(result);  // Output: -1
```

### For Loops

```madola
// Range syntax: start...end (inclusive)
sum := 0;
for i in 1...10 {
    sum := sum + i;
}
print(sum);  // Output: 55

// Calculate pi using Leibniz formula
pi_a := 0.0;
for i in 0...1000 {
    pi_a := pi_a + ((-1)^i) / ((2*i)+1);
}
pi_a := pi_a * 4;
print(pi_a);  // Output: ~3.14159
```

### While Loops

```madola
fn fibonacci(n) {
    a := 0;
    b := 1;
    count := 0;

    while (count < n) {
        temp := a;
        a := b;
        b := temp + b;
        count := count + 1;
    }

    return a;
}

print(fibonacci(10));  // Output: 55
```

### Break Statement

```madola
fn findFirst(target) {
    i := 0;
    while (i < 100) {
        if (i == target) {
            break;
        }
        i := i + 1;
    }
    return i;
}

result := findFirst(42);
print(result);  // Output: 42
```

---

## Functions

### Function Declaration

```madola
fn square(x) {
    return x * x;
}

result := square(5);
print(result);  // Output: 25
```

### Multiple Parameters

```madola
fn pythagoras(a, b) {
    return math.sqrt(a^2 + b^2);
}

hypotenuse := pythagoras(3, 4);
print(hypotenuse);  // Output: 5
```

### Recursive Functions

```madola
fn factorial(n) {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

print(factorial(5));   // Output: 120
print(factorial(10));  // Output: 3628800
```

### Piecewise Functions

Piecewise functions define different expressions for different conditions:

```madola
// Absolute value function
abs_val(x) := piecewise {
    (x, x >= 0),
    (-x, otherwise)
};

print(abs_val(-5));  // Output: 5
print(abs_val(3));   // Output: 3
print(abs_val(0));   // Output: 0

// Step function
step_func(t) := piecewise {
    (0, t < 1),
    (1, t < 2),
    (2, otherwise)
};

print(step_func(0.5));  // Output: 0
print(step_func(1.5));  // Output: 1
print(step_func(3.0));  // Output: 2

// Complex piecewise
math_piece(x) := piecewise {
    (x^2, x < 0),
    (x + 1, x <= 2),
    (2*x - 1, otherwise)
};

print(math_piece(-2));  // Output: 4
print(math_piece(1));   // Output: 2
print(math_piece(3));   // Output: 5
```

---

## Built-in Functions

### Math Namespace Functions

#### Square Root

```madola
result := math.sqrt(16);
print(result);  // Output: 4

// Error checking
// math.sqrt(-1);  // Error: Cannot take square root of negative number
```

#### Trigonometric Functions

```madola
pi := 3.14159265358979323846;

// Sine
sin_0 := math.sin(0);
print(sin_0);  // Output: 0

sin_pi_2 := math.sin(pi/2);
print(sin_pi_2);  // Output: 1

// Cosine
cos_0 := math.cos(0);
print(cos_0);  // Output: 1

cos_pi := math.cos(pi);
print(cos_pi);  // Output: -1

// Tangent
tan_0 := math.tan(0);
print(tan_0);  // Output: 0

tan_pi_4 := math.tan(pi/4);
print(tan_pi_4);  // Output: 1

// Trigonometric identity: sin²(x) + cos²(x) = 1
x := pi/4;
identity := math.sin(x)^2 + math.cos(x)^2;
print(identity);  // Output: 1
```

#### Summation

```madola
// Basic summation: Σ(i) from i=1 to 10
result := math.summation(i, i, 1, 10);
print(result);  // Output: 55

// Sum of squares: Σ(i²) from i=1 to 5
squares := math.summation(i^2, i, 1, 5);
print(squares);  // Output: 55

// Sum with coefficient: Σ(2*i) from i=1 to 4
doubled := math.summation(2*i, i, 1, 4);
print(doubled);  // Output: 20

// Harmonic series partial sum: Σ(1/i) from i=1 to 4
harmonic := math.summation(1/i, i, 1, 4);
print(harmonic);  // Output: 2.083333...

// Variable bounds
n := 6;
sum_n := math.summation(i, i, 1, n);
print(sum_n);  // Output: 21
```

### Utility Functions

#### Type Checking

```madola
x := 42;
arr := [1, 2, 3];

t1 := type(x);
print(t1);  // Output: "number"

t2 := type(arr);
print(t2);  // Output: "array"
```

#### Time

```madola
start := time();
// ... perform computations ...
end := time();
elapsed := end - start;
print(elapsed);  // Output: elapsed milliseconds
```

#### Table

The `table()` function creates formatted tables for displaying data:

```madola
// Simple table with headers and data
table(["Name", "Age", "Score"],
       ["Alice"; "Bob"; "Charlie"],
       [25; 30; 28],
       [95.5; 87.2; 92.8]);
// Output: Table 1 created with 3 columns and 3 rows

```

**Syntax:**
```madola
table(headers_array, column1, column2, ...);
```

**Parameters:**
- `headers_array`: Row vector of strings for column headers (use commas)
- `column1, column2, ...`: Column vectors for each column (use semicolons)

**Requirements:**
- Number of headers must match number of columns
- All data columns must be arrays
- Headers should be strings (can contain LaTeX notation)

**Examples:**
```madola
// Using numeric variables (row or column vectors both work)
x_values := [1, 2, 3, 4, 5];
y_values := [1, 4, 9, 16, 25];
table(["x", "x²"], x_values, y_values);

// Using inline column vectors (semicolons)
table(["x", "x²"],
      [1; 2; 3; 4; 5],
      [1; 4; 9; 16; 25]);

// Mixed data with string columns (must use inline arrays)
table(["Location", "Distance", "Category"],
      ["Point A"; "Point B"; "Point C"],
      [3.2; 7.8; 1.5],
      ["Near"; "Far"; "Near"]);
```

**Note:** String arrays must be used inline in the `table()` function call. String array variables are not currently supported.

---

## Arrays and Matrices

### Vector Operations

```madola
// Row and column vectors
u := [1, 0, 2];      // Row vector
v := [1; 2; 1];      // Column vector

// Accessing elements
first := u[0];
print(first);  // Output: 1
```

### Matrix Operations

```madola
// Define a 3x3 matrix
A := [2, 1, 0;
      1, 3, 1;
      0, 1, 2];

// Matrix-vector multiplication
v := [1; 2; 1];
Av := A * v;
print(Av);  // Column vector result

// Vector-matrix multiplication
u := [1, 0, 2];
uA := u * A;
print(uA);  // Row vector result

// Quadratic form: u * A * v
quad := u * A * v;
print(quad);  // Scalar result
```

### Building Arrays Programmatically

```madola
// Using a loop to fill an array
arr := [0, 0, 0, 0, 0];
for i in 0...4 {
    arr[i] := i * i;
}
// arr is now [0, 1, 4, 9, 16]
```

---

## Units

MADOLA supports values with physical units:

```madola
// Define values with units
length := 5 mm;
mass := 3.5 kg;
area := 10 m^2;
force := 100 N;
pressure := 50 kPa;

// Compound units using expressions
velocity := 25 * m/s;
acceleration := 9.8 * m/s^2;
density := 1000 * kg/m^3;

print(length);       // Output: 5 mm
print(velocity);     // Output: 25 m/s
print(acceleration); // Output: 9.8 m/s^2
```

**Supported unit syntax:**
- Simple units: `5 mm`, `3.5 kg`, `100 N`
- Units with exponents: `10 m^2`, `5 cm^3`
- Compound units: `25 * m/s`, `9.8 * m/s^2`, `1000 * kg/m^3`

**Note:** For compound units, use multiplication and division operators with unit identifiers (e.g., `25 * m/s` not `25 m/s`).

---

## Comments

### Single-line Comments

```madola
// This is a single-line comment
x := 42;  // Comment at end of line
```

### Multi-line Comments

```madola
/*
 * This is a multi-line comment
 * spanning multiple lines
 */
y := 100;
```

## Document Structure

MADOLA supports document structuring directives for creating formatted output.

### Version Declaration

```madola
@version 0.01
```

### Headings

```madola
@h1 {
Main Title}

@h1[center]{
Centered Title}

@h2{
Subsection}

@h3{
Sub-subsection}

@h4{
Minor heading}
```

### Multi-line Headings

```madola
@h1[right]{
This is title}

@h4{
Second line
Third line}
```

### Paragraphs

```madola
@p{
This is a regular paragraph.}

@p[center]{
This paragraph is centered.}

// Paragraphs with formatting
@p{
This is *italic* text and **bold** text.
This is ***bold-italic*** text for testing.}
```

### Block Syntax

```madola
@h1[center]{
This is a title in block syntax}

@h4{
Line 1
Line 2}

@p{
Paragraph content with *emphasis*
and **strong emphasis**.}
```

---

## Decorators

Decorators modify how statements are processed or displayed.

### @eval Decorator

Forces evaluation and display of results:

```madola
@eval
x := 5 * 10;
// Displays: x := 50

@eval
result := math.sqrt(144);
// Displays: result := 12
```

### @skip Decorator

Skips rendering of the next statement:

```madola
@skip
intermediate := 999;  // Won't appear in output

final := 42;
print(final);  // Will appear
```

### @layout Decorator

Formats multiple statements in a single row (horizontal layout). Useful for displaying related equations side-by-side.

```madola
// Display two equations on the same line
@layout1x2
x := 10;
y := 20;
// Output: x = 10    y = 20

// Display two column vectors side-by-side
@layout1x2
v_1 := [1; 2; 3];
v_2 := [4; 5; 6];
// Output: v₁ = [1; 2; 3]    v₂ = [4; 5; 6]

// Display three equations on the same line
@layout1x3
a := 1;
b := 2;
c := 3;
// Output: a = 1    b = 2    c = 3
```

**Syntax:** `@layoutRxC` where R is rows (usually 1) and C is columns (number of statements to group).

**Common patterns:**
- `@layout1x2` - Two statements side-by-side (scalars, vectors, or matrices)
- `@layout1x3` - Three statements side-by-side
- `@layout1x4` - Four statements side-by-side

**Note:** The decorator applies to the next C statements, grouping them horizontally in the output.

### @gen_cpp Decorator

Generates C++ source code for functions, saved to `~/.madola/gen_cpp/` directory.

```madola
@gen_cpp
fn square(x) {
    return x * x;
}
// Generates: square.cpp in ~/.madola/gen_cpp/
```

**Use cases:**
- Export mathematical functions to C++
- Integration with existing C++ projects
- Performance-critical code generation

### @gen_addon Decorator

Compiles functions to WASM modules and stores in `~/.madola/trove/` for reuse.

```madola
@gen_addon
fn factorial(n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
// Generates: factorial.wasm and factorial.js in ~/.madola/trove/
```

**Use cases:**
- Create reusable WASM function libraries
- High-performance web applications
- Cross-project function sharing

### @merge Decorator

Merges multiple statements or expressions into a single output line for compact display.

```madola
@merge
x := 10;
y := 20;
z := 30;
// Output: x = 10, y = 20, z = 30 (on one line)
```

**Use cases:**
- Compact variable initialization display
- Space-efficient output formatting
- Related assignments grouping

### @version Decorator

Specifies document version metadata (not displayed in HTML output).

```madola
@version 0.01
```

---

## Importing

### Import Specific Functions

```madola
// Import from another MADOLA file
from math_lib import square, cube, factorial;

// Use imported functions
x := square(5);      // 25
y := cube(3);        // 27
z := factorial(5);   // 120

print(x);
print(y);
print(z);
```

### Import with Aliases

```madola
from utilities import longFunctionName as short;

result := short(10);
print(result);
```

### Example math_lib.mda

```madola
// math_lib.mda - reusable math functions

fn square(x) {
    return x * x;
}

fn cube(x) {
    return x * x * x;
}

fn factorial(n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

fn pythagoras(a, b) {
    return math.sqrt(a^2 + b^2);
}
```

---

## Visualization and Graphing

MADOLA provides built-in functions for creating 2D graphs, 3D visualizations, and data tables.

### 2D Graphs with graph()

Create 2D line graphs from arrays of x and y values.

**Syntax:** `graph(x_array, y_array, [title])`

- `x_array` - Array of x-axis values
- `y_array` - Array of y-axis values
- `title` (optional) - String title for the graph

```madola
// Create data for a quadratic function
for i in 0...10 {
    x[i] := i;
    y[i] := i^2 + 2*i + 1;
}

// Create graph with title
graph(x, y, "Quadratic Function");

// Create graph without title (auto-generated)
graph(x, y);
```

**Example: Plotting Multiple Functions**

```madola
// Generate x values
for i in 0...20 {
    x[i] := i - 10;
    linear[i] := 2 * x[i] + 1;
    quadratic[i] := x[i]^2;
}

graph(x, linear, "Linear: y = 2x + 1");
graph(x, quadratic, "Quadratic: y = x²");
```

**Example: Trigonometric Functions**

```madola
// Plot sine and cosine
for i in 0...100 {
    x[i] := i * 0.1;
    sine[i] := math.sin(x[i]);
    cosine[i] := math.cos(x[i]);
}

graph(x, sine, "Sine Wave");
graph(x, cosine, "Cosine Wave");
```

**Complete 2D Graph Example: Quadratic Function**

```madola
// 2D Graph Example: Quadratic Function
// This demonstrates creating a simple 2D graph

// For loop creating data for graph
for i in 0...8 {
    x[i] := i;
    y[i] := i^2 + 2*i + 1;
}

graph(x, y, "Quadratic Function");
```

**Output:** The code above generates an interactive 2D line graph showing the quadratic function y = x² + 2x + 1. The graph automatically scales to fit the data and includes axis labels and data points.

### 3D Visualizations with graph_3d()

Create 3D geometric visualizations (e.g., brick with hole).

**Syntax:** `graph_3d(title, [width, height, depth, hole_width, hole_height, hole_depth])`

```madola
// Simple 3D brick with default dimensions
graph_3d("Simple Brick");

// Custom dimensions
graph_3d("Custom Brick", 5.0, 3.0, 4.0, 2.5, 1.5, 2.0);
```

**Complete 3D Graph Example: Engineering Brick**

```madola
// 3D Visualization Example: Brick with Hole
// This demonstrates creating a 3D geometric visualization

@h2{
Brick with Rectangular Hole}

@p{
This example creates a 3D visualization of a brick with a rectangular hole through it.
Useful for engineering calculations and volume computations.}

// Define brick dimensions
brick_w := 10;
brick_h := 6;
brick_d := 8;

// Define hole dimensions
hole_w := 4;
hole_h := 3;
hole_d := 8;

// Calculate volumes
brick_v := brick_w * brick_h * brick_d;
hole_v := hole_w * hole_h * hole_d;
net_v := brick_v - hole_v;

print("Brick Volume: " + brick_v + "\n");
print("Hole Volume: " + hole_v + "\n");
print("Net Volume: " + net_v + "\n");

// Create 3D visualization
graph_3d("Engineering Brick with Hole", 
         brick_w, brick_h, brick_d,
         hole_w, hole_h, hole_d);
```

**Output:** The code above generates:
1. Calculated volume values printed to the output
2. An interactive 3D visualization showing the brick with a rectangular hole
3. The 3D model can be rotated with mouse drag, zoomed with scroll wheel, and panned with right-click drag

### Data Tables with table()

Create formatted tables from arrays of data.

**Syntax:** `table(headers_array, column1, column2, ...)`

```madola
// Create a table with numeric data
names := ["Alice", "Bob", "Charlie"];
ages := [25, 30, 35];
scores := [95.5, 87.3, 92.8];

table(["Name", "Age", "Score"], names, ages, scores);
```

**Example: Computation Results Table**

```madola
// Calculate factorial values
for i in 1...6 {
    n[i-1] := i;
    fact[i-1] := factorial(i);
}

table(["n", "n!"], n, fact);
```

### Notes

- Arrays for `graph()` must have the same length
- Graph titles support Unicode characters and LaTeX-style symbols
- In HTML output, graphs are rendered interactively using D3.js
- Tables can contain numeric or string data

---

## Complete Examples

### Example 0: Complex Number Arithmetic

```madola
@h1{Complex Number Operations}

@p{
This example demonstrates comprehensive complex number arithmetic,
including addition, subtraction, multiplication, and division.}

// Define complex numbers
z1 := 1 + 2i;
z2 := 3 - 4i;

@h2{Basic Operations}

// Addition
sum := z1 + z2;
print("Sum: " + sum);

// Subtraction
diff := z1 - z2;
print("Difference: " + diff);

// Multiplication
product := z1 * z2;
print("Product: " + product);

// Division
quotient := z1 / z2;
print("Quotient: " + quotient);

@h2{Advanced Operations}

// Power
z_squared := z1^2;
print("z1²: " + z_squared);

// Magnitude
mag := math.abs(z1);
print("Magnitude of z1: " + mag);

@h2{Practical Application: AC Circuit}

// Calculate impedance in an AC circuit
R := 50;              // Resistance (Ω)
X_L := 30;            // Inductive reactance (Ω)
Z := R + X_L * i;     // Total impedance

V := 220;             // Voltage (V)
I := V / Z;           // Current

print("Impedance: " + Z);
print("Current: " + I);
print("Current magnitude: " + math.abs(I));
```

### Example 1: Calculating Pi

```madola
@version 0.01

@h1 {
Calculating Pi using Leibniz Formula}

@p {
The Leibniz formula states:
π/4 = 1 - 1/3 + 1/5 - 1/7 + 1/9 - ...}

sum := 0.0;
for i in 0...10000 {
    sum := sum + ((-1)^i) / ((2*i)+1);
}

pi_e := sum * 4;
print(pi_e);  // Output: ~3.14159
```

### Example 2: Greatest Common Divisor

```madola
@h2{
Euclidean Algorithm for GCD}

fn gcd(a, b) {
    while (b != 0) {
        temp := b;
        b := a % b;
        a := temp;
    }
    return a;
}

result := gcd(48, 18);
print(result);  // Output: 6
```

### Example 3: Matrix Computation

```madola
@h2{
Matrix-Vector Operations}

// Define a transformation matrix
T := [2, 0;
      0, 3];

// Define a vector
v := [1; 1];

// Apply transformation
result := T * v;
print(result);  // Output: [2; 3]
```

### Example 4: Numerical Integration (Simpson's Rule)

```madola
// Simpson's rule for numerical integration

// Define function to integrate
fn f(x){
    return x^2;
}

// Integration parameters
a := 0;
b := 1;
n := 100;

// Simpson's rule using math.summation
h := (b - a) / n;

// Simpson's formula: (h/3) * [f(a) + f(b) + 4*sum(odd) + 2*sum(even)]
area := (h / 3) * (f(a) + f(b) + 
        4 * math.summation(f(a + (2*k-1) * h), k, 1, n/2) +
        2 * math.summation(f(a + 2*k * h), k, 1, n/2 - 1));

print(area);  // Output: 0.333
```

### Example 5: Dichotomy Method (Binary Search)

```madola
@h2{Finding roots using dichotomy method}

// Define the function whose root we want to find
// f(x) = x² - 2, root should be √2 ≈ 1.414
fn f(x){
    return x^2 - 2;
}

// Dichotomy method implementation
a := 1;
b := 2;
tolerance := 0.0001;

while ((b - a) > tolerance){
    mid := (a + b) / 2;
    f_{mid} := f(mid);
    
    if (f_{mid} == 0){
        a := mid;
        b := mid;
    }else if (f(a) * f_{mid} < 0){
        b := mid;
    }else{
        a := mid;
    }
}

root := (a + b) / 2;
print(root);  // Output: ~1.414
```

---

## Best Practices

1. **Use descriptive variable names**: `velocity` instead of `v`
2. **Comment complex algorithms**: Help others understand your logic
3. **Break complex expressions into steps**: Improves readability
4. **Use functions for reusable code**: Avoid repetition
5. **Organize with headings and sections**: Structure your documents
6. **Test edge cases**: Especially for piecewise and recursive functions
7. **Use appropriate data types**: Arrays for collections, units for physical quantities
8. **Import shared functions**: Create reusable library files

---

## Error Handling

MADOLA provides clear error messages:

```madola
// Undefined variable
x := undefinedVar + 5;
// Error: Variable 'undefinedVar' not defined

// Type mismatch
arr := [1, 2, 3];
result := arr + "string";
// Error: Cannot add array to string

// Division by zero
x := 10 / 0;
// Error: Division by zero

// Square root of negative (real numbers)
y := math.sqrt(-4);
// Error: Cannot take square root of negative number

// Wrong number of arguments
fn add(a, b) { return a + b; }
result := add(5);
// Error: Function add expects 2 arguments, got 1
```

---

## Language Syntax Summary

### Keywords
- `fn` - function declaration
- `if`, `else` - conditional statements
- `for`, `in` - for loops
- `while` - while loops
- `return` - return from function
- `break` - exit loop early
- `piecewise`, `otherwise` - piecewise functions
- `from`, `import`, `as` - importing

### Operators
- Arithmetic: `+`, `-`, `*`, `/`, `^`
- Comparison: `<`, `>`, `<=`, `>=`, `==`, `!=`
- Logical: `&&`, `||`, `!`
- Assignment: `:=`

### Delimiters
- `;` - statement terminator
- `{}` - blocks
- `[]` - arrays/indexing
- `()` - grouping/function calls
- `,` - list separator
- `;` (in arrays) - row separator
- `...` - range operator

### Decorators
- `@version` - document version (metadata, not displayed)
- `@h1`, `@h2`, `@h3`, `@h4` - headings (with optional alignment: `[center]`, `[left]`, `[right]`)
- `@p` - paragraphs (preserves whitespace and line breaks)
- `@eval` - force evaluation display
- `@skip` - skip rendering next statement
- `@layout1xN` - format N statements horizontally (e.g., `@layout1x2`, `@layout1x3`)
- `@resolveAlign` - show step-by-step equation solving with aligned equals signs
- `@gen_cpp` - generate C++ source code (saves to `~/.madola/gen_cpp/`)
- `@gen_addon` - compile to WASM module (saves to `~/.madola/trove/`)
- `@merge` - merge multiple statements into single output line

---

## Conclusion

MADOLA combines mathematical notation with programming constructs, making it ideal for:
- Engineering calculations
- Scientific computing
- Mathematical documentation
- Educational materials
- Numerical analysis
- Matrix operations
- Unit conversions and dimensional analysis

For more examples, see the `regression/fixtures/` directory in the MADOLA repository.
