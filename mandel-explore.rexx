/* macro for Fractal */
numeric digits 320
parse arg x0 y0 w0
if x0 = '' then x0 = -0.5
if y0 = '' then y0 = 0.0
if w0 = '' then w0 = 2.5
say x0 /* more negative shifts image to right */
say y0 /* more negative shifts image down */
say w0
eps = 2.2204460492503131e-16
xres = 800
yres = 600
ctyp = 0
mandel = 'fractal 0 'ctyp
reset xres yres 20000 17 1
spectrum 1 0 25 000764 50 206bdd 100 edffff 150 ffaa00 215 310230
b = explore(x0,y0,w0,mandel)
if b < 0 then exit
reset 1600 1200 20000 16
spectrum 1 0 25 000764 50 206bdd 100 edffff 150 ffaa00 215 310230
view xc yc w
mandel
address system 'echo -e "Software\n\t fractal mandel\n\t 'xc' 'yc' 'w'" > /tmp/fractal.txt'
'save(|./bin/ifftoppm|pnmtopng -compression 9 -text /tmp/fractal.txt > mandel-out.png)'
exit

explore:
arg x0,y0,w0,fractal

b = 6
do while b > 3
xc = x0
yc = y0
w = w0
say xc
say yc
say w
sc = 4
do while sc > 0
display.button = 0
sc = sc - 1
view xc yc w sc
fractal
b = display.button
if b > 0 then leave
end
if xres > yres then do
    dx = w * xres / yres;
    dy = w;
    end
else do
    dx = w;
    dy = w * yres / xres;
    end
x0 = xc + dx * ((display.x / xres) - 0.5)
y0 = yc + dy * ((display.y / yres) - 0.5)
if b = 4 then w0 = w / 10
if b = 5 then w0 = w * 10
end
return b

