TESTS=$(sort $(basename $(wildcard test*.lama)))
LOGS=$(TESTS:=.log)

LAMAC=lamac
INTERPRETER=../build/main
RM=rm -rf

.PHONY: check $(TESTS)

check: $(TESTS)# expr_tests

$(TESTS): %: %.lama
	@echo "regression/$@"
	$(LAMAC) -b $<
	@cat $@.input | LAMA=../runtime ./$(INTERPRETER) $@.bc > $@.log && diff $@.log orig/$@.log

clean:
	$(RM) *.bc *.s *.i *~ $(LOGS) $(TESTS) *.run *.sm
	make -C expressions clean
	make -C deep-expressions clean