EXE = cbc-file
LDLIBS = -lbearssl

SCRIPT = get-otp

all: $(EXE) $(SCRIPT)

$(SCRIPT): $(SCRIPT).in
	sed -e 's|@PREFIX@|$(PREFIX)|' $^ > $@

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(PREFIX)/libexec
	install -Dm755 cbc-file $(DESTDIR)$(PREFIX)/libexec/
	install -Dm755 get-otp $(DESTDIR)$(PREFIX)/bin/
