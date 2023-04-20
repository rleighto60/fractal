/* macro for Fractal */
xres = 1600
yres = 1200
xc = -0.5
yc = 0.0
size = 2.5
reset xres yres 4000 16
view xc yc size
fractal 0
'save(|ppmlabel -x 10 -y 20 -text "view -0.5 0.0 2.5" -text "mandel 0" |pnmtopng -compression 9  > mandel0.png)'
address system 'display "|pngtopnm mandel0.png"' with output stem out.
dx = size * xres / yres
dy = size
xn = xc + dx * ((out.1 / xres) - 0.5)
yn = yc + dy * ((out.2 / yres) - 0.5)
say 'x 'out.1' y 'out.2' button 'out.3
say 'xc 'xn' yc 'yn
view xn yn 0.8
fractal 0
'save(|ppmlabel -x 10 -y 20 -text "view 'xn' 'yn' 0.8" -text "mandel 0" |pnmtopng -compression 9  > mandel1.png)'
address system 'display "|pngtopnm mandel1.png"' with output stem out.
say 'x 'out.1' y 'out.2' button 'out.3
exit
