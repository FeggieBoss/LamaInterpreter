TESTS=$(sort $(basename $(wildcard *.lama)))

LAMAC=lamac
INTERPRETER=../build/main

.PHONY: check $(TESTS)

check: $(TESTS)

$(TESTS): %: %.lama
	$(LAMAC) -b $<
	LAMA=../runtime && `which time` -f "LAMAC: $@\t%U" sh -c 'echo 0 | $(LAMAC) -s $<'
	LAMA=../runtime && `which time` -f "MY_INTERPRETER: $@\t%U" sh -c './$(INTERPRETER) $@.bc'

clean:
	$(RM) *.bc test*.log *.s *~ $(TESTS) *.i
