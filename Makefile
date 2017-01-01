PREFIX := /usr/local/bin

.PHONY: install uninstall clean

cfg-checker: main.cpp
	g++ -flto -O3 -o cfg-checker main.cpp \
		-std=c++11 -Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter

install: cfg-checker
	mkdir -p "$(PREFIX)"
	cp cfg-checker "$(PREFIX)/cfg-checker"

uninstall:
	rm -f "$(PREFIX)/cfg-checker"

clean:
	rm -f cfg-checker
