#ifndef COMMON_FORMATS_JSON_H
#define COMMON_FORMATS_JSON_H

#include <common/types.h>

/*
 * ============================================================================
 * Path Syntax
 * ============================================================================
 * 
 * JSON paths support the following syntax:
 *   NULL             Root value
 *   "field"          Object field at root
 *   "parent.child"   Nested object field
 *   "[0]"            Array element (when root is array)
 *   "[0].field"      Field in array element
 *   "array[0]"       Element in named array
 *   "a.b[0].c"       Complex nested path
 * 
 * Examples:
 *   json_get_i64(json, "user.id", &id);
 *   json_get_string(json, "items[0].name", &name);
 *   json_get_value(json, NULL, &root);
 * 
 * ============================================================================
 */

/*
 * ============================================================================
 * API Overview
 * ============================================================================
 * 
 * The JSON parser provides four extraction API levels, each following the
 * same pattern with different source parameters:
 * 
 * Pattern: json{SOURCE}_get_{TYPE}({SOURCE_PARAMS}, {TYPE_PARAMS})
 * 
 * Where SOURCE is:
 *   (empty)   - Direct extraction from JSON buffer
 *               json_get_*(data_buf json_data, const char* path, ...)
 * 
 *   _cursor   - Extraction from cached location
 *               json_cursor_get_*(json_cursor cursor, ...)
 * 
 *   _value    - Extraction from nested container
 *               json_value_get_*(const json_value* parent, const char* path, ...)
 * 
 *   _indexed  - Extraction from indexed container (O(1) lookups)
 *               json_indexed_get_*(const json_indexed* idx, const char* key, ...)
 * 
 * Where TYPE is one of:
 *   type, value, null, true, false, i64, f64, string, bool
 * 
 * Key behaviors:
 *   - _get_type() returns unvalidated guesses based on first character
 *   - _get_i64() rejects floats; use _get_f64() for lenient number parsing
 *   - _get_f64() accepts both integers and floats
 *   - _get_null/true/false() return 0 if match, -1 otherwise
 * 
 * Additional functions at each level:
 *   json_has_key()         - Check existence + optional cursor output
 *   json_value_has_key()   - Check existence in container + optional cursor
 *   json_indexed_has_key() - Check existence in index (O(1)) + optional cursor
 *   (No json_cursor_has_key - cursor already represents found value)
 * 
 * Performance guide:
 *   Single field:
 *       json_get_*()
 * 
 *   Multiple fields (static paths):
 *       json_query queries[] = {
 *           JSON_QUERY_I64("user.id", &user_id),
 *           JSON_QUERY_STRING("user.name", &user_name),
 *           JSON_QUERY_STRING("user.email", &user_email),
 *       };
 *       json_get_batch(json, queries, 3);
 * 
 *   Multiple fields (dynamic container):
 *       json_value user;
 *       json_get_value(json, "user", &user);
 *       json_indexed idx;
 *       json_index(&user, &idx);
 *       json_indexed_get_i64(&idx, "id", &id);
 *       json_indexed_get_string(&idx, "name", &name);
 *       json_indexed_free(&idx);
 * 
 *   Check then extract:
 *       json_cursor cursor;
 *       if (json_has_key(json, "user.name", &cursor)) {
 *           json_cursor_get_string(cursor, &name);
 *       }
 * 
 * ============================================================================
 */

// ============================================================================
// Type System
// ============================================================================

/**
 * Token types inferred from first character (unvalidated guess).
 * Used for quick type checking without parsing.
 */
typedef enum json_toktype {
    JSON_TOKTYPE_NULL   = 1,    // Starts with 'n' (expects "null")
    JSON_TOKTYPE_TRUE   = 2,    // Starts with 't' (expects "true")
    JSON_TOKTYPE_FALSE  = 3,    // Starts with 'f' (expects "false")
    JSON_TOKTYPE_NUMBER = 4,    // Starts with digit or '-' (could be integer or float)
    JSON_TOKTYPE_STRING = 5,    // Starts with '"'
    JSON_TOKTYPE_ARRAY  = 6,    // Starts with '['
    JSON_TOKTYPE_OBJECT = 7     // Starts with '{'
} json_toktype;

/**
 * Value types after validation and parsing.
 * Used in json_value after the value has been fully parsed.
 */
