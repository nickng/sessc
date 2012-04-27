# Global rules
#

.PHONY: all clean clean-all

$(BUILD_DIR)/%.o: %.c $(INCLUDE_DIR)/%.h
	$(CC) $(CFLAGS) -c $*.c \
	  -o $(BUILD_DIR)/$*.o

clean:
	$(RM) -r $(BUILD_DIR)/*
	$(RM) -r $(BIN_DIR)/*

clean-docs:
	$(RM) -r $(DOCS_DIR)/*

clean-all: clean
	$(RM) $(LIB_DIR)/lib*.a
