#include "tree_sitter/api.h"

#ifdef __cplusplus
extern "C" {
#endif

// Tree-sitter parser declaration for MADOLA
const TSLanguage *tree_sitter_madola();

#ifdef __cplusplus
}
#endif

// Tree-sitter node type constants for MADOLA
typedef enum {
    TS_MADOLA_SOURCE_FILE,
    TS_MADOLA_STATEMENT,
    TS_MADOLA_ASSIGNMENT_STATEMENT,
    TS_MADOLA_PRINT_STATEMENT,
    TS_MADOLA_EXPRESSION,
    TS_MADOLA_IDENTIFIER,
    TS_MADOLA_NUMBER
} TSMadolaNodeType;