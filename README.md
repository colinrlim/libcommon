# libcommon

A portable C utility library providing performance-oriented primitives for systems programming. Designed for zero-cost abstractions, cache-friendly data structures, and cross-platform compatibility.

## Overview

libcommon provides foundational utilities used across multiple projects (GatorStation, c-webstack, etc.) with a focus on:

- **Performance-first design**: Cache-optimized structures, SWAR-ready layouts
- **Zero-copy operations**: `str_view` for string handling without allocations
- **Data-oriented patterns**: Structure of Arrays (SoA) for hot/cold data separation
- **Minimal dependencies**: Standard C library only
- **Cross-platform**: Windows and Linux support

## Features

### Core Types (`types.h`)
- Fixed-width integer aliases: `u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64`
- Floating-point aliases: `f32`, `f64`
- `str_view`: Zero-copy string view (pointer + length)
- `data_buf`: Generic buffer descriptor

### Hashing (`hash.h`)
- `str_hash()`: FNV-1a hash for `str_view` keys
- `str_eq()`: String equality comparison
- Returns minimum 1 (0 reserved for empty slots in open-addressed tables)

### Dynamic Buffer (`buf.h`)
- Macro-generated type-safe dynamic arrays via `DEFINE_BUFFER(T, T_STRUCT)`
- Amortized O(1) push, batch append
- Doubles capacity on growth (initial minimum: 8 elements)

### Hash Map (`map.h`)
- Macro-generated type-safe hash maps via `DEFINE_MAP(T, T_STRUCT)`
- `str_view` keys with open addressing and linear probing
- 70% load factor, automatic growth

### Symbol Table (`symbol_table.h`)
- Macro-generated scoped hash map via `DEFINE_SYMBOL_TABLE(T, T_STRUCT)`
- Push/pop scope with automatic restore of shadowed values
- Useful for compilers, interpreters, template engines, config overlays

### ASCII Operations (`ascii_mask.h`)
- 128-bit character set bitmaps for ASCII 0-127
- O(1) character class checking (digits, hex, alphanumeric, whitespace)
- Predefined masks: `ASCII_MASK_DIGITS`, `ASCII_MASK_HEX`, `ASCII_MASK_IDENTIFIER`, etc.
- Efficient range operations for character classes

### String Conversion (`strconv.h`, `bufconv.h`)
- Fast integer parsing: `strconv_u32()`, `bufconv_i64()`, etc.
- Floating-point parsing: `strconv_f32_signed()`, `bufconv_f64_unsigned()`
- `strconv_*`: Null-terminated strings
- `bufconv_*`: Bounded buffers (pairs with `str_view`)
- Zero-allocation, inline functions
- No overflow checking (caller responsibility for bounded inputs)

### Memory Mapping (`mmap.h`)
- Cross-platform file memory mapping (POSIX mmap / Windows CreateFileMapping)
- Read-only access for zero-copy file loading
- Automatic cleanup on close

### Regular Expressions (`regex/regex_expr.h`)
- Structure of Arrays (SoA) regex representation
- Lightweight 2-byte tokens for instruction cache efficiency
- Separate hot stream (control flow) from cold data (literals/masks)
- Designed for route matching and pattern parsing

### Vector Math (`math_types.h`)
- Macro-generated vector types: `vec2f`, `vec3f`, `vec4f`, `vec2d`, `vec3d`, `vec4d`
- Component-wise operations: add, subtract, multiply, divide
- Scalar operations: `vec3f_muls()`, `vec3f_adds()`
- Dot product and sum operations
- Multiple accessor patterns: `.x`/`.y`/`.z`, `.r`/`.g`/`.b`, `.s`/`.t`/`.p`
- Pass-by-value for register optimization
- Zero-cost abstractions via inline functions and macros

## Usage Patterns

### Pattern 1: Vendored (Copy-Paste)
Copy files directly into your project:

```bash
# Copy libcommon into your project
cp -r libcommon/include/common your-project/include/
cp -r libcommon/src/common your-project/src/

# In your Makefile
CFLAGS := -Iinclude
```

```c
#include <common/types.h>
#include <common/ascii_mask.h>
```

### Pattern 2: Git Submodule
Add libcommon as a submodule:

```bash
git submodule add <libcommon-repo-url> external/libcommon
git submodule update --init --recursive

# In your Makefile
CFLAGS := -Iexternal/libcommon/include
```

```c
#include <common/types.h>
#include <common/regex/regex_expr.h>
```

### Pattern 3: System Installation
Install libcommon globally:

```bash
cd libcommon
make
sudo make install  # Installs to /usr/local by default

# In your Makefile
CFLAGS := -I/usr/local/include
LDFLAGS := -L/usr/local/lib -lcommon
```

```c
#include <common/types.h>
#include <common/mmap.h>
```

## Building

**Prerequisites**: GCC compiler

### Static Library (Default)
```bash
make                # Builds libcommon.a
```

### Shared Library
```bash
make shared         # Linux: libcommon.so, Windows: libcommon.dll
```

