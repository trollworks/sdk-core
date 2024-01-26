DESTDIR := /usr/local

.PHONY: test
test:
	@make -C tests all

.PHONY: install
install:
	@mkdir -p $(DESTDIR)/include
	@cp -R include $(DESTDIR)
