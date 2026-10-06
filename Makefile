# OpenOrbis SDL2 homebrew build. Set OO_PS4_TOOLCHAIN to a release/toolchain root.
TITLE := Poo Poo on the Toilet
VERSION := 01.00
TITLE_ID := PPOO00001
CONTENT_ID := IV0000-PPOO00001_00-POOPOOTOILET0001

TOOLCHAIN := $(OO_PS4_TOOLCHAIN)
TARGET := PooPooOnTheToilet
BUILD_DIR := build
PKG_DIR := pkg
SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

LIBS := -lc -lkernel -lc++ -lSceUserService -lSceVideoOut -lSceAudioOut -lScePad -lSceSysmodule -lSceFreeType -lSDL2
CFLAGS := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c -O2 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include
CXXFLAGS := $(CFLAGS) -std=c++14 -isystem $(TOOLCHAIN)/include/c++/v1
LDFLAGS := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
CCX := clang++
LD := ld.lld
PLATFORM_DIR := linux
endif
ifeq ($(UNAME_S),Darwin)
CCX := /usr/local/opt/llvm/bin/clang++
LD := /usr/local/opt/llvm/bin/ld.lld
PLATFORM_DIR := macos
endif

.PHONY: all clean package check-toolchain
all: package

check-toolchain:
	@test -n "$(TOOLCHAIN)" || (echo "OO_PS4_TOOLCHAIN is not set"; exit 1)
	@test -x "$(TOOLCHAIN)/bin/$(PLATFORM_DIR)/create-eboot" || (echo "OpenOrbis Linux/macOS tools not found"; exit 1)

package: $(CONTENT_ID).pkg

$(CONTENT_ID).pkg: $(PKG_DIR)/pkg.gp4
	$(TOOLCHAIN)/bin/$(PLATFORM_DIR)/PkgTool.Core pkg_build $< .

$(PKG_DIR)/pkg.gp4: $(PKG_DIR)/eboot.bin $(PKG_DIR)/sce_sys/param.sfo
	cd $(PKG_DIR) && $(TOOLCHAIN)/bin/$(PLATFORM_DIR)/create-gp4 -out pkg.gp4 --content-id=$(CONTENT_ID) --files "eboot.bin sce_sys/param.sfo"

$(PKG_DIR)/sce_sys/param.sfo: Makefile | check-toolchain
	@mkdir -p $(PKG_DIR)/sce_sys
	$(TOOLCHAIN)/bin/$(PLATFORM_DIR)/PkgTool.Core sfo_new $@
	$(TOOLCHAIN)/bin/$(PLATFORM_DIR)/PkgTool.Core sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(TOOLCHAIN)/bin/$(PLATFORM_DIR)/PkgTool.Core sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(TOOLCHAIN)/bin/$(PLATFORM_DIR)/PkgTool.Core sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(PLATFORM_DIR)/PkgTool.Core sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(TOOLCHAIN)/bin/$(PLATFORM_DIR)/PkgTool.Core sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(TOOLCHAIN)/bin/$(PLATFORM_DIR)/PkgTool.Core sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(PLATFORM_DIR)/PkgTool.Core sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(PLATFORM_DIR)/PkgTool.Core sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(TOOLCHAIN)/bin/$(PLATFORM_DIR)/PkgTool.Core sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(TOOLCHAIN)/bin/$(PLATFORM_DIR)/PkgTool.Core sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

$(PKG_DIR)/eboot.bin: $(OBJECTS) | check-toolchain
	@mkdir -p $(PKG_DIR)
	$(LD) $(OBJECTS) -o $(BUILD_DIR)/$(TARGET).elf $(LDFLAGS)
	$(TOOLCHAIN)/bin/$(PLATFORM_DIR)/create-eboot -in=$(BUILD_DIR)/$(TARGET).elf -out=$@ --paid 0x3800000000000011

$(BUILD_DIR)/%.o: src/%.cpp | check-toolchain
	@mkdir -p $(BUILD_DIR)
	$(CCX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(OBJECTS) $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).oelf $(PKG_DIR)/eboot.bin $(PKG_DIR)/pkg.gp4 $(PKG_DIR)/sce_sys/param.sfo $(CONTENT_ID).pkg