typedef enum json_valtype {
    JSON_VALTYPE_NULL       = 1,
    JSON_VALTYPE_BOOLEAN    = 2,
    JSON_VALTYPE_I64        = 3,
    JSON_VALTYPE_F64        = 4,
    JSON_VALTYPE_STRING     = 5,
    JSON_VALTYPE_ARRAY      = 6,
    JSON_VALTYPE_OBJECT     = 7
} json_valtype;

/**
 * Lightweight JSON value - zero-copy, no allocations.
 * Contains validated and parsed data.
 * 
 * @note For STRING types:
 *       - The span is COMPLETE (both ptr and len are valid)
 *       - String parsing requires scanning to find the closing quote anyway
 * 
 * @note For ARRAY and OBJECT types obtained from json_get_value():
 *       - The pointer points to the container start ('[' or '{')
 *       - The length may be 0 or incomplete until json_index() is called
 *       - This design avoids scanning the entire container unnecessarily
 */
typedef struct json_value {
    union {
        int         boolean;
        i64         integer;
        f64         decimal;
        str_view    string;     // Points into original buffer (complete span)
        str_view    array;      // Points to array in buffer (span may be incomplete)
        str_view    object;     // Points to object in buffer (span may be incomplete)
    };
    u8 type;  // json_valtype
} json_value;

/**
 * Cursor pointing to the start of a JSON value.
 * Used to cache location and avoid re-scanning.
 * 
 * @note A cursor is just a pointer to the first character of a token.
 *       The token has NOT been validated or parsed yet.
 */
typedef const char* json_cursor;

// ============================================================================
// Primary API: Direct Value Extraction
// ============================================================================

/**
 * Extract any JSON value at the given path.
 * This is the universal function - all typed extractors build on this.
 * 
 * @param json_data Input JSON buffer
 * @param path      JSON path (NULL for root)
 * @param out_value Output value (zero-copy - pointers into json_data)
 * @return 0 if primitive value (null/bool/number/string) - ready to use immediately
 *         1 if container (array/object) - see below for efficient usage
 *        -1 if not found or parse error
 * 
 * @note When return code is 0 (primitive):
 *       - Value is complete, validated, and ready to use
 *       - out_value.string/integer/boolean/decimal contains the parsed value
 * 
 * @note When return code is 1 (container):
 *       - out_value.object/array points to container start ('{' or '[')
 *       - The span length is NOT yet computed (avoids scanning entire structure)
 *       - For single field: use json_value_get_*() directly
 *       - For multiple fields: use json_index() for O(k) lookups
 *       - For many fields with shared prefixes: use json_get_batch()
 * 
 * @example Primitive value:
 *   json_value val;
 *   if (json_get_value(json, "count", &val) == 0) {
 *       printf("Count: %lld\n", val.integer);
 *   }
 * 
 * @example Container - single field:
 *   json_value user;
 *   if (json_get_value(json, "user", &user) == 1) {
 *       i64 id;
 *       json_value_get_i64(&user, "id", &id);  // Scans from container start
 *   }
 * 
 * @example Container - multiple fields:
 *   json_value user;
 *   if (json_get_value(json, "user", &user) == 1) {
 *       json_indexed idx;
 *       json_index(&user, &idx);  // Scans once, builds index
 *       
 *       i64 id;
 *       str_view name, email;
 *       json_indexed_get_i64(&idx, "id", &id);
 *       json_indexed_get_string(&idx, "name", &name);
 *       json_indexed_get_string(&idx, "email", &email);
 *       
 *       json_indexed_free(&idx);
 *   }
 * 
 * @example Best performance - batch extraction:
 *   json_query queries[] = {
 *       JSON_QUERY_I64("user.id", &id),
 *       JSON_QUERY_STRING("user.name", &name),
 *       JSON_QUERY_STRING("user.email", &email),
 *   };
 *   json_get_batch(json, queries, 3);  // One traversal
 */
int json_get_value(data_buf json_data, const char* path, json_value* out_value);

/**
 * Check if value at path is null.
 * @param json_data Input JSON buffer
 * @param path      JSON path (NULL for root)
 * @return 0 if null, -1 if not found or not null
 */
int json_get_null(data_buf json_data, const char* path);

/**
 * Check if value at path is true.
 * @param json_data Input JSON buffer
 * @param path      JSON path (NULL for root)
 * @return 0 if true, -1 if not found or not true
 */
int json_get_true(data_buf json_data, const char* path);

/**
 * Check if value at path is false.
 * @param json_data Input JSON buffer
 * @param path      JSON path (NULL for root)
 * @return 0 if false, -1 if not found or not false
 */
