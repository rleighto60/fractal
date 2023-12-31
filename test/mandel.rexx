/* macro for Fractal */
xres = 1600
yres = 1200
xc = -0.5
yc = 0.0
size = 2.5
reset xres yres 4000 16
view xc yc size
fractal
display
say display.x display.y display.button
'save(|ppmlabel -x 10 -y 20 -text "view -0.5 0.0 2.5" |pnmtopng -compression 9 > mandel.png)'
exit
