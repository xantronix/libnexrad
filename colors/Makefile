CC		= cc
CFLAGS		= -g -Wno-unused-result -fno-inline -Wall -O2 -I../include
LDFLAGS		= -lnexrad -L../src

GENCLUTS	= gencluts
CLUTS		= reflectivity.clut velocity.clut

PREFIX		= /usr/local
INSTALL_SHARE	= $(PREFIX)/share
INSTALL_NEXRAD	= $(INSTALL_SHARE)/nexrad
INSTALL_COLORS	= $(INSTALL_NEXRAD)/colors

RM		= /bin/rm
MKDIR		= /bin/mkdir
RMDIR		= /bin/rmdir
INSTALL		= /usr/bin/install

all: $(CLUTS)

$(CLUTS): $(GENCLUTS)
	./$(GENCLUTS)

$(GENCLUTS): %: %.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

install: $(CLUTS)
	$(MKDIR) -p -m 0755 $(INSTALL_COLORS)
	$(INSTALL) -c -m 0644 $(CLUTS) $(INSTALL_COLORS)

uninstall:
	$(RM) $(addprefix $(INSTALL_COLORS)/, $(CLUTS))
	$(RMDIR) $(INSTALL_COLORS)
	$(RMDIR) $(INSTALL_NEXRAD) || true

clean:
	$(RM) -f $(GENCLUTS) $(CLUTS)
