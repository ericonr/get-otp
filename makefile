EXE = cbc-file
LDLIBS = -lbearssl -largon2
OPT = -O2
CFLAGS = -std=c99 $(OPT) -Wall -Wextra -Werror=implicit -pedantic

SCRIPT = get-otp

all: $(EXE) $(SCRIPT)

$(SCRIPT): $(SCRIPT).in
	sed -e 's|@PREFIX@|$(PREFIX)|' $^ > $@

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(PREFIX)/libexec
	install -Dm755 cbc-file $(DESTDIR)$(PREFIX)/libexec/
	install -Dm755 get-otp $(DESTDIR)$(PREFIX)/bin/
