CC := gcc
MODULES := common common/regex
SRC_DIRS := $(MODULES:%=src/%)
CFLAGS := -Wall -Wextra -Iinclude -O2 -std=c11
AR := ar

ifeq ($(OS),Windows_NT)
    EXT := .dll
    LIB_PREFIX := lib
    STATIC_LIB_EXT := .a
    IMPORT_LIB_EXT := .lib
    
    RM = del /Q
    RMDIR = rmdir /Q /S
    FIXPATH = $(subst /,\,$1)
    MKDIR_P = if not exist $(call FIXPATH,$1) mkdir $(call FIXPATH,$1)
    
    SHARED_FLAGS := -shared -Wl,--out-implib,libcommon$(IMPORT_LIB_EXT)
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        EXT := .dylib
        SHARED_FLAGS := -dynamiclib -install_name @rpath/libcommon$(EXT)
    else
        EXT := .so
        SHARED_FLAGS := -shared -fPIC
    endif
    
    LIB_PREFIX := lib
    LIB_EXT :=
    STATIC_LIB_EXT := .a
    
    RM = rm -f
    RMDIR = rm -rf
    FIXPATH = $1
    MKDIR_P = mkdir -p $1
endif

BUILD_DIR := build
PREFIX ?= /usr/local

# Recursive wildcard function
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRCS := $(call rwildcard,$(SRC_DIRS),*.c)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

TARGET_STATIC := $(LIB_PREFIX)common$(STATIC_LIB_EXT)
TARGET_SHARED := $(LIB_PREFIX)common$(EXT)

# Default: Build static library
all: $(TARGET_STATIC)

# Static library
$(TARGET_STATIC): $(OBJS)
	$(AR) rcs $@ $^

# Shared library
shared: CFLAGS += -fPIC
shared: $(TARGET_SHARED)

$(TARGET_SHARED): $(OBJS)
	$(CC) $(SHARED_FLAGS) -o $@ $^ $(LDFLAGS)

# Compile object files
$(BUILD_DIR)/%.c.o: %.c
	$(call MKDIR_P,$(dir $@))
	$(CC) $(CFLAGS) -c $< -o $@

# Install targets
install: $(TARGET_STATIC)
	$(call MKDIR_P,$(PREFIX)/lib)
	$(call MKDIR_P,$(PREFIX)/include)
	cp -r include/common $(PREFIX)/include/
ifeq ($(OS),Windows_NT)
	copy /Y $(call FIXPATH,$(TARGET_STATIC)) $(call FIXPATH,$(PREFIX)/lib/)
else
	cp $(TARGET_STATIC) $(PREFIX)/lib/
endif
	@echo "Installed to $(PREFIX)"

install-shared: shared
	$(call MKDIR_P,$(PREFIX)/lib)
	$(call MKDIR_P,$(PREFIX)/include)
	cp -r include/common $(PREFIX)/include/
ifeq ($(OS),Windows_NT)
	copy /Y $(call FIXPATH,$(TARGET_SHARED)) $(call FIXPATH,$(PREFIX)/lib/)
	copy /Y $(call FIXPATH,$(LIB_PREFIX)common$(LIB_EXT)) $(call FIXPATH,$(PREFIX)/lib/)
else
	cp $(TARGET_SHARED) $(PREFIX)/lib/
	ifeq ($(UNAME_S),Linux)
		ldconfig || true
	endif
endif
	@echo "Installed shared library to $(PREFIX)"

uninstall:
	$(RM) $(call FIXPATH,$(PREFIX)/lib/$(TARGET_STATIC))
	$(RM) $(call FIXPATH,$(PREFIX)/lib/$(TARGET_SHARED))
ifeq ($(OS),Windows_NT)
	$(RM) $(call FIXPATH,$(PREFIX)/lib/$(LIB_PREFIX)common$(LIB_EXT))
	$(RMDIR) $(call FIXPATH,$(PREFIX)/include/common)
else
	$(RMDIR) $(PREFIX)/include/common
endif
	@echo "Uninstalled from $(PREFIX)"

clean:
ifeq ($(OS),Windows_NT)
	@if exist $(call FIXPATH,$(BUILD_DIR)) $(RMDIR) $(call FIXPATH,$(BUILD_DIR))
	@if exist $(call FIXPATH,$(TARGET_STATIC)) $(RM) $(call FIXPATH,$(TARGET_STATIC))
	@if exist $(call FIXPATH,$(TARGET_SHARED)) $(RM) $(call FIXPATH,$(TARGET_SHARED))
	@if exist $(call FIXPATH,$(LIB_PREFIX)common$(LIB_EXT)) $(RM) $(call FIXPATH,$(LIB_PREFIX)common$(LIB_EXT))
else
	$(RMDIR) $(BUILD_DIR)
	$(RM) $(TARGET_STATIC) $(TARGET_SHARED)
endif

.PHONY: all shared install install-shared uninstall clean