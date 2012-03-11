# 
# src/parser/Makefile
#

ROOT := ../..
include $(ROOT)/Common.mk

OBJECTS = parser.o lexer.o st_node.o
OBJS    = $(addprefix $(BUILD_DIR)/,$(OBJECTS))

all: parser 

parser: main.c $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/parser \
	  $(OBJS) main.c $(LD_FLAGS)

parser.c: parser.y
	yacc -d -o parser.c parser.y

lexer.c: lexer.l
	lex -o lexer.c lexer.l

$(INCLUDE_DIR)/parser.h: parser.c
	mv parser.h $(INCLUDE_DIR)

$(INCLUDE_DIR)/lexer.h:
	touch $(INCLUDE_DIR)/lexer.h

$(BUILD_DIR)/%.o: %.c $(INCLUDE_DIR)/%.h
	$(CC) $(CFLAGS) -I. -o $(BUILD_DIR)/$*.o -c $*.c

include $(ROOT)/Rules.mk
