EXE = cbc-file
LDLIBS = -lbearssl -largon2
OPT = -O2
CFLAGS = -std=c99 $(OPT) -Wall -Wextra -Werror=implicit -pedantic

SCRIPT = get-otp encrypt-otp

all: $(EXE) $(SCRIPT)

get-otp: get-otp.in
encrypt-otp: encrypt-otp.in
%: %.in
	sed -e 's|@PREFIX@|$(PREFIX)|' $^ > $@

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -Dm755 cbc-file $(DESTDIR)$(PREFIX)/bin/
	install -Dm755 get-otp $(DESTDIR)$(PREFIX)/bin/
	install -Dm755 encrypt-otp $(DESTDIR)$(PREFIX)/bin/

clean:
	rm -f $(EXE) $(SCRIPT)
