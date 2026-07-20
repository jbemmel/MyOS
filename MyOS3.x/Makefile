SHELL := /bin/bash

BUILD_DIR ?= build/linux
MODE ?= debug

CXX ?= g++
LD ?= ld
AR ?= ar
OBJCOPY ?= objcopy
OBJDUMP ?= objdump
SIZE ?= size
PYTHON ?= python3
GZIP ?= gzip
DOXYGEN ?= doxygen

CORE_SRC := $(shell find code/Core/src -name '*.cpp' -print | sort)
MODULE_SRC := $(shell find code/Atmosphere/src -name '*.cpp' -print | sort)
CORE_OBJ := $(patsubst code/Core/src/%.cpp,$(BUILD_DIR)/obj/core/%.o,$(CORE_SRC))
MODULE_OBJ := $(patsubst code/Atmosphere/src/%.cpp,$(BUILD_DIR)/obj/module/%.o,$(MODULE_SRC))
CORE_DEP := $(CORE_OBJ:.o=.d)
MODULE_DEP := $(MODULE_OBJ:.o=.d)
DOC_INPUT := $(shell find code -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.c' -o -name '*.cpp' \) -print | sort)

COMMON_FLAGS := -m32 -std=gnu++98 -ffreestanding -fno-pie -fno-rtti \
	-fexceptions -fnon-call-exceptions -fno-threadsafe-statics \
	-fno-enforce-eh-specs -fomit-frame-pointer -mregparm=3 -mrtd \
	-falign-functions=8 -mpreferred-stack-boundary=2 -fvisibility=hidden \
	-fzero-initialized-in-bss -fmessage-length=0 -fno-stack-protector \
	-DMYOS -DMYOS_ELF -D__builtin_stdarg_start=__builtin_va_start -MMD -MP
CORE_INCLUDES := -Icode/Core/include -Icode/Core/src
MODULE_INCLUDES := $(CORE_INCLUDES) -Icode/Atmosphere/include -Icode/Atmosphere/src

ifeq ($(MODE),release)
MODE_FLAGS := -O3 -DNDEBUG
else
MODE_FLAGS := -O2 -DDEBUG
endif

CXXFLAGS := $(COMMON_FLAGS) $(MODE_FLAGS)
CORE_CXXFLAGS := $(CXXFLAGS) $(CORE_INCLUDES)
# MyOS's module loader expects the historical leading-underscore ABI.
MODULE_CXXFLAGS := $(CXXFLAGS) -DMODULE -fleading-underscore $(MODULE_INCLUDES)

.PHONY: all clean debug release check tools docs doxygen

all: $(BUILD_DIR)/kernel.bin

debug:
	$(MAKE) MODE=debug all

release:
	$(MAKE) MODE=release BUILD_DIR=build/linux-release all

check: $(CORE_OBJ) $(MODULE_OBJ)

docs: build/doc/html/index.html

# Compatibility alias for the old Ant target name.
doxygen: docs

build/doc/html/index.html: doxyfile $(DOC_INPUT)
	@command -v "$(DOXYGEN)" >/dev/null || { echo "Missing required tool: $(DOXYGEN)" >&2; exit 1; }
	$(DOXYGEN) doxyfile
	@echo "Built $@"

tools:
	@for tool in $(CXX) $(LD) $(AR) $(OBJCOPY) $(OBJDUMP) $(SIZE) $(PYTHON) $(GZIP); do \
		command -v "$$tool" >/dev/null || { echo "Missing required tool: $$tool" >&2; exit 1; }; \
	done

$(BUILD_DIR)/obj/core/%.o: code/Core/src/%.cpp build/gen_src/myosconfig.h
	@mkdir -p "$(@D)"
	$(CXX) $(CORE_CXXFLAGS) -c "$<" -o "$@"

$(BUILD_DIR)/obj/module/%.o: code/Atmosphere/src/%.cpp build/gen_src/myosconfig.h
	@mkdir -p "$(@D)"
	$(CXX) $(MODULE_CXXFLAGS) -c "$<" -o "$@"

$(BUILD_DIR)/libCore.a: $(CORE_OBJ)
	@mkdir -p "$(@D)"
	$(AR) rcs "$@" $^

$(BUILD_DIR)/module.elf: $(MODULE_OBJ)
	@mkdir -p "$(@D)"
	$(LD) -m elf_i386 -r --gc-sections -u _initModule -o "$@" $^

$(BUILD_DIR)/exports.S: $(BUILD_DIR)/libCore.a $(BUILD_DIR)/module.elf buildtools/generate_elf_exports.py
	$(PYTHON) buildtools/generate_elf_exports.py \
		--core "$(BUILD_DIR)/libCore.a" --module "$(BUILD_DIR)/module.elf" --output "$@"

$(BUILD_DIR)/exports.o: $(BUILD_DIR)/exports.S
	$(CXX) -m32 -c "$<" -o "$@"

$(BUILD_DIR)/kernel.elf: $(CORE_OBJ) $(BUILD_DIR)/exports.o buildtools/MyOS_linux.linkscript
	$(LD) -m elf_i386 -T buildtools/MyOS_linux.linkscript \
		--no-undefined --gc-sections --strip-discarded --warn-constructors \
		-o "$@" $(CORE_OBJ) $(BUILD_DIR)/exports.o
	$(SIZE) "$@"
	$(OBJDUMP) -d -C -S -l "$@" > "$(BUILD_DIR)/kernel.asm"

$(BUILD_DIR)/kernel.bin.loader: $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) --only-section=.text.uz -O binary "$<" "$@"

$(BUILD_DIR)/kernel.bin.code: $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) -g -S --remove-section=.text.uz -R .data -R .bss -R .stack -O binary "$<" "$@"

$(BUILD_DIR)/kernel.bin.data: $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) -g -S --only-section=.data -O binary "$<" "$@"

$(BUILD_DIR)/kernel.bin.core.gz: $(BUILD_DIR)/kernel.bin.code $(BUILD_DIR)/kernel.bin.data $(BUILD_DIR)/module.elf
	cat $^ > "$(BUILD_DIR)/kernel.bin.core"
	$(GZIP) -9 -n -c "$(BUILD_DIR)/kernel.bin.core" > "$@"

$(BUILD_DIR)/kernel.bin: code/Core/src/Boot/bootphase2 $(BUILD_DIR)/kernel.bin.loader $(BUILD_DIR)/kernel.bin.core.gz
	cat $^ > "$@"
	@echo "Built $@"

clean:
	@case "$(BUILD_DIR)" in build/linux|build/linux-release) ;; *) echo "Refusing unsafe BUILD_DIR: $(BUILD_DIR)" >&2; exit 1;; esac
	rm -rf -- "$(BUILD_DIR)"

-include $(CORE_DEP) $(MODULE_DEP)
