CC	= cc
CFLAGS	= -Wall -O2 -I..
LDFLAGS	=
HEADERS	= message.h radar.h types.h
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
