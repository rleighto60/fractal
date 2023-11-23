#
default: spectrum.png mandel.png julia.png

%.png: %.rexx
	../bin/fractal $<

clean:
	rm -f *.png