int json_get_false(data_buf json_data, const char* path);

/**
 * Extract an integer value at the given path.
 * Parses a JSON number as an integer. Fails if the number has a decimal point.
 * 
 * @param json_data Input JSON buffer
 * @param path      JSON path (NULL for root)
 * @param out_value Output integer
 * @return 0 on success, -1 if not found, wrong type, or not a valid integer
 * 
 * @note This function rejects floats (e.g., "123.0" will fail).
 *       Use json_get_f64() if you want to accept any numeric type.
 * @note Large integers (beyond 2^53) may lose precision if parsed with json_get_f64().
 */
int json_get_i64(data_buf json_data, const char* path, i64* out_value);

/**
 * Extract a float value at the given path.
 * Parses a JSON number as a float. Also accepts integers.
 * 
 * @param json_data Input JSON buffer
 * @param path      JSON path (NULL for root)
 * @param out_value Output float
 * @return 0 on success, -1 if not found or wrong type
 * 
 * @note This function accepts both integers and floats.
 *       Use json_get_i64() if you need strict integer validation.
 * @note Large integers (beyond 2^53) may lose precision when parsed as f64.
 */
int json_get_f64(data_buf json_data, const char* path, f64* out_value);

/**
 * Extract a string value at the given path.
 * Returns zero-copy pointer into original buffer.
 * 
 * @param json_data Input JSON buffer
 * @param path      JSON path (NULL for root)
 * @param out_value Output string view (points into json_data)
 * @return 0 on success, -1 if not found or wrong type
 * 
 * @note String is NOT null-terminated. Use str_view.len.
 * @note If string contains escapes, they are NOT unescaped in this call.
 */
int json_get_string(data_buf json_data, const char* path, str_view* out_value);

/**
 * Extract a boolean value (true or false) as an integer at the given path.
 * Convenience wrapper that accepts either true or false.
 * 
 * @param json_data Input JSON buffer
 * @param path      JSON path (NULL for root)
 * @param out_value Output integer that will be set to 1 (true) or 0 (false)
 * @return 0 on success, -1 if not found or not a boolean
 * 
 * @note For explicit true/false checking, use json_get_true() or json_get_false()
 */
int json_get_bool(data_buf json_data, const char* path, int* out_value);

// ============================================================================
// Cursor-Based API (Zero-Cost Location Cache)
// ============================================================================

/**
 * Check if a value exists at the given path.
 * Optionally returns cursor to avoid re-scanning.
 * 
 * @param json_data     Input JSON buffer
 * @param path          JSON path (NULL for root)
 * @param out_cursor    Optional output cursor (NULL if you don't need location)
 * @return 1 if exists, 0 if not found
 * 
 * @example
 *   json_cursor cursor;
 *   if (json_has_key(json, "user.profile", &cursor)) {
 *       i64 id;
 *       json_cursor_get_i64(cursor, &id);  // No re-scan!
 *   }
 */
int json_has_key(data_buf json_data, const char* path, json_cursor* out_cursor);

/**
 * Get the token type of a value at the given path.
 * Type is inferred from the first character ONLY (unvalidated guess).
 * Optionally returns cursor to avoid re-scanning.
 * 
 * @param json_data     Input JSON buffer
 * @param path          JSON path (NULL for root)
 * @param out_type      Output token type (UNVALIDATED - inferred from first char)
 * @param out_cursor    Optional output cursor (NULL if you don't need location)
 * @return 0 on success, -1 if not found
 * 
 * @warning This function does NOT validate the value, only peeks at the first character.
 *          Token type inference:
 *          'n' -> NULL, 't' -> TRUE, 'f' -> FALSE
 *          digit/'-' -> NUMBER (could be integer or float, or malformed)
 *          '"' -> STRING, '[' -> ARRAY, '{' -> OBJECT
 *          
 *          The value could be malformed (e.g., "tru" instead of "true").
 *          Actual validation happens during json_cursor_get_*() or json_get_value().
 * 
 * @example Cursor avoids re-scanning:
 *   json_cursor cursor;
 *   json_toktype type;
 *   if (json_get_type(json, "optional", &type, &cursor) == 0) {
 *       if (type == JSON_TOKTYPE_NULL) {
 *           printf("Field is null\n");
 *       } else if (type == JSON_TOKTYPE_STRING) {
 *           str_view s;
 *           json_cursor_get_string(cursor, &s);  // No re-scan!
 *           printf("Field: %.*s\n", (int)s.len, s.ptr);
 *       }
 *   }
 */
