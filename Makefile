all:
	$(MAKE) -C src all

.PHONY: examples
examples:
	$(MAKE) -C examples all

install:
	$(MAKE) -C src install
	$(MAKE) -C colors all install

uninstall:
	$(MAKE) -C src uninstall
	$(MAKE) -C colors uninstall

clean-colors:
	$(MAKE) -C colors clean

clean-examples:
	$(MAKE) -C examples clean

clean: clean-colors clean-examples
	$(MAKE) -C src clean
