// MADOLA Language Definition for Monaco Editor

function registerMadolaLanguage() {
    if (!window.monaco) {
        console.error('Monaco Editor not loaded');
        return;
    }

    // Register the MADOLA language
    monaco.languages.register({ id: 'madola' });

    // Define tokens for syntax highlighting
    monaco.languages.setMonarchTokensProvider('madola', {
        defaultToken: '',
        tokenPostfix: '.mda',

        keywords: [
            'fn', 'if', 'else', 'for', 'in', 'while', 'return',
            'import', 'from', 'as', 'export', 'const', 'let'
        ],

        decorators: [
            '@gen_cpp', '@gen_addon', '@resolveAlign', '@h1', '@h2', '@h3', '@h4',
            '@p', '@align_left', '@align_center', '@align_right', '@bold', '@italic'
        ],

        operators: [
            '=', '>', '<', '!', '~', '?', ':',
            '==', '<=', '>=', '!=', '&&', '||', '++', '--',
            '+', '-', '*', '/', '&', '|', '^', '%', '<<',
            '>>', '>>>', '+=', '-=', '*=', '/=', '&=', '|=',
            '^=', '%=', '<<=', '>>=', '>>>='
        ],

        // Common regular expressions
        symbols: /[=><!~?:&|+\-*\/\^%]+/,
        escapes: /\\(?:[abfnrtv\\"']|x[0-9A-Fa-f]{1,4}|u[0-9A-Fa-f]{4}|U[0-9A-Fa-f]{8})/,
        digits: /\d+(_+\d+)*/,
        octaldigits: /[0-7]+(_+[0-7]+)*/,
        binarydigits: /[0-1]+(_+[0-1]+)*/,
        hexdigits: /[[0-9a-fA-F]+(_+[0-9a-fA-F]+)*/,

        tokenizer: {
            root: [
                // Decorators
                [/@[a-zA-Z_]\w*/, {
                    cases: {
                        '@decorators': 'decorator',
                        '@default': 'decorator'
                    }
                }],

                // Identifiers and keywords
                [/[a-zA-Z_]\w*/, {
                    cases: {
                        '@keywords': 'keyword',
                        '@default': 'identifier'
                    }
                }],

                // Greek letters (LaTeX style)
                [/\\[a-zA-Z]+/, 'type.identifier'],

                // Whitespace
                { include: '@whitespace' },

                // Delimiters and operators
                [/[{}()\[\]]/, '@brackets'],
                [/[<>](?!@symbols)/, '@brackets'],
                [/@symbols/, {
                    cases: {
                        '@operators': 'operator',
                        '@default': ''
                    }
                }],

                // Numbers
                [/(@digits)[eE]([\-+]?(@digits))?/, 'number.float'],
                [/(@digits)\.(@digits)([eE][\-+]?(@digits))?/, 'number.float'],
                [/0[xX](@hexdigits)/, 'number.hex'],
                [/0[oO]?(@octaldigits)/, 'number.octal'],
                [/0[bB](@binarydigits)/, 'number.binary'],
                [/(@digits)/, 'number'],

                // Delimiter: after number because of .\d floats
                [/[;,.]/, 'delimiter'],

                // Strings
                [/"([^"\\]|\\.)*$/, 'string.invalid'],  // non-terminated string
                [/'([^'\\]|\\.)*$/, 'string.invalid'],  // non-terminated string
                [/"/, 'string', '@string_double'],
                [/'/, 'string', '@string_single'],

                // Assignment operator
                [/:=/, 'operator.assignment'],
            ],

            whitespace: [
                [/[ \t\r\n]+/, ''],
                [/\/\*/, 'comment', '@comment'],
                [/\/\/.*$/, 'comment'],
            ],

            comment: [
                [/[^\/*]+/, 'comment'],
                [/\*\//, 'comment', '@pop'],
                [/[\/*]/, 'comment']
            ],

            string_double: [
                [/[^\\"]+/, 'string'],
                [/@escapes/, 'string.escape'],
                [/\\./, 'string.escape.invalid'],
                [/"/, 'string', '@pop']
            ],

            string_single: [
                [/[^\\']+/, 'string'],
                [/@escapes/, 'string.escape'],
                [/\\./, 'string.escape.invalid'],
                [/'/, 'string', '@pop']
            ],
        },
    });

    // Define theme colors for MADOLA
    monaco.editor.defineTheme('madola-light', {
        base: 'vs',
        inherit: true,
        rules: [
            { token: 'decorator', foreground: 'e67e22', fontStyle: 'bold' },
            { token: 'keyword', foreground: '0000ff', fontStyle: 'bold' },
            { token: 'operator.assignment', foreground: 'd73a49', fontStyle: 'bold' },
            { token: 'number', foreground: '098658' },
            { token: 'number.float', foreground: '098658' },
            { token: 'string', foreground: 'a31515' },
            { token: 'comment', foreground: '008000', fontStyle: 'italic' },
            { token: 'type.identifier', foreground: '795e26' },
        ],
        colors: {}
    });

    monaco.editor.defineTheme('madola-dark', {
        base: 'vs-dark',
        inherit: true,
        rules: [
            { token: 'decorator', foreground: 'f39c12', fontStyle: 'bold' },
            { token: 'keyword', foreground: '569cd6', fontStyle: 'bold' },
            { token: 'operator.assignment', foreground: 'f92672', fontStyle: 'bold' },
            { token: 'number', foreground: 'b5cea8' },
            { token: 'number.float', foreground: 'b5cea8' },
            { token: 'string', foreground: 'ce9178' },
            { token: 'comment', foreground: '6a9955', fontStyle: 'italic' },
            { token: 'type.identifier', foreground: 'dcdcaa' },
        ],
        colors: {}
    });

    // Configure language features
    monaco.languages.setLanguageConfiguration('madola', {
        comments: {
            lineComment: '//',
            blockComment: ['/*', '*/']
        },
        brackets: [
            ['{', '}'],
            ['[', ']'],
            ['(', ')']
        ],
        autoClosingPairs: [
            { open: '{', close: '}' },
            { open: '[', close: ']' },
            { open: '(', close: ')' },
            { open: '"', close: '"' },
            { open: "'", close: "'" },
        ],
        surroundingPairs: [
            { open: '{', close: '}' },
            { open: '[', close: ']' },
            { open: '(', close: ')' },
            { open: '"', close: '"' },
            { open: "'", close: "'" },
        ],
        folding: {
            markers: {
                start: new RegExp('^\\s*//\\s*#?region\\b'),
                end: new RegExp('^\\s*//\\s*#?endregion\\b')
            }
        }
    });

    console.log('MADOLA language registered successfully');
}

// Auto-register when Monaco is available
if (typeof window !== 'undefined') {
    if (window.monaco) {
        registerMadolaLanguage();
    } else {
        // Wait for Monaco to load
        const checkMonaco = setInterval(() => {
            if (window.monaco) {
                clearInterval(checkMonaco);
                registerMadolaLanguage();
            }
        }, 100);
    }
}