int json_get_type(data_buf json_data, const char* path, json_toktype* out_type, json_cursor* out_cursor);

// ----------------------------------------------------------------------------
// Extract from cursor (validates and parses)
// ----------------------------------------------------------------------------

/**
 * Get token type from cursor.
 * Type is inferred from first character (unvalidated guess).
 * 
 * @param cursor    Cursor from json_has_key
 * @param out_type  Output token type (UNVALIDATED - inferred from first char)
 * @return 0 on success, -1 on error
 * 
 * @warning Same as json_get_type() - this is an unvalidated guess.
 *          Actual validation happens during extraction.
 */
int json_cursor_get_type(json_cursor cursor, json_toktype* out_type);

/**
 * Check if cursor points to null.
 * Validates that the value is exactly "null".
 * 
 * @param cursor Cursor from json_has_key/json_get_type
 * @return 0 if null, -1 if not null or malformed
 */
int json_cursor_get_null(json_cursor cursor);

/**
 * Check if cursor points to true.
 * Validates that the value is exactly "true".
 * 
 * @param cursor Cursor from json_has_key/json_get_type
 * @return 0 if true, -1 if not true or malformed
 */
int json_cursor_get_true(json_cursor cursor);

/**
 * Check if cursor points to false.
 * Validates that the value is exactly "false".
 * 
 * @param cursor Cursor from json_has_key/json_get_type
 * @return 0 if false, -1 if not false or malformed
 */
int json_cursor_get_false(json_cursor cursor);

/**
 * Extract and parse integer from cursor.
 * Validates that the value is a well-formed integer (rejects floats).
 * 
 * @param cursor    Cursor from json_has_key/json_get_type
 * @param out_value Output integer
 * @return 0 on success, -1 if wrong type or malformed
 * 
 * @note Rejects floats. Use json_cursor_get_f64() to accept any numeric type.
 */
int json_cursor_get_i64(json_cursor cursor, i64* out_value);

/**
 * Extract and parse float from cursor.
 * Validates that the value is a well-formed number (accepts integers).
 * 
 * @param cursor    Cursor from json_has_key/json_get_type
 * @param out_value Output float
 * @return 0 on success, -1 if wrong type or malformed
 * 
 * @note Accepts both integers and floats.
 */
int json_cursor_get_f64(json_cursor cursor, f64* out_value);

/**
 * Extract string from cursor.
 * Validates string format and returns span (without quotes).
 * 
 * @param cursor    Cursor from json_has_key/json_get_type
 * @param out_value Output string span (zero-copy, points into buffer)
 * @return 0 on success, -1 if wrong type or malformed
 * 
 * @note String is NOT null-terminated. Use str_view.len.
 * @note Escapes are NOT unescaped.
 */
int json_cursor_get_string(json_cursor cursor, str_view* out_value);

/**
 * Extract and parse boolean (true or false) as an integer from cursor.
 * Convenience wrapper that accepts either true or false.
 * 
 * @param cursor    Cursor from json_has_key/json_get_type
 * @param out_value Output integer that will be set to 1 (true) or 0 (false)
 * @return 0 on success, -1 if not a boolean or malformed
 * 
 * @note For explicit true/false checking, use json_cursor_get_true() or json_cursor_get_false()
 */
int json_cursor_get_bool(json_cursor cursor, int* out_value);

/**
 * Extract and parse value from cursor into json_value.
 * Validates and parses the value completely.
 * 
 * @param cursor    Cursor from json_has_key/json_get_type
 * @param out_value Output value (validated and parsed)
 * @return 0 if primitive, 1 if container, -1 if malformed
 * 
 * @note For containers, span length may be incomplete.
 *       Use json_index() for multi-field access.
 */
int json_cursor_get_value(json_cursor cursor, json_value* out_value);

// ============================================================================
// Array Iteration (Zero-Copy Iterator)
// ============================================================================

/**
 * Iterator for arrays - no allocations.
 * 
 * @example
 *   json_array_iter iter;
 *   json_array_begin(json, "users", &iter);
 *   
 *   json_value item;
 *   while (json_array_next(&iter, &item)) {
 *       if (item.type == JSON_VALTYPE_OBJECT) {
 *           i64 id;
 *           json_value_get_i64(&item, "id", &id);
 *       }
 *   }
 */
typedef struct json_array_iter {
    const char* ptr;
    const char* end;
    size_t      index;
} json_array_iter;

