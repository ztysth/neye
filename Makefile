CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LDFLAGS =

SRCDIR = src
CONFIGDIR = config
DOCDIR = doc
BINDIR = bin
PREFIX = /usr/local

TARGET = neye
SOURCES = $(SRCDIR)/main.c $(SRCDIR)/ini.c $(SRCDIR)/config.c $(SRCDIR)/utils.c $(SRCDIR)/monitor.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean install uninstall help

all: $(BINDIR)/$(TARGET)

$(BINDIR):
	mkdir -p $(BINDIR)

$(BINDIR)/$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -I$(SRCDIR) -c $< -o $@

install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(BINDIR)/$(TARGET) $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)$(PREFIX)/share/neye/config
	install -m 644 $(CONFIGDIR)/default.ini $(DESTDIR)$(PREFIX)/share/neye/config/
	@echo "Installation complete. Run 'neye --help' for usage."

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	rm -rf $(DESTDIR)$(PREFIX)/share/neye
	@echo "Uninstallation complete."

clean:
	rm -f $(OBJECTS)
	rm -rf $(BINDIR)

help:
	@echo "Available targets:"
	@echo "  all          - Build neye"
	@echo "  clean        - Remove build artifacts"
	@echo "  install      - Install neye to $(PREFIX)/bin"
	@echo "  uninstall    - Remove neye from system"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Note: Please ensure src/ini.h and src/ini.c are present before building"