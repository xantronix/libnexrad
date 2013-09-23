CC		= cc
CFLAGS		= -g -Wno-unused-result -fno-inline -Wall -O2
LDFLAGS		= -lnexrad

EXAMPLES	= test savepng

RM		= /bin/rm

all: $(EXAMPLES)

$(EXAMPLES): %: %.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

clean:
	$(RM) -f $(EXAMPLES)
