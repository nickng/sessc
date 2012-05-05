# 
# src/connmgr/Makefile
#

ROOT := ../..
include $(ROOT)/Common.mk

connmgr: main.c $(BUILD_DIR)/connmgr.o $(BUILD_DIR)/st_node.o $(BUILD_DIR)/parser.o $(BUILD_DIR)/lexer.o
	$(CC) $(CFLAGS) -o $(BIN_DIR)/connmgr main.c \
		$(BUILD_DIR)/connmgr.o \
		$(BUILD_DIR)/st_node.o \
		$(BUILD_DIR)/parser.o \
		$(BUILD_DIR)/lexer.o

include $(ROOT)/Rules.mk
