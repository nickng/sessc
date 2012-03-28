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

CC      := gcc
MPICC   := mpicc
CFLAGS  := -Wall -I$(INCLUDE_DIR) # -m64 -fPIC
LDFLAGS := -L$(LIB_DIR) # -lsess -lzmq

# Other options

AR      := ar
ARFLAGS := -cvq

# Other tools

CP      := cp
MAKE    := make
MKDIR   := mkdir -p
DOXYGEN := doxygen
