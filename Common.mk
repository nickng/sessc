#
# Common.mk
# Common make configuration options for sessc
#

INCLUDE_DIR := $(ROOT)/include
SRC_DIR     := $(ROOT)/src
LIB_DIR     := $(ROOT)/lib
DOCS_DIR    := $(ROOT)/docs
BUILD_DIR   := $(ROOT)/build
BIN_DIR     := $(ROOT)/bin

# Compile options

DEBUG   := -g -D__DEBUG__ 
RELEASE := -O3
PROFILE := -g -pg

CC      := clang
MPICC   := mpicc.openmpi
CFLAGS  := -Wall -I$(INCLUDE_DIR)
LDFLAGS := -L$(LIB_DIR) -L/usr/include/mpi -lsc-mpi -lmpi -lm

# TARGET := debug

ifneq (,$(findstring debug,$(TARGET)))
	CFLAGS += $(DEBUG)
else ifneq (,$(findstring prof,$(TARGET)))
	CFLAGS += $(PROFILE)
else
	CFLAGS += $(RELEASE)
endif

# Other options

AR      := ar
ARFLAGS := -cvq

# Other tools

CP      := cp
MAKE    := make
MKDIR   := mkdir -p
DOXYGEN := doxygen
