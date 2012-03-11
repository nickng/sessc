# 
# src/common/Makefile
#

ROOT := ../..
include $(ROOT)/Common.mk

all: $(BUILD_DIR)/st_node.o

$(BUILD_DIR)/%.o: %.c $(INCLUDE_DIR)/%.h
	$(CC) $(CFLAGS) -c $*.c \
	  -o $(BUILD_DIR)/$*.o

include $(ROOT)/Rules.mk
