ROOT := ../..
include $(ROOT)/Common.mk

OBJS := $(BUILD_DIR)/session.o $(BUILD_DIR)/primitives.o $(BUILD_DIR)/utils.o $(BUILD_DIR)/parser.o $(BUILD_DIR)/lexer.o $(BUILD_DIR)/st_node.o $(BUILD_DIR)/connmgr.o
LDFLAGS += -lzmq

all: $(OBJS) $(BUILD_DIR)/libsc.a

# Override runtime build rules (header files under sc/)
$(BUILD_DIR)/%.o: %.c $(INCLUDE_DIR)/sc/%.h
	$(CC) $(CFLAGS) -c $*.c \
	  -o $(BUILD_DIR)/$*.o

$(BUILD_DIR)/%.a: $(OBJS)
	$(RM) $(BUILD_DIR)/$*.a $(LIB_DIR)/$*.a
	$(AR) $(ARFLAGS) $@ $(OBJS)
	$(CP) -v $@ $(LIB_DIR)/$*.a

include $(ROOT)/Rules.mk
