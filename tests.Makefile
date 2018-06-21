# makefile for running tests, see README

ifdef VALGRIND
VALGRINDOPTS ?= -q --leak-check=full --error-exitcode=1
endif


.PHONY: all
all: ctests $(wildcard ötests/test_*.ö examples/*.ö)
	@echo ok

executables:
	make ö ctestsrunner


ctests: executables
	./ctestsrunner

# http://clarkgrubb.com/makefile-style-guide#phony-target-arg
ötests/test_%.ö: executables FORCE
	$(VALGRIND) $(VALGRINDOPTS) ./ö $@

examples/%.ö: executables FORCE
	bash -c 'diff <($(VALGRIND) $(VALGRINDOPTS) ./ö $@) examples/output/$*.txt'

FORCE:
