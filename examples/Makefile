CC		= cc
CFLAGS		= -I../include -g -Wno-unused-result -fno-inline -Wall -O2
LDFLAGS		= -L../src -lnexrad -lbz2 -lz -lm

EXAMPLES	= display render psychedelic

RM		= /bin/rm

all: $(EXAMPLES)

$(EXAMPLES): %: %.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

clean:
	$(RM) -f $(EXAMPLES)
