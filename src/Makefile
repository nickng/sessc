ROOT := ..
include $(ROOT)/Common.mk

all:
	$(MAKE) --directory=common
	$(MAKE) --directory=parser
	$(MAKE) --directory=scribble
	$(MAKE) --directory=connmgr
	$(MAKE) --directory=runtime

include $(ROOT)/Rules.mk
