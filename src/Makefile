INCLUDE_PATH	= ../include
HEADER_SUBDIR	= nexrad

CC		= cc
CFLAGS		= -g -Wno-unused-result -fno-inline -fPIC -Wall -O2 -I$(INCLUDE_PATH)
LDFLAGS		=
LLFLAGS		= -shared -Wl,-soname=$(SONAME)

HEADERS		= message.h chunk.h symbology.h graphic.h tabular.h packet.h radial.h \
		  raster.h date.h error.h

OBJS		= message.o chunk.o symbology.o graphic.o tabular.o packet.o radial.o \
		  raster.o date.o error.o

VERSION_MAJOR	= 0
VERSION_MINOR	= 0.0
VERSION		= $(VERSION_MAJOR).$(VERSION_MINOR)

SONAME_SHORT	= libnexrad.so
SONAME		= $(SONAME_SHORT).$(VERSION_MAJOR)
SONAME_FULL	= $(SONAME_SHORT).$(VERSION)

PREFIX		= /usr/local
INSTALL_LIB	= $(PREFIX)/lib
INSTALL_INCLUDE	= $(PREFIX)/include

RM		= /bin/rm
LN		= /bin/ln
INSTALL		= /usr/bin/install

all: $(SONAME_FULL)

$(SONAME_FULL): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LLFLAGS) -o $(SONAME_FULL)

$(OBJS): %.o: %.c $(addprefix $(INCLUDE_PATH)/$(HEADER_SUBDIR)/, $(HEADERS))
	$(CC) $(CFLAGS) -c $<

install: all
	$(INSTALL) -d -m 0755 $(INSTALL_INCLUDE)/nexrad
	$(INSTALL) -c -m 0644 $(addprefix $(INCLUDE_PATH)/$(HEADER_SUBDIR)/, $(HEADERS)) $(INSTALL_INCLUDE)/$(HEADER_SUBDIR)
	$(INSTALL) -c -m 0755 $(SONAME_FULL) $(INSTALL_LIB)
	$(LN) -f -s $(SONAME_FULL) $(INSTALL_LIB)/$(SONAME)
	$(LN) -f -s $(SONAME_FULL) $(INSTALL_LIB)/$(SONAME_SHORT)

uninstall:
	$(RM) -f $(INSTALL_LIB)/$(SONAME_FULL)
	$(RM) -f $(INSTALL_LIB)/$(SONAME)
	$(RM) -f $(INSTALL_LIB)/$(SONAME_SHORT)
	$(RM) -f $(addprefix $(INSTALL_INCLUDE)/$(HEADER_SUBDIR)/, $(HEADERS))

clean:
	$(RM) -f $(SONAME_FULL) $(OBJS)
