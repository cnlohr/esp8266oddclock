include user.cfg
-include esp82xx/common.mf
-include esp82xx/main.mf

% :
	$(warning This is the empty rule. Something went wrong.)
	@true

ifndef TARGET
$(info Modules were not checked out... use git clone --recursive in the future. Pulling now.)
$(shell git submodule update --init --recursive 2>/dev/null)
endif

setbaud : setbaud.c
	gcc -o $@ $^

monitor : setbaud
	stty -F /dev/ttyUSB0 -echo raw
	./setbaud /dev/ttyUSB0 50000
	cat /dev/ttyUSB0

# Example for a custom rule.
# Most of the build is handled in main.mf
.PHONY : showvars
showvars:
	$(foreach v, $(.VARIABLES), $(info $(v) = $($(v))))
	true

