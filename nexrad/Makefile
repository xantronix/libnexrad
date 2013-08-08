CC	= cc
CFLAGS	= -g -std=c99 -fno-inline -Wall -O2 -I..
LDFLAGS	=
HEADERS	= message.h message/format.h types.h
OBJS	= message.o test.o
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
