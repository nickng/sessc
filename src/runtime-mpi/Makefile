ROOT := ../..
include $(ROOT)/Common.mk

OBJS := $(BUILD_DIR)/session-mpi.o $(BUILD_DIR)/utils.o $(BUILD_DIR)/parser.o $(BUILD_DIR)/lexer.o $(BUILD_DIR)/st_node.o

all: $(OBJS) $(BUILD_DIR)/libsc-mpi.a

# Override runtime build rules (header files under sc/)
$(BUILD_DIR)/%.o: %.c $(INCLUDE_DIR)/sc/%.h
	mpicc $(CFLAGS) -c $*.c \
	  -o $(BUILD_DIR)/$*.o

$(BUILD_DIR)/%.a: $(OBJS)
	$(RM) $(BUILD_DIR)/$*.a $(LIB_DIR)/$*.a
	$(AR) $(ARFLAGS) $@ $(OBJS)
	$(CP) -v $@ $(LIB_DIR)/$*.a

include $(ROOT)/Rules.mk