/**
 * Initialize iterator for an array at the given path.
 * @param json_data Input JSON buffer
 * @param path      JSON path (NULL for root array)
 * @param out_iter  Output iterator (uninitialized)
 * @return 0 on success, -1 if not found or not an array
 */
int json_array_begin(data_buf json_data, const char* path, json_array_iter* out_iter);

/**
 * Advance to next array item.
 * @param iter      Iterator state (must be initialized by json_array_begin)
 * @param out_value Output value for current item (zero-copy)
 * @return 1 if item available, 0 if done iterating
 */
int json_array_next(json_array_iter* iter, json_value* out_value);

/**
 * Get current array index during iteration.
 * @param iter Active iterator
 * @return Current index (0-based)
 */
size_t json_array_index(const json_array_iter* iter);

// ============================================================================
// Object Iteration (Zero-Copy Iterator)
// ============================================================================

/**
 * Iterator for objects - no allocations.
 * 
 * @example
 *   json_object_iter iter;
 *   json_object_begin(json, NULL, &iter);  // NULL = root object
 *   
 *   str_view key;
 *   json_value value;
 *   while (json_object_next(&iter, &key, &value)) {
 *       printf("Key: %.*s, Type: %d\n", 
 *              (int)key.len, key.ptr, value.type);
 *   }
 */
typedef struct json_object_iter {
    const char* ptr;
    const char* end;
} json_object_iter;

/**
 * Initialize iterator for an object at the given path.
 * @param json_data Input JSON buffer
 * @param path      JSON path (NULL for root object)
 * @param out_iter  Output iterator (uninitialized)
 * @return 0 on success, -1 if not found or not an object
 */
int json_object_begin(data_buf json_data, const char* path, json_object_iter* out_iter);

/**
 * Advance to next key-value pair in object.
 * @param iter      Active iterator (must be initialized)
 * @param out_key   Output key (zero-copy pointer into buffer)
 * @param out_value Output value (zero-copy)
 * @return 1 if pair available, 0 if done iterating
 */
int json_object_next(json_object_iter* iter, str_view* out_key, json_value* out_value);

// ============================================================================
// Extract from json_value (For nested access)
// ============================================================================

/**
 * Extract a value from within a json_value.
 * Useful when iterating arrays/objects and need to access nested fields.
 * 
 * @param parent    Parent json_value (must be OBJECT or ARRAY type)
 * @param path      JSON path within parent
 * @param out_value Output value (zero-copy)
 * @return 0 if primitive, 1 if container, -1 if not found or parent is wrong type
 * 
 * @note This function scans from the container start, so it works correctly
 *       even if the parent container has an incomplete span (from json_get_value).
 * 
 * @example From array iteration:
 *   json_value user;  // From json_array_next()
 *   json_value email;
 *   json_value_get(&user, "profile.email", &email);
 * 
 * @example From json_get_value (container):
 *   json_value data;
 *   if (json_get_value(json, "data", &data) == 1) {  // Container
 *       // Extract single field without indexing
 *       i64 id;
 *       json_value_get_i64(&data, "id", &id);
 *   }
 */
int json_value_get(const json_value* parent, const char* path, json_value* out_value);

/**
 * Check if a value exists within a json_value.
 * Optionally returns cursor to avoid re-scanning.
 * 
 * @param parent     Parent json_value (must be OBJECT or ARRAY type)
 * @param path       JSON path within parent
 * @param out_cursor Optional output cursor (NULL if you don't need location)
 * @return 1 if exists, 0 if not found
 */
int json_value_has_key(const json_value* parent, const char* path, json_cursor* out_cursor);

/**
 * Get token type of value within a json_value.
 * Type is inferred from first character (unvalidated guess).
 * Optionally returns cursor to avoid re-scanning.
 * 
 * @param parent     Parent json_value (must be OBJECT or ARRAY type)
 * @param path       JSON path within parent
 * @param out_type   Output token type (UNVALIDATED - inferred from first char)
 * @param out_cursor Optional output cursor (NULL if you don't need location)
 * @return 0 on success, -1 if not found
 * 
 * @warning Same as json_get_type() - this is an unvalidated guess.
 */
int json_value_get_type(const json_value* parent, const char* path, json_toktype* out_type, json_cursor* out_cursor);

/**
 * Check if value within a json_value is null.
 * @param parent Parent json_value (must be OBJECT or ARRAY type)
 * @param path   JSON path within parent
 * @return 0 if null, -1 if not found or not null
 */
