CC		= cc
CFLAGS		:= -g -Wno-unused-result -fno-inline -Wall -O2 $(shell pkg-config --cflags cairo)
LDFLAGS		:= -lnexrad $(shell pkg-config --libs cairo)

EXAMPLES	= display savepng

RM		= /bin/rm

all: $(EXAMPLES)

$(EXAMPLES): %: %.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

clean:
	$(RM) -f $(EXAMPLES)