### Installation
```bash
sudo make install           # Installs to /usr/local
sudo make install PREFIX=/opt/libcommon  # Custom prefix
```

### Uninstallation
```bash
sudo make uninstall
```

### Cleaning
```bash
make clean          # Remove build artifacts
```

## Code Examples

### Type Aliases and Zero-Copy Strings
```c
#include <common/types.h>
#include <stdio.h>

void process_data(str_view input) {
    // Zero-copy string view - no allocation
    printf("Processing %zu bytes\n", input.len);
    
    u32 value = 0;
    // Inline string parsing
    for (size_t i = 0; i < input.len; i++) {
        u8 c = (u8)(input.ptr[i] - '0');
        if (c > 9) break;
        value = (value * 10) + c;
    }
}

int main(void) {
    const char* text = "12345";
    str_view view = { .ptr = text, .len = 5 };
    process_data(view);
    return 0;
}
```

### Dynamic Buffer
```c
#include <common/buf.h>
#include <common/types.h>
#include <stdio.h>

DEFINE_BUFFER(i32, int_buf)

int main(void) {
    int_buf buf;
    int_buf_init(&buf, 16);
    
    // Amortized O(1) push
    for (i32 i = 0; i < 100; i++) {
        int_buf_push(&buf, i * i);
    }
    
    printf("Count: %zu, Capacity: %zu\n", buf.count, buf.cap);
    
    int_buf_free(&buf);
    return 0;
}
```

### Hash Map
```c
#include <common/map.h>
#include <common/types.h>
#include <stdio.h>

typedef struct user {
    i32 id;
    i32 age;
} user;

DEFINE_MAP(user, user_map)

int main(void) {
    user_map users;
    user_map_init(&users, 64);
    
    user_map_set(&users, (str_view){"alice", 5}, (user){1, 30});
    user_map_set(&users, (str_view){"bob", 3}, (user){2, 25});
    
    user* found = user_map_get(&users, (str_view){"alice", 5});
    if (found) {
        printf("Found alice: id=%d, age=%d\n", found->id, found->age);
    }
    
    user_map_free(&users);
    return 0;
}
```

### Symbol Table with Scoping
```c
#include <common/symbol_table.h>
#include <common/types.h>
#include <stdio.h>

typedef struct var_def {
    i32 value;
} var_def;

DEFINE_SYMBOL_TABLE(var_def, var_table)

int main(void) {
    var_table vars;
    var_table_init(&vars);
    
    // Global scope
    var_table_insert(&vars, (str_view){"x", 1}, (var_def){10});
    
    var_table_push_scope(&vars);
    {
        // Inner scope shadows x
        var_table_insert(&vars, (str_view){"x", 1}, (var_def){20});
        
        var_def* x = var_table_lookup(&vars, (str_view){"x", 1});
        printf("Inner x = %d\n", x->value);  // Prints 20
    }
    var_table_pop_scope(&vars);
    
    var_def* x = var_table_lookup(&vars, (str_view){"x", 1});
    printf("Outer x = %d\n", x->value);  // Prints 10
    
    var_table_free(&vars);
    return 0;
}
```

### ASCII Character Classes
```c
#include <common/ascii_mask.h>
#include <stdio.h>

int is_valid_identifier(str_view name) {
    // Fast character class checking - O(1) per character
    ascii_mask valid = ASCII_MASK_IDENTIFIER; // [0-9a-zA-Z_]
    
    for (size_t i = 0; i < name.len; i++) {
        if (!ascii_mask_check(&valid, (u8)name.ptr[i])) {
            return 0; // Invalid character
        }
    }
    return 1;
}

int main(void) {
    str_view valid = { "user_id123", 10 };
    str_view invalid = { "user-id!", 8 };
    
    printf("'user_id123' valid: %d\n", is_valid_identifier(valid));
    printf("'user-id!' valid: %d\n", is_valid_identifier(invalid));
    return 0;
}
```

### String Conversion
```c
#include <common/strconv.h>
#include <common/bufconv.h>
#include <stdio.h>

int main(void) {
    // Null-terminated string parsing
    const char* num_str = "42";
    u32 value;
    if (strconv_u32(num_str, &value) == STRCONV_RESULT_SUCCESS) {
        printf("Parsed: %u\n", value);
    }
    
    // Bounded buffer parsing (pairs with str_view)
    str_view bounded = { "12345", 3 };  // Only parse "123"
    u32 partial;
    if (bufconv_u32(bounded.ptr, bounded.len, &partial) == BUFCONV_RESULT_SUCCESS) {
        printf("Parsed bounded: %u\n", partial);
    }
    
    // Float parsing
    const char* float_str = "-3.14159";
    f32 pi;
    if (strconv_f32_signed(float_str, &pi) == STRCONV_RESULT_SUCCESS) {
        printf("Parsed: %f\n", pi);
    }
    
    return 0;
}
```

