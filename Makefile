#
# Project makefile
#
# Makefiles are organised in a recursive structure
# Each directory has its own Makefile, and should
#  1. Set ROOT to relative path from project root
#  2. Include Common.mk (for compiler flags etc.)
#  3. Write directory-specific rules
#  4. Include Rules.mk (for global rules)
#

ROOT := .
include $(ROOT)/Common.mk

.PHONY: docs

all: scribble-tool runtime runtime-mpi

scribble-tool:
	$(MAKE) --directory=$(SRC_DIR)/common
	$(MAKE) --directory=$(SRC_DIR)/parser
	$(MAKE) --directory=$(SRC_DIR)/scribble

runtime:
	$(MAKE) --directory=$(SRC_DIR)/common
	$(MAKE) --directory=$(SRC_DIR)/parser
	$(MAKE) --directory=$(SRC_DIR)/scribble
	$(MAKE) --directory=$(SRC_DIR)/connmgr
	$(MAKE) --directory=$(SRC_DIR)/runtime

runtime-mpi: runtime
	$(MAKE) --directory=$(SRC_DIR)/runtime-mpi

docs:
	$(DOXYGEN) sessc.doxygen

include $(ROOT)/Rules.mk
