all: build run

rebuild: clean build

build:
	mkdir build && cd build && cmake ../ && make

run:
	cd build && ./server localhost

clean:
	rm -rf build