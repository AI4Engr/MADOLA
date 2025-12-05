module.exports = grammar({
  name: 'madola',

  conflicts: $ => [
    [$.parameter_list, $.primary_expression],
    [$.unit_expression]
  ],

  extras: $ => [
    /\s/
  ],

  rules: {
    source_file: $ => repeat($.statement),

    statement: $ => choice(
      $.assignment_statement,
      $.print_statement,
      $.expression_statement,
      $.comment_statement,
      $.decorated_function_declaration,
      $.function_declaration,
      $.piecewise_function_declaration,
      $.return_statement,
      $.for_statement,
      $.while_statement,
      $.import_statement,
      $.if_statement,
      $.decorated_statement,
      $.skip_statement,
      $.break_statement,
      $.heading_statement,
      $.version_statement,
      $.paragraph_statement
    ),

    decorated_statement: $ => seq(
      repeat1($.decorator),
      choice(
        $.assignment_statement,
        $.expression_statement,
        $.print_statement,
        $.comment_statement
      )
    ),

    skip_statement: $ => '@skip',

    break_statement: $ => seq('break', ';'),

    assignment_statement: $ => seq(
      choice(
        $.identifier,
        $.array_access
      ),
      choice(':=', '='),
      $.expression,
      optional(choice(
        seq('|-', $.before_comment),
        seq('-|', $.after_comment)
      )),
      ';'
    ),

    print_statement: $ => seq(
      'print',
      '(',
      $.expression,
      ')',
      ';'
    ),

    expression_statement: $ => seq(
      $.expression,
      ';'
    ),

    comment_statement: $ => choice(
      $.single_line_comment,
      $.multi_line_comment
    ),

    single_line_comment: $ => token(seq('//', /.*/)),

    multi_line_comment: $ => token(seq(
      '/*',
      repeat(choice(
        /[^*]/,
        seq('*', /[^\/]/)
      )),
      '*/'
    )),

    before_comment: $ => /[^;]+/,

    after_comment: $ => /[^;]+/,

    decorator: $ => choice(
      prec.right(seq('@', $.identifier, optional(seq('[', $.decorator_style, ']')))),
      seq('@', 'array', $.number, 'x', $.number)
    ),

    decorator_style: $ => /[a-zA-Z][a-zA-Z0-9_-]*/,

    heading_statement: $ => prec.right(seq(
      choice('@h1', '@h2', '@h3', '@h4'),
      optional(seq('[', $.heading_style, ']')),
      '{',
      $.heading_content,
      '}'
    )),

    heading_style: $ => /[a-zA-Z][a-zA-Z0-9_-]*/,

    heading_content: $ => token(prec(-1, /[^}]+/)),

    text_line: $ => token(prec(-1, /[^\r\n@/;:{}]+/)),

    version_statement: $ => seq('@version', $.version_number),

    version_number: $ => /[0-9]+(\.[0-9]+)*/,

    paragraph_statement: $ => seq(
      '@p',
      optional(seq('[', $.paragraph_style, ']')),
      '{',
      $.paragraph_content,
      '}'
    ),

    paragraph_style: $ => /[a-zA-Z][a-zA-Z0-9_-]*/,

    paragraph_content: $ => token(prec(-1, /[^}]+/)),

    decorated_function_declaration: $ => seq(
      repeat1($.decorator),
      $.function_declaration
    ),

    return_statement: $ => seq(
      'return',
      $.expression,
      ';'
    ),

    function_declaration: $ => seq(
      'fn',
      $.identifier,
      '(',
      optional($.parameter_list),
      ')',
      '{',
      repeat($.statement),
      '}'
    ),

    parameter_list: $ => seq(
      $.identifier,
      repeat(seq(',', $.identifier))
    ),

    for_statement: $ => seq(
      'for',
      $.identifier,
      'in',
      $.range_expression,
      '{',
      repeat($.statement),
      '}'
    ),

    while_statement: $ => seq(
      'while',
      '(',
      $.expression,
      ')',
      choice(
        seq('{', repeat($.statement), '}'),
        $.statement
      )
    ),

    if_statement: $ => prec.right(seq(
      'if',
      '(',
      $.expression,
      ')',
      choice(
        // Block form: { statements }
        seq('{', repeat($.statement), '}'),
        // Single statement form
        $.statement
      ),
      optional(seq(
        'else',
        choice(
          // Block form: { statements }
          seq('{', repeat($.statement), '}'),
          // Single statement form
          $.statement
        )
      ))
    )),

    range_expression: $ => prec.left(0, seq(
      $.primary_expression,
      '...',
      $.primary_expression
    )),

    expression: $ => $.pipe_expression,

    pipe_expression: $ => choice(
      $.logical_or_expression,
      prec.left(-3, seq($.logical_or_expression, '|', $.substitution_list))
    ),

    substitution_list: $ => prec.left(seq(
      $.substitution_pair,
      repeat(seq(',', $.substitution_pair))
    )),

    substitution_pair: $ => seq(
      $.identifier,
      ':',
      $.logical_or_expression
    ),

    logical_or_expression: $ => choice(
      $.logical_and_expression,
      prec.left(-2, seq($.logical_or_expression, '||', $.logical_and_expression))
    ),

    logical_and_expression: $ => choice(
      $.comparison_expression,
      prec.left(-1, seq($.logical_and_expression, '&&', $.comparison_expression))
    ),

    comparison_expression: $ => choice(
      $.additive_expression,
      prec.left(0, seq($.comparison_expression, choice('>', '<', '>=', '<=', '==', '!='), $.additive_expression))
    ),

    additive_expression: $ => choice(
      $.multiplicative_expression,
      prec.left(1, seq($.additive_expression, choice('+', '-'), $.multiplicative_expression))
    ),

    multiplicative_expression: $ => choice(
      $.power_expression,
      prec.left(2, seq($.multiplicative_expression, choice('*', '/', '%'), $.power_expression))
    ),

    power_expression: $ => choice(
      $.primary_expression,
      $.function_call,
      $.method_call,
      $.unary_expression,
      prec.right(3, seq($.primary_expression, '^', $.power_expression)),
      prec.right(3, seq($.function_call, '^', $.power_expression)),
      prec.right(3, seq($.method_call, '^', $.power_expression)),
      prec.right(3, seq($.unary_expression, '^', $.power_expression))
    ),

    binary_expression: $ => choice(
      $.additive_expression,
      $.multiplicative_expression,
      $.power_expression
    ),

    unary_expression: $ => prec(4, seq(
      choice('+', '-', '!'),
      $.primary_expression
    )),

    primary_expression: $ => choice(
      $.identifier,
      $.complex_number,
      $.number,
      $.string_literal,
      $.unit_expression,
      $.piecewise_expression,
      $.array_expression,
      $.array_access,
      seq('(', $.expression, ')')
    ),

    string_literal: $ => seq(
      '"',
      /[^"]*/,
      '"'
    ),

    unit_expression: $ => prec(10, seq(
      $.number,
      $.unit_identifier,
      optional(prec(11, seq('^', $.number)))
    )),

    unit_identifier: $ => choice(
      // Length units
      'mm', 'cm', 'm', 'km', 'in', 'ft', 'yd', 'mi',
      // Mass units
      'kg', 'g', 'mg', 'lb', 'oz', 'ton',
      // Force units
      'N', 'kN', 'lbf', 'kip',
      // Pressure/Stress units
      'Pa', 'kPa', 'MPa', 'GPa', 'psi', 'ksi',
      // Time units
      's', 'ms', 'min', 'h',
      // Temperature units
      'K', 'C', 'F',
      // Volume units
      'L', 'gal'
    ),

    function_call: $ => prec(1, seq(
      $.identifier,
      '(',
      optional($.argument_list),
      ')'
    )),

    method_call: $ => prec(2, seq(
      $.primary_expression,
      '.',
      $.identifier,
      '(',
      optional($.argument_list),
      ')'
    )),

    argument_list: $ => prec(1, seq(
      $.expression,
      repeat(seq(',', $.expression))
    )),

    import_statement: $ => choice(
      seq(
        'from',
        $.identifier,
        'import',
        $.import_item_list,
        ';'
      ),
      seq(
        'import',
        $.identifier,
        ';'
      )
    ),

    import_item_list: $ => seq(
      $.import_item,
      repeat(seq(',', $.import_item))
    ),

    import_item: $ => seq(
      $.identifier,
      optional(seq('as', $.identifier))
    ),

    function_list: $ => seq(
      $.identifier,
      repeat(seq(',', $.identifier))
    ),

    identifier: $ => choice(
      // LaTeX fraction - most specific first
      $.latex_frac,
      // Greek letters with optional subscripts
      $.greek_identifier,
      // Regular identifiers with subscripts - use token to make it atomic
      token(seq(/[a-zA-Z][a-zA-Z0-9]*/, /_\{[a-zA-Z0-9_,]+\}/)),
      // Standard identifiers (lowest precedence)
      /[a-zA-Z_][a-zA-Z0-9_]*/
    ),

    greek_identifier: $ => seq(
      choice(
        // Lowercase Greek letters
        '\\alpha', '\\beta', '\\gamma', '\\delta', '\\epsilon', '\\varepsilon',
        '\\zeta', '\\eta', '\\theta', '\\vartheta', '\\iota', '\\kappa',
        '\\lambda', '\\mu', '\\nu', '\\xi', '\\pi', '\\varpi', '\\rho',
        '\\varrho', '\\sigma', '\\varsigma', '\\tau', '\\upsilon', '\\phi',
        '\\varphi', '\\chi', '\\psi', '\\omega',
        // Uppercase Greek letters
        '\\Alpha', '\\Beta', '\\Gamma', '\\Delta', '\\Epsilon', '\\Zeta',
        '\\Eta', '\\Theta', '\\Iota', '\\Kappa', '\\Lambda', '\\Mu',
        '\\Nu', '\\Xi', '\\Pi', '\\Rho', '\\Sigma', '\\Tau', '\\Upsilon',
        '\\Phi', '\\Chi', '\\Psi', '\\Omega'
      ),
      optional($.subscript)
    ),

    latex_frac: $ => seq(
      '\\frac',
      '{',
      $.latex_content,
      '}',
      '{',
      $.latex_content,
      '}'
    ),

    latex_content: $ => repeat1(choice(
      /[a-zA-Z_][a-zA-Z0-9_]*/,  // Regular identifiers inside LaTeX
      seq('\\', choice(
        'alpha', 'beta', 'gamma', 'delta', 'epsilon', 'varepsilon',
        'zeta', 'eta', 'theta', 'vartheta', 'iota', 'kappa',
        'lambda', 'mu', 'nu', 'xi', 'pi', 'varpi', 'rho',
        'varrho', 'sigma', 'varsigma', 'tau', 'upsilon', 'phi',
        'varphi', 'chi', 'psi', 'omega',
        'Alpha', 'Beta', 'Gamma', 'Delta', 'Epsilon', 'Zeta',
        'Eta', 'Theta', 'Iota', 'Kappa', 'Lambda', 'Mu',
        'Nu', 'Xi', 'Pi', 'Rho', 'Sigma', 'Tau', 'Upsilon',
        'Phi', 'Chi', 'Psi', 'Omega'
      )),
      seq('{', optional($.latex_content), '}'),  // Nested braces
      /[^{}\\]+/  // Regular text without braces or backslashes
    )),

    subscript: $ => seq(
      '_{',
      /[a-zA-Z0-9_,\s]+/,
      '}'
    ),

    piecewise_function_declaration: $ => seq(
      $.identifier,
      '(',
      optional($.parameter_list),
      ')',
      ':=',
      $.piecewise_expression,
      ';'
    ),

    piecewise_expression: $ => seq(
      'piecewise',
      '{',
      repeat(seq($.piecewise_case, ',')),
      optional($.piecewise_case),
      '}'
    ),

    piecewise_case: $ => seq(
      '(',
      $.expression,
      ',',
      choice(
        $.expression,  // condition
        'otherwise'    // default case
      ),
      ')'
    ),

    number: $ => /\d+(\.\d+)?/,

    complex_number: $ => choice(
      // Complex numbers with both real and imaginary parts: 1 + 2i, 3 - 4i
      token(seq(
        /\d+(\.\d+)?/,
        /\s*/,
        choice('+', '-'),
        /\s*/,
        optional(/\d+(\.\d+)?/),
        'i'
      )),
      // Pure imaginary numbers: 2i, i
      token(seq(
        optional(/\d+(\.\d+)?/),
        'i'
      ))
    ),

    array_expression: $ => seq(
      '[',
      optional($.array_elements),
      ']'
    ),

    array_elements: $ => choice(
      $.row_vector_elements,
      $.column_vector_elements,
      $.matrix_elements
    ),

    row_vector_elements: $ => prec.left(1, seq(
      $.expression,
      repeat(seq(',', $.expression))
    )),

    column_vector_elements: $ => prec.left(2, seq(
      $.expression,
      repeat1(seq(';', $.expression))
    )),

    matrix_elements: $ => prec.left(3, seq(
      $.matrix_row,
      repeat1(seq(';', $.matrix_row))
    )),

    matrix_row: $ => seq(
      $.expression,
      repeat(seq(',', $.expression))
    ),

    array_access: $ => seq(
      $.identifier,
      '[',
      $.expression,
      optional(';'),
      ']'
    )
  }
});