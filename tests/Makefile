ROOT := ..
include $(ROOT)/Common.mk

LDFLAGS += -lcunit

tests: test_normalisation test_parser

test_parser: test_parser.c
	$(CC) $(CFLAGS) -o $(BIN_DIR)/test_parser \
		$(BUILD_DIR)/parser.o \
		$(BUILD_DIR)/lexer.o \
		$(BUILD_DIR)/st_node.o \
		test_parser.c \
		$(LDFLAGS)

test_normalisation: test_normalisation.c
	$(CC) $(CFLAGS) -o $(BIN_DIR)/test_normalisation \
		$(BUILD_DIR)/parser.o \
		$(BUILD_DIR)/lexer.o \
		$(BUILD_DIR)/st_node.o \
		$(BUILD_DIR)/canonicalise.o \
		test_normalisation.c \
		$(LDFLAGS)

include $(ROOT)/Rules.mk
