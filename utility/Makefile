TARGET=ergebnis
LDFLAGS=-s

start:
	-@rm $(TARGET) >&/dev/null
ifndef ZAHL
	@echo USAGE:
	@echo 'make ZAHL=<toCalc>'
	@false
endif
	- make ZAHL=$(ZAHL) recurseBin
	@echo
	cat $(TARGET)
	@echo
	-@rm $(TARGET) >&/dev/null

recurseBin:
	- perl -e 'if ($(ZAHL) < 2) { exit 1; }' && make ZAHL=`perl -e 'if ($(ZAHL) >= 1) { print $(ZAHL) / 2} else { exit 1; }'` recurseBin
	perl -e 'print $(ZAHL) % 2' >> $(TARGET)
	perl -e 'if ($(ZAHL) < 2) { exit 1; }'

mailtest:
	make psycmail
	cp psycmail /usr/depot/mbin/psycmail
	rm psycmail
	echo "$@"|elm lynx -s "$@"

