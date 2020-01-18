all: build run

rebuild: clean build

build:
	mkdir build && cd build && cmake ../ && make

run:
	cd build && ./server 44446

clean:
	rm -rf build