int json_value_get_null(const json_value* parent, const char* path);

/**
 * Check if value within a json_value is true.
 * @param parent Parent json_value (must be OBJECT or ARRAY type)
 * @param path   JSON path within parent
 * @return 0 if true, -1 if not found or not true
 */
int json_value_get_true(const json_value* parent, const char* path);

/**
 * Check if value within a json_value is false.
 * @param parent Parent json_value (must be OBJECT or ARRAY type)
 * @param path   JSON path within parent
 * @return 0 if false, -1 if not found or not false
 */
int json_value_get_false(const json_value* parent, const char* path);

/**
 * Extract an integer from within a json_value.
 * @param parent    Parent json_value (must be OBJECT or ARRAY type)
 * @param path      JSON path within parent
 * @param out_value Output integer
 * @return 0 on success, -1 if not found or wrong type
 * 
 * @note Rejects floats. Use json_value_get_f64() to accept any numeric type.
 */
int json_value_get_i64(const json_value* parent, const char* path, i64* out_value);

/**
 * Extract a float from within a json_value.
 * @param parent    Parent json_value (must be OBJECT or ARRAY type)
 * @param path      JSON path within parent
 * @param out_value Output float
 * @return 0 on success, -1 if not found or wrong type
 * 
 * @note Accepts both integers and floats.
 */
int json_value_get_f64(const json_value* parent, const char* path, f64* out_value);

/**
 * Extract a boolean as an integer from within a json_value.
 * Convenience wrapper that accepts either true or false.
 * 
 * @param parent    Parent json_value (must be OBJECT or ARRAY type)
 * @param path      JSON path within parent
 * @param out_value Output integer that will be set to 1 (true) or 0 (false)
 * @return 0 on success, -1 if not found or wrong type
 * 
 * @note For explicit true/false checking, use json_value_get_true() or json_value_get_false()
 */
int json_value_get_bool(const json_value* parent, const char* path, int* out_value);

/**
 * Extract a string from within a json_value.
 * @param parent    Parent json_value (must be OBJECT or ARRAY type)
 * @param path      JSON path within parent
 * @param out_value Output string view (zero-copy)
 * @return 0 on success, -1 if not found or wrong type
 */
int json_value_get_string(const json_value* parent, const char* path, str_view* out_value);

// ============================================================================
// Indexed Containers (For Multi-Field Access)
// ============================================================================

/**
 * Indexed container - optimized for fast multi-field lookups.
 * Created by json_index() when you need to extract multiple fields from
 * the same container without re-scanning.
 */
typedef struct json_indexed {
    void*       expr;   // Internal tokenized representation (opaque)
    str_view    span;   // Original span in buffer
    u8          type;   // JSON_VALTYPE_ARRAY or JSON_VALTYPE_OBJECT
} json_indexed;

/**
 * Index an array/object for efficient multi-field access.
 * Scans the container once to build an indexed representation for O(k) lookups.
 * 
 * @param value         Container value (type must be ARRAY or OBJECT)
 *                      Typically obtained from json_get_value() with return code 1
 * @param out_idx       Output indexed container
 * @return 0 on success, -1 if wrong type or parse error
 * 
 * @note This allocates memory for token index. Must call json_indexed_free().
 * @note This function scans the entire container to:
 *       1. Determine the complete span (if not already known)
 *       2. Build a token index for fast field lookups
 * @note Only use this if you need to extract MULTIPLE fields from the same container.
 *       For single field extraction, json_value_get_*() is more efficient.
 * 
 * @example
 *   json_value user;
 *   if (json_get_value(json, "user", &user) == 1) {  // Container
 *       json_indexed idx;
 *       json_index(&user, &idx);  // Scans once, builds index
 *       
 *       // Now all field extractions are O(k) lookups, not O(n) scans
 *       i64 id, age;
 *       str_view name, email, phone;
 *       int is_active;
 *       
 *       json_indexed_get_i64(&idx, "id", &id);
 *       json_indexed_get_i64(&idx, "age", &age);
 *       json_indexed_get_string(&idx, "name", &name);
 *       json_indexed_get_string(&idx, "email", &email);
 *       json_indexed_get_string(&idx, "phone", &phone);
 *       json_indexed_get_bool(&idx, "is_active", &is_active);
 *       
 *       json_indexed_free(&idx);
 *   }
 * 
 * @note Alternative: For fields with shared path prefixes, json_get_batch()
 *       may be more efficient as it shares the initial path traversal.
 */
