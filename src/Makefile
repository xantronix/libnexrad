INCLUDE_PATH	= ../include
HEADER_SUBDIR	= nexrad

CC		= $(CROSS)cc
CFLAGS		= -g -Wno-unused-result -fno-inline -fPIC -Wall -O2 \
		  -I$(INCLUDE_PATH)
LLFLAGS		= -shared -Wl,-soname=$(SONAME)
LDFLAGS		=

HEADERS		= message.h chunk.h product.h symbology.h graphic.h tabular.h packet.h \
		  radial.h raster.h date.h error.h block.h header.h vector.h

HEADERS_PRIVATE	= util.h

OBJS		= message.o chunk.o product.o symbology.o graphic.o tabular.o packet.o \
		  radial.o raster.o date.o error.o util.o

VERSION_MAJOR	= 0
VERSION_MINOR	= 0.0
VERSION		= $(VERSION_MAJOR).$(VERSION_MINOR)

STATIC		= libnexrad.a

SONAME_SHORT	= libnexrad.so
SONAME		= $(SONAME_SHORT).$(VERSION_MAJOR)
SONAME_FULL	= $(SONAME_SHORT).$(VERSION)

PREFIX		= /usr/local
INSTALL_LIB	= $(PREFIX)/lib
INSTALL_INCLUDE	= $(PREFIX)/include

HEADERS_BUILD	= $(addprefix $(INCLUDE_PATH)/$(HEADER_SUBDIR)/, $(HEADERS))
HEADERS_SYSTEM	= $(addprefix $(INSTALL_INCLUDE)/$(HEADER_SUBDIR)/, $(HEADERS))

AR		= $(CROSS)ar
RANLIB		= $(CROSS)ranlib

RM		= /bin/rm
LN		= /bin/ln
RMDIR		= /bin/rmdir
INSTALL		= /usr/bin/install

all: $(STATIC) $(SONAME_FULL)

$(STATIC): $(OBJS)
	$(AR) rc $(STATIC) $(OBJS)
	$(RANLIB) $(STATIC)

$(SONAME_FULL): $(OBJS)
	$(CC) $(LLFLAGS) $(OBJS) $(LDFLAGS) -o $(SONAME_FULL)

$(OBJS): %.o: %.c $(HEADERS_BUILD) $(HEADERS_PRIVATE)
	$(CC) $(CFLAGS) -c $<

install-libdir:
	$(INSTALL) -d -m 0755 $(INSTALL_LIB)

install-static: $(STATIC)
	$(INSTALL) -c -m 0644 $(STATIC) $(INSTALL_LIB)

install-shared: $(SONAME_FULL)
	$(INSTALL) -c -m 0755 $(SONAME_FULL) $(INSTALL_LIB)
	$(LN) -f -s $(SONAME_FULL) $(INSTALL_LIB)/$(SONAME)
	$(LN) -f -s $(SONAME_FULL) $(INSTALL_LIB)/$(SONAME_SHORT)

install-libs: install-static install-shared

install-headers: $(HEADERS_BUILD)
	$(INSTALL) -d -m 0755 $(INSTALL_INCLUDE)/$(HEADER_SUBDIR)
	$(INSTALL) -c -m 0644 $(HEADERS_BUILD) $(INSTALL_INCLUDE)/$(HEADER_SUBDIR)

install: install-libdir install-libs install-headers

uninstall-headers:
	$(RM) -f $(HEADERS_SYSTEM)
	$(RMDIR) $(INSTALL_INCLUDE)/$(HEADER_SUBDIR)

uninstall-shared:
	$(RM) -f $(INSTALL_LIB)/$(SONAME_SHORT)
	$(RM) -f $(INSTALL_LIB)/$(SONAME)
	$(RM) -f $(INSTALL_LIB)/$(SONAME_FULL)

uninstall-static:
	$(RM) -f $(INSTALL_LIB)/$(STATIC)

uninstall-libs: uninstall-shared uninstall-static

uninstall: uninstall-headers uninstall-libs

clean:
	$(RM) -f $(SONAME_FULL) $(SONAME) $(SONAME_SHORT) $(OBJS)
