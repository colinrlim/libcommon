#include <common/regex/regex_expr.h>

#include <stdlib.h>

#define DEFINE_REGEX_EXPR_GROW(T, property)                                         \
static int grow_##property##s(regex_expr* out) {                                    \
    if (out->property##_count < out->property##_capacity) return 0;                 \
    size_t  new_cap = out->property##_capacity ? out->property##_capacity * 2 : 4;  \
    T*      new_mem = realloc(out->property##s, new_cap * sizeof(T));               \
    if (!new_mem) return -1;                                                        \
    out->property##s            = new_mem;                                          \
    out->property##_capacity    = new_cap;                                          \
    return 0;                                                                       \
}                                                                                   \

DEFINE_REGEX_EXPR_GROW(str_view,    literal);
DEFINE_REGEX_EXPR_GROW(ascii_mask,  mask);

int regex_expr_lex(str_view pattern, regex_expr* const restrict out_expr) {
    if (!pattern.ptr || !out_expr) return -1;

    out_expr->count         = 0;
    out_expr->capture_count = 0;
    
    out_expr->literals          = NULL;
    out_expr->literal_count     = 0;
    out_expr->literal_capacity  = 0;
    out_expr->masks         = NULL;
    out_expr->mask_count    = 0;
    out_expr->mask_capacity = 0;
    
    // Max token count is set to length of pattern string literal
    out_expr->tokens = malloc((pattern.len > 0 ? pattern.len : 1) * sizeof(regex_token));
    if (!out_expr->tokens) return -1;

    const char* p   = pattern.ptr;
    const char* end = pattern.ptr + pattern.len;

    // Fast-access pointer to hot stream
    regex_token* t_ptr = out_expr->tokens;

    while (p < end) {
        char c = *p;
        switch (c) {
        /* --- Operators & Anchors --- */
        case '|': t_ptr->type = REGEX_TOKEN_OR;             t_ptr->c = 0; t_ptr++; p++; break;
        case '*': t_ptr->type = REGEX_TOKEN_STAR;           t_ptr->c = 0; t_ptr++; p++; break;
        case '+': t_ptr->type = REGEX_TOKEN_PLUS;           t_ptr->c = 0; t_ptr++; p++; break;
        case '?': t_ptr->type = REGEX_TOKEN_QUEST;          t_ptr->c = 0; t_ptr++; p++; break;
        case '.': t_ptr->type = REGEX_TOKEN_ANY;            t_ptr->c = 0; t_ptr++; p++; break;
        case '^': t_ptr->type = REGEX_TOKEN_ANCHOR_START;   t_ptr->c = 0; t_ptr++; p++; break;
        case '$': t_ptr->type = REGEX_TOKEN_ANCHOR_END;     t_ptr->c = 0; t_ptr++; p++; break;
        case '(': {
            const size_t id = out_expr->capture_count++;
            t_ptr->type = REGEX_TOKEN_CAPTURE;
            t_ptr->c    = (u8)(id & 0xFF);
            t_ptr++;
            p++; // skip '('
            break;
        }
        case ')': // In case of infix
            t_ptr->type = REGEX_TOKEN_PAREN_CLOSE;
            t_ptr->c    = 0;
            t_ptr++;
            p++; // skip ')'
            break;

        /* --- Character Classes --- */
        case '[': {
            p++; // skip '['
            if (grow_masks(out_expr) != 0) goto error;
            
            ascii_mask* mask = &out_expr->masks[out_expr->mask_count++];
            mask->chunks[0] = 0;
            mask->chunks[1] = 0;
            
            while (p < end && *p != ']') {
                const u8 r_start = (u8)*p;
                p++;        // consume start char
                if (p + 1 < end && *p == '-') {
                    p++;    // skip '-'
                    const u8 r_end = (u8)*p;
                    p++;    // consume end char
                    ascii_mask_add_range(mask, r_start, r_end);
                } else {
                    ascii_mask_add_char(mask, r_start);
                }
            }
            if (p < end) p++; else goto error; // No ']'
            t_ptr->type = REGEX_TOKEN_MASK;
            t_ptr->c    = 0;
            t_ptr++;
            break;
        }
        
        /* --- Escapes & Literals --- */
        case '\\':
            p++;
            if (p >= end) goto error;
            c = *p;
            __attribute__((fallthrough));
        default: {
            const char* start = p;
            
            // Scan for string fusion
            while (p < end) {
                char n = *p;
                if (n == '.' || n == '^' || n == '$' || n == '(' || n == ')' || 
                    n == '[' || n == ']' || n == '{' || n == '}' || 
                    n == '*' || n == '+' || n == '?' || n == '|' || n == '\\') {
                    break;
                }
                p++;
            }
            
            const size_t len = p - start;
            if (len == 0) { // Special char after backslash (e.g. \*)
                t_ptr->type = REGEX_TOKEN_CHAR;
                t_ptr->c    = (u8)c;
                t_ptr++;
                p++;
            } else if (len == 1) {
                t_ptr->type = REGEX_TOKEN_CHAR;
                t_ptr->c    = (u8)*start;
                t_ptr++;
            } else {
                if (grow_literals(out_expr) != 0) goto error;
                out_expr->literals[out_expr->literal_count++] = (str_view){ start, len };
                
                t_ptr->type = REGEX_TOKEN_STR;
                t_ptr->c    = (u8)*start; // Guard Byte
                t_ptr++;
            }
            break;
        }
        }
    }
    out_expr->count = (size_t)(t_ptr - out_expr->tokens);
    return 0;
error:
    regex_expr_free(out_expr);
    return -1;
}

void regex_expr_free(regex_expr* expr) {
    if (expr->tokens)   free(expr->tokens);
    if (expr->literals) free(expr->literals);
    if (expr->masks)    free(expr->masks);
}