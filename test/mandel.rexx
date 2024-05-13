/* macro for Fractal */
xres = 1600
yres = 1200
xc = -0.5
yc = 0.0
size = 2.5
reset xres yres 4000 16
view xc yc size
fractal
'save(mandel.iff)'
'save(|../bin/ifftoppm|pnmtopng -compression 9 > mandel.png)'
exit
