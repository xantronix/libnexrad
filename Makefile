all:
	$(MAKE) -C src all

.PHONY: examples
examples:
	$(MAKE) -C examples all

install:
	$(MAKE) -C src install

uninstall:
	$(MAKE) -C src uninstall

clean:
	$(MAKE) -C src clean
	$(MAKE) -C examples clean
