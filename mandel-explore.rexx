/* macro for Fractal */
numeric digits 32
parse arg x0 y0 w0
if x0 = '' then x0 = -0.5
if y0 = '' then y0 = 0.0
if w0 = '' then w0 = 2.5
say x0' 'y0' 'w0
xres = 1600
yres = 1200
ctyp = 0
mandel = 'fractal 0 'ctyp
reset xres yres 10000 16
spectrum 1 0 25 000764 50 206bdd 100 edffff 150 ffaa00 215 310230
b = explore(x0,y0,w0,mandel)
if b < 0 then exit
reset 6400 4800 50000 16
spectrum 1 0 25 000764 50 206bdd 100 edffff 150 ffaa00 215 310230
view xc yc w
mandel
address system 'echo -e "Software\n\t fractal mandel\n\t 'xc' 'yc' 'w'" > /tmp/fractal.txt'
'save(|./bin/ifftoppm|pnmtopng -compression 9 -text /tmp/fractal.txt > mandel-out.png)'
save
if b = 3 then do
xi = xc
yi = yc
julia = 'fractal 0 'xi' 'yi' 'ctyp
reset xres yres 10000 16
spectrum 1 0 25 000764 50 206bdd 100 edffff 150 ffaa00 215 310230
b = explore(0.0,0.0,2.5,julia)
if b < 0 then exit
reset 6400 4800 50000 16
spectrum 1 0 25 000764 50 206bdd 100 edffff 150 ffaa00 215 310230
view xc yc w
julia
address system 'echo -e "Software\n\t fractal julia\n\t xi='xi' yi='yi'\n\t xc='xc' yc='yc' w='w'" > /tmp/fractal.txt'
'save(|./bin/ifftoppm|pnmtopng -compression 9 -text /tmp/fractal.txt > julia-out.png)'
end
exit

explore:
arg x0,y0,w0,fractal

b = 4
do while b > 3
xc = x0
yc = y0
w = w0
say xc' 'yc' 'w
view xc yc w
fractal
'display('spectrum')'
x0 = display.x
y0 = display.y
b = display.button
if b = 4 then w0 = w / 2
if b = 5 then w0 = w * 2
if w0 < 1e-13 then b = 2
else if w0 < 1e-12 then say 'getting close to the extreme'
end
return b

