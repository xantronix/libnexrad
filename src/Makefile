INCLUDE	= ../include

CC	= cc
CFLAGS	= -g -Wno-unused-result -fno-inline -Wall -O2 -I$(INCLUDE)
LDFLAGS	=
HEADERS	= $(shell ls $(INCLUDE)/nexrad/*.h)
OBJS	= message.o chunk.o symbology.o graphic.o tabular.o packet.o radial.o \
	  raster.o date.o test.o
NAME	= test

RM	= /bin/rm

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(NAME)

$(OBJS): %.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $<

install:

clean:
	$(RM) -f $(NAME) $(OBJS)
