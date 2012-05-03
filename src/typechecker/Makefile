#
# Session Type Checker makefile.
#
# Put under LLVM_ROOT/tools/clang/examples/SessionTypeChecker/
#

SESSCC_PATH      := src/sessc
SESSCC_INC_DIR   := $(SESSCC_PATH)/include
SESSCC_BUILD_DIR := $(SESSCC_PATH)/build

CLANG_LEVEL := ../..
LIBRARYNAME = SessionTypeChecker 

# If we don't need RTTI or EH, there's no reason to export anything
# from the plugin.
ifneq ($(REQUIRES_RTTI), 1)
ifneq ($(REQUIRES_EH), 1)
#EXPORTED_SYMBOL_FILE = $(PROJ_SRC_DIR)/SessionTypeChecker.exports
endif
endif

LINK_LIBS_IN_SHARED = 0
SHARED_LIBRARY = 1

include $(CLANG_LEVEL)/Makefile

CXXFLAGS += -I$(SESSCC_INC_DIR) $(SESSCC_BUILD_DIR)/st_node.o $(SESSCC_BUILD_DIR)/parser.o $(SESSCC_BUILD_DIR)/lexer.o


ifeq ($(OS),Darwin)
  LDFLAGS=-Wl,-undefined,dynamic_lookup
endif
