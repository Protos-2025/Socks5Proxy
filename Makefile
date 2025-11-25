all: server client

bin:
	@mkdir -p bin

server: bin
	@$(MAKE) -C src/server all

client: bin
	@$(MAKE) -C src/client all

test: bin
	@$(MAKE) -C tests all

clean:
	$(MAKE) -C src/server clean
	$(MAKE) -C src/client clean
	$(MAKE) -C src/shared clean
	$(MAKE) -C tests clean
	rm -rf bin

.PHONY: bin server client test clean all
