# Global rules
#

.PHONY: all clean clean-all

clean:
	$(RM) $(BUILD_DIR)/*
	$(RM) -r $(BIN_DIR)/*

clean-docs:
	$(RM) -r $(DOCS_DIR)/*

clean-all: clean
	$(RM) $(LIB_DIR)/lib*.a