### Memory-Mapped File Loading
```c
#include <common/mmap.h>
#include <common/types.h>
#include <stdio.h>

int main(void) {
    data_buf file;
    
    int result = mmap_open("config.txt", &file);
    if (result == 0) {
        printf("Loaded %zu bytes\n", file.size);
        
        // Zero-copy access to file contents
        const char* contents = (const char*)file.data;
        // ... process contents ...
        
        mmap_close(&file);
    }
    
    return 0;
}
```

### Regex Lexer for Route Matching
```c
#include <common/regex/regex_expr.h>
#include <common/types.h>
#include <stdio.h>

int main(void) {
    str_view pattern = { "/users/[0-9]+", 13 };
    regex_expr expr;
    
    if (regex_expr_lex(pattern, &expr) == 0) {
        printf("Lexed %zu tokens\n", expr.count);
        printf("Captures: %zu\n", expr.capture_count);
        
        // Access hot stream (tokens) separately from cold data
        for (size_t i = 0; i < expr.count; i++) {
            printf("Token %zu: type=%d\n", i, expr.tokens[i].type);
        }
        
        regex_expr_free(&expr);
    }
    
    return 0;
}
```

### Vector Math Operations
```c
#include <common/math_types.h>
#include <stdio.h>

int main(void) {
    // Multiple accessor patterns: .x/.y/.z, .r/.g/.b, .s/.t/.p
    vec3f position = { .x = 1.0f, .y = 2.0f, .z = 3.0f };
    vec3f velocity = { .x = 0.5f, .y = -1.0f, .z = 0.0f };
    
    // Component-wise operations (passed by value for register optimization)
    vec3f new_position = vec3f_add(position, velocity);
    printf("New position: (%f, %f, %f)\n", new_position.x, new_position.y, new_position.z);
    
    // Scalar operations
    vec3f scaled = vec3f_muls(velocity, 2.0f);
    printf("Scaled velocity: (%f, %f, %f)\n", scaled.x, scaled.y, scaled.z);
    
    // Dot product
    float dot = vec3f_dot(position, velocity);
    printf("Dot product: %f\n", dot);
    
    // Color accessor pattern
    vec3f color = { .r = 1.0f, .g = 0.5f, .b = 0.0f };
    printf("Color RGB: (%f, %f, %f)\n", color.r, color.g, color.b);
    
    return 0;
}
```

## Design Principles

### Performance-First
- **Inline critical functions**: Marked `static inline __attribute__((always_inline))`
- **Pass small structs by value**: Register optimization for `str_view`, configs
- **Structure of Arrays**: Hot/cold data separation (see `regex_expr`)
- **Cache-friendly layouts**: 64-byte alignment considerations, minimal padding

### Data-Oriented Design
- Separate control flow from data (SoA patterns)
- Batch operations where possible
- Minimize pointer chasing

### Zero-Cost Abstractions
- `str_view` instead of string copies
- Macros for code generation (`DEFINE_BUFFER`, `DEFINE_MAP`, `DEFINE_SYMBOL_TABLE`)
- Compile-time decisions via inlining

### Const Correctness
- Liberal use of `const` and `restrict` keywords
- Prevents unintended mutations
- Enables compiler optimizations

## System Requirements

- **OS**: Windows (10/11) or Linux (Modern distributions)
- **Architecture**: x86_64, ARM64, RISC-V, or any **Little Endian** platform
  - **Note**: This library requires Little Endian byte ordering for certain bit manipulation patterns and potential future SWAR optimizations. Big Endian systems are not supported.
- **Compiler**: GCC 7+ (primary toolchain)

## File Structure

```
libcommon/
├── include/
│   └── common/
│       ├── types.h           # Core type definitions
│       ├── compiler.h        # Compiler detection and attributes
│       ├── hash.h            # FNV-1a hashing, string comparison
│       ├── buf.h             # Generic dynamic buffer
│       ├── map.h             # Generic hash map
│       ├── symbol_table.h    # Scoped hash map
│       ├── ascii_mask.h      # ASCII character sets
│       ├── strconv.h         # String conversion (null-terminated)
│       ├── bufconv.h         # Buffer conversion (bounded)
│       ├── mmap.h            # Memory-mapped files
│       ├── math_types.h      # Vector math types
│       ├── internal/
│       │   └── conv_impl.h   # Shared conversion implementation
│       ├── formats/
│       │   └── json_read.h   # JSON parsing
│       └── regex/
│           └── regex_expr.h  # Regex lexer
├── src/
│   └── common/
│       ├── mmap.c            # Cross-platform mmap implementation
│       └── regex/
│           └── regex_expr.c  # Regex lexer implementation
├── Makefile
└── README.md
```

## Contributing

When adding new utilities:
1. Follow existing naming conventions (lowercase, underscores)
2. Use `COMMON_*_H` header guards
3. Inline performance-critical code
4. Document with Doxygen-style comments
5. Test on Windows and Linux

## License

TBD

## Related Projects

- **GatorStation**: PlayStation 1 emulator using libcommon
- **c-webstack**: High-performance C web backend framework
