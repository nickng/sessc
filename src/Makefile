ROOT := ..
include $(ROOT)/Common.mk

all:
	$(MAKE) --directory=common
	$(MAKE) --directory=parser
	$(MAKE) --directory=connmgr
	$(MAKE) --directory=runtime
	$(MAKE) --directory=scribble

include $(ROOT)/Rules.mk