int json_index(const json_value* value, json_indexed* out_idx);

/**
 * Free indexed container.
 * @param idx Indexed container to free
 */
void json_indexed_free(json_indexed* idx);

// ----------------------------------------------------------------------------
// Extract from indexed container (fast - uses token index)
// ----------------------------------------------------------------------------

/**
 * Extract value from indexed container.
 * Uses token index for faster lookups than scanning.
 * 
 * @param idx       Indexed container
 * @param key       Field name (for objects) or NULL (for arrays with index in path)
 * @param out_value Output value
 * @return 0 on success, -1 if not found or wrong type
 */
int json_indexed_get_value(const json_indexed* idx, const char* key, json_value* out_value);

/**
 * Check if a key exists in indexed container.
 * Optionally returns cursor to avoid re-scanning.
 * 
 * @param idx        Indexed container
 * @param key        Field name (for objects) or NULL (for arrays)
 * @param out_cursor Optional output cursor (NULL if you don't need location)
 * @return 1 if exists, 0 if not found
 * 
 * @note For indexed containers, existence check is O(1) using the token index.
 */
int json_indexed_has_key(const json_indexed* idx, const char* key, json_cursor* out_cursor);

/**
 * Get token type of value in indexed container.
 * Type is looked up from the token index (fast - no parsing).
 * Optionally returns cursor to avoid re-scanning.
 * 
 * @param idx        Indexed container
 * @param key        Field name (for objects) or NULL (for arrays)
 * @param out_type   Output token type
 * @param out_cursor Optional output cursor (NULL if you don't need location)
 * @return 0 on success, -1 if not found
 * 
 * @note For indexed containers, the type is stored in the index (O(1) lookup).
 */
int json_indexed_get_type(const json_indexed* idx, const char* key, json_toktype* out_type, json_cursor* out_cursor);

/**
 * Check if value in indexed container is null.
 * @param idx Indexed container
 * @param key Field name (for objects) or NULL (for arrays)
 * @return 0 if null, -1 if not found or not null
 */
int json_indexed_get_null(const json_indexed* idx, const char* key);

/**
 * Check if value in indexed container is true.
 * @param idx Indexed container
 * @param key Field name (for objects) or NULL (for arrays)
 * @return 0 if true, -1 if not found or not true
 */
int json_indexed_get_true(const json_indexed* idx, const char* key);

/**
 * Check if value in indexed container is false.
 * @param idx Indexed container
 * @param key Field name (for objects) or NULL (for arrays)
 * @return 0 if false, -1 if not found or not false
 */
int json_indexed_get_false(const json_indexed* idx, const char* key);

/**
 * Extract integer from indexed container.
 * @param idx       Indexed container
 * @param key       Field name (for objects) or NULL (for arrays)
 * @param out_value Output integer
 * @return 0 on success, -1 if not found or wrong type
 * 
 * @note Rejects floats. Use json_indexed_get_f64() to accept any numeric type.
 */
int json_indexed_get_i64(const json_indexed* idx, const char* key, i64* out_value);

/**
 * Extract float from indexed container.
 * @param idx       Indexed container
 * @param key       Field name (for objects) or NULL (for arrays)
 * @param out_value Output float
 * @return 0 on success, -1 if not found or wrong type
 * 
 * @note Accepts both integers and floats.
 */
int json_indexed_get_f64(const json_indexed* idx, const char* key, f64* out_value);

/**
 * Extract boolean as integer from indexed container.
 * Convenience wrapper that accepts either true or false.
 * @param idx       Indexed container
 * @param key       Field name (for objects) or NULL (for arrays)
 * @param out_value Output integer that will be set to 1 (true) or 0 (false)
 * @return 0 on success, -1 if not found or wrong type
 * 
 * @note For explicit true/false checking, use json_indexed_get_true() or json_indexed_get_false()
 */
int json_indexed_get_bool(const json_indexed* idx, const char* key, int* out_value);

/**
 * Extract string from indexed container.
 * @param idx       Indexed container
 * @param key       Field name (for objects) or NULL (for arrays)
 * @param out_value Output string (zero-copy)
 * @return 0 on success, -1 if not found or wrong type
 */
int json_indexed_get_string(const json_indexed* idx, const char* key, str_view* out_value);

/**
 * Iterate indexed array efficiently.
 * @param idx       Indexed array
 * @param out_iter  Output iterator
 * @return 0 on success, -1 if not an array
 */
