CC	= cc
CFLAGS	= -g -fno-inline -Wall -O2 -I..
LDFLAGS	=
HEADERS	= $(ls *.h)
OBJS	= message.o chunk.o symbology.o packet.o test.o
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
