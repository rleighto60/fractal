#
all:
	mkdir -p bin
	cd src; make
	cd test; make

clean:
	cd src; make clean
	cd test; make clean
	rm -rf bin
