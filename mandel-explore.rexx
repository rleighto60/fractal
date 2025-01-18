/* macro for Fractal */
numeric digits 320
parse arg iff
xr = 800
yr = 600
sc = 0
reset xr yr 20000 17
if iff = '' then do
    x0 = -0.5
    y0 = 0.0
    w0 = 2.5
    spectrum 0 25 000764 50 206bdd 100 edffff 150 ffaa00 215 310230
    view x0 y0 w0
    fractal
    end
else do
    read iff
    xr = display.xres
    yr = display.yres
    sc = display.scale
    x0 = fractal.xc
    y0 = fractal.yc
    w0 = fractal.size
    end
display
b = explore(display.button)
if b < 0 then exit
address system 'echo -e "Software\n\t fractal mandel\n\t 'xc' 'yc' 'w'" > /tmp/fractal.txt'
'save(mandel-out.fff)'
address system './bin/ifftoppm mandel-out.fff|pnmtopng -compression 9 -text /tmp/fractal.txt > mandel-out.png'
exit

explore:
arg b

do while b > 3
    xc = x0
    yc = y0
    w = w0
    if xr > yr then do
        dx = w * xr / yr;
        dy = w;
    end
    else do
        dx = w;
        dy = w * yr / xr;
    end
    if b < 6 then do
        x0 = xc + dx * ((display.x / xr) - 0.5)
        y0 = yc + dy * ((display.y / yr) - 0.5)
    end
    if b = 4 then w0 = w / 10
    if b = 5 then w0 = w * 10
    if sc = 1 then sc = 4
    say b
    do while sc > 0
        display.button = 0
        sc = sc - 1
        say 'view' x0 y0 w0 sc
        view x0 y0 w0 sc
        fractal
        b = display.button
        if b > 0 then leave
    end
    sc = 4
end
return b

