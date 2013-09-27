all:
	$(MAKE) -C src all
	$(MAKE) -C colors all

.PHONY: examples
examples:
	$(MAKE) -C examples all

install:
	$(MAKE) -C src install
	$(MAKE) -C colors install

uninstall:
	$(MAKE) -C src uninstall
	$(MAKE) -C colors uninstall

clean:
	$(MAKE) -C src clean
	$(MAKE) -C colors clean
	$(MAKE) -C examples clean
