
SUBDIR := $(sort $(wildcard */))
SUBDIR := $(filter-out  sc_record.lv2/, $(SUBDIR))

.PHONY: $(SUBDIR) recurse

$(MAKECMDGOALS) recurse: $(SUBDIR)

$(SUBDIR):
	@exec $(MAKE) -j 1 -C $@ $(MAKECMDGOALS)

doc:
	@rm -rf ./docs
	doxygen Doxyfile
	cp ./examples/index.html ./docs/
