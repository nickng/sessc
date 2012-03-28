ROOT := ..
include $(ROOT)/Common.mk

LDFLAGS += -lcunit

tests: test_parser

test_parser: test_parser.c
	$(CC) $(CFLAGS) -o $(BIN_DIR)/test_parser \
		$(BUILD_DIR)/parser.o \
		$(BUILD_DIR)/lexer.o \
		$(BUILD_DIR)/st_node.o \
		test_parser.c \
		$(LDFLAGS)

include $(ROOT)/Rules.mk
