TESTS=$(sort $(basename $(wildcard generated*.lama)))

LAMAC = lamac
INTERPRETER=../../build/main

.PHONY: check $(TESTS)

check: $(TESTS)

$(TESTS): %: %.lama
	@echo "deep-expressions/$@"
	$(LAMAC) -b $<
	@cat $@.input | LAMA=../runtime ./$(INTERPRETER) $@.bc > $@.log && diff $@.log orig/$@.log

clean:
	rm -f *.bc *.log *.s *~
	find . -maxdepth 1 -type f -not -name '*.*' -not -name 'Makefile' -delete

