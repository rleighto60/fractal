#
default: spectrum.jpg mandel.jpg julia.jpg

%.jpg: %.rexx
	../bin/fractal $<

clean:
	rm -f *.jpg

