# 
# src/scribble/Makefile
#

ROOT := ../..
include $(ROOT)/Common.mk

OBJECTS = canonicalise.o print.o project.o parser.o lexer.o st_node.o
OBJS    = $(addprefix $(BUILD_DIR)/,$(OBJECTS))

all: project-tool scribble-tool

project-tool: project-tool.c $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/project \
	  $(OBJS) project-tool.c $(LD_FLAGS)

scribble-tool: scribble-tool.c $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/scribble-tool \
	  $(OBJS) scribble-tool.c $(LD_FLAGS)

# Override runtime build rules (header files under scribble/)
$(BUILD_DIR)/%.o: %.c $(INCLUDE_DIR)/scribble/%.h
	$(CC) $(CFLAGS) -c $*.c \
	  -o $(BUILD_DIR)/$*.o

include $(ROOT)/Rules.mk
