#ifndef COMMON_REGEX_EXPR_H
#define COMMON_REGEX_EXPR_H

#include <common/types.h>
#include <common/ascii_mask.h>

/**
 * Regex Token Types.   
 * The Lexer produces these tokens linearly based on the input string.
 * It is agnostic to the notation (Infix vs. Prefix/Polish).
 */
typedef enum regex_token_type {
    /* --- Atoms (Leaf Nodes) --- */
    REGEX_TOKEN_CHAR,           // c = literal character
    REGEX_TOKEN_STR,            // c = first char (Guard Byte). Index implicit in 'literals' array.
    REGEX_TOKEN_MASK,           // c = unused. Index implicit in 'masks' array.
    REGEX_TOKEN_ANY,            // c = unused. Matches '.'

    /* --- Control Flow / Operators --- */
    REGEX_TOKEN_CONCAT,         // explicit concatenation (e.g. '&') if supported
    REGEX_TOKEN_OR,             // '|'

    REGEX_TOKEN_STAR,           // '*'
    REGEX_TOKEN_PLUS,           // '+'
    REGEX_TOKEN_QUEST,          // '?'
    REGEX_TOKEN_CAPTURE,        // '(' Starts a capture. c = capture_id.
    REGEX_TOKEN_PAREN_CLOSE,    // ')' In case of infix

    /* --- Anchors --- */
    REGEX_TOKEN_ANCHOR_START,   // '^'
    REGEX_TOKEN_ANCHOR_END      // '$'
} regex_token_type;

/**
 * Ultra-compact 2-byte Token.
 * - Alignment: 1 byte
 * - Size:      2 bytes
 * - Cache Density: 32 tokens per 64-byte cache line.
 */
typedef struct regex_token {
    u8 type;    // regex_token_type
    u8 c;       // Payload or Guard Character
} regex_token;

/**
 * Structure of Arrays (SoA) Regex Representation.
 * Decouples the lightweight control flow (tokens) from heavy data (literals/masks)
 * to maximize instruction cache coherency during execution.
 */
typedef struct regex_expr {
    /* --- Hot Stream --- */
    regex_token*    tokens;
    size_t          count;

    /* --- Cold Data Pools --- */
    str_view*       literals;
    size_t          literal_count;
    size_t          literal_capacity;

    ascii_mask*     masks;
    size_t          mask_count;
    size_t          mask_capacity;

    /* --- Metadata --- */
    size_t          capture_count;
} regex_expr;

/**
 * Scans a regex pattern string into a flat token array.
 * @param pattern   The source string
 * @param out_expr  Valid pointer to the output struct. Pointers inside must be NULL or valid.
 * @return 0 on success, -1 on failure.
 * @note This function ALWAYS allocates new memory for 'out_expr->tokens' based on the 
 * pattern length. The caller is responsible for ensuring 'out_expr' is initialized 
 * (pointers set to NULL) or was previously freed to avoid memory leaks.
 */
int regex_expr_lex(str_view pattern, regex_expr* const restrict out_expr);

/**
 * Frees all dynamic memory associated with the regex expression.
 * @param expr Expression to free. Must NOT be NULL.
 * @note This function only calls free() on internal pointers. It does NOT zero
 * out the structure fields or the 'out_expr' pointer itself. The struct should 
 * be considered invalid/undefined after this call.
 */
void regex_expr_free(regex_expr* expr);

#endif // COMMON_REGEX_EXPR_H