int json_indexed_array_begin(const json_indexed* idx, json_array_iter* out_iter);

/**
 * Iterate indexed object efficiently.
 * @param idx       Indexed object
 * @param out_iter  Output iterator
 * @return 0 on success, -1 if not an object
 */
int json_indexed_object_begin(const json_indexed* idx, json_object_iter* out_iter);

// ============================================================================
// Batch Extraction (Avoid Duplicate Path Traversal)
// ============================================================================

/**
 * Query descriptor for batch extraction.
 * Specifies a path and where to store the result.
 */
typedef struct json_query {
    const char*     path;       // Path to extract (e.g., "user.profile.email")
    json_valtype    expected;   // Expected type (or JSON_VALTYPE_NULL for any)
    void*           out_value;  // Output pointer (type depends on expected)
    int             status;     // Output: 0 = success, -1 = not found, -2 = wrong type
} json_query;

/**
 * Extract multiple values in a single pass.
 * More efficient than multiple json_get_*() calls when paths share prefixes.
 * 
 * @param json_data Input JSON buffer
 * @param queries   Array of query descriptors
 * @param count     Number of queries
 * @return Number of successful extractions
 * 
 * @example
 *   i64 user_id;
 *   str_view user_name, user_email;
 *   int is_active;
 *   
 *   json_query queries[] = {
 *       JSON_QUERY_I64("user.id", &user_id),
 *       JSON_QUERY_STRING("user.name", &user_name),
 *       JSON_QUERY_STRING("user.email", &user_email),
 *       JSON_QUERY_BOOL("user.active", &is_active),
 *   };
 *   
 *   int found = json_get_batch(json, queries, 4);
 *   
 *   // Check individual status
 *   if (queries[0].status == 0) {
 *       printf("User ID: %lld\n", user_id);
 *   }
 */
int json_get_batch(data_buf json_data, json_query* queries, size_t count);

/**
 * Typed convenience macros for batch extraction.
 * Automatically sets expected type based on usage.
 */
#define JSON_QUERY_NULL(path)                                                                   \
    { .path = (path), .expected = JSON_VALTYPE_NULL, .out_value = NULL, .status = -1 }          \

#define JSON_QUERY_I64(path, out_ptr)                                                           \
    { .path = (path), .expected = JSON_VALTYPE_I64, .out_value = (out_ptr), .status = -1 }      \

#define JSON_QUERY_F64(path, out_ptr)                                                           \
    { .path = (path), .expected = JSON_VALTYPE_F64, .out_value = (out_ptr), .status = -1 }      \

#define JSON_QUERY_BOOL(path, out_ptr)                                                          \
    { .path = (path), .expected = JSON_VALTYPE_BOOLEAN, .out_value = (out_ptr), .status = -1 }  \

#define JSON_QUERY_STRING(path, out_ptr)                                                        \
    { .path = (path), .expected = JSON_VALTYPE_STRING, .out_value = (out_ptr), .status = -1 }   \

#define JSON_QUERY_VALUE(path, out_ptr)                                                         \
    { .path = (path), .expected = JSON_VALTYPE_NULL, .out_value = (out_ptr), .status = -1 }     \

// ============================================================================
// Advanced: Tokenization (Rarely Needed)
// ============================================================================

/**
 * Most users should never need this.
 * Only use if you need to inspect the token stream directly.
 */
typedef struct json_expr {
    u8*         types;
    str_view*   spans;
    size_t      token_count;
    size_t      span_count;
} json_expr;

/**
 * Tokenize JSON into a lightweight token stream.
 * @param json_data Input JSON buffer
 * @param out_expr  Output token stream (caller must free with json_expr_free)
 * @return 0 on success, -1 on parse error
 * 
 * @note This allocates memory for token arrays. Must call json_expr_free().
 * @note Most users should use json_get_value() instead.
 */
int json_expr_lex(data_buf json_data, json_expr* out_expr);

/**
 * Free memory allocated by json_expr_lex.
 * @param expr Token stream to free
 */
void json_expr_free(json_expr* expr);

// ============================================================================
// Validation
// ============================================================================

/**
 * Validate JSON grammar without parsing.
 * Fast check before processing untrusted input.
 * 
 * @param json_data Input JSON buffer
 * @return 0 if valid, -1 if invalid
 * 
 * @note This only checks syntax, not semantics.
 * @note Numbers are not validated for overflow.
 */
int json_validate(data_buf json_data);

#endif // COMMON_FORMATS_JSON_H