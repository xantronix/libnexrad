all:
	$(MAKE) -C src all

.PHONY: examples
examples:
	$(MAKE) -C examples all

install:
	$(MAKE) -C src install

uninstall:
	$(MAKE) -C src uninstall

clean-examples:
	$(MAKE) -C examples clean

clean: clean-examples
	$(MAKE) -C src clean
