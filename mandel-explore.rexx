/* macro for Fractal */
numeric digits 32
xres = 1600
yres = 1200
ctyp = 1
mandel = 'fractal 0 'ctyp
reset xres yres 10000 16
palette 32 2 070f19 7B98F1 010015 2D62F3 22010b 4FDCE5 1f1306 618647 041d00 E3E099 220d21 556A5B 210708 37A40D 1a1b11 090e2f 0d021c 4B2841 21001e 7D72C3 1c0806 1F6C35 051207 B19617 071b08 B370DF 200703 A57A2B 0f0102 07345D 1b0611
b = explore(xres,yres,-0.5,0.0,2.5,mandel)
reset 6400 4800 50000 16
palette 32 2 070f19 7B98F1 010015 2D62F3 22010b 4FDCE5 1f1306 618647 041d00 E3E099 220d21 556A5B 210708 37A40D 1a1b11 090e2f 0d021c 4B2841 21001e 7D72C3 1c0806 1F6C35 051207 B19617 071b08 B370DF 200703 A57A2B 0f0102 07345D 1b0611
view xc yc w
say 'view 'xc' 'yc' 'w
say mandel
mandel
address system 'echo -e "Software\n\t fractal mandel\n\t xc='xc' yc='yc' w='w'" > /tmp/fractal.txt'
'save(|pnmtopng -compression 9 -text /tmp/fractal.txt > mandel-out.png)'
if b \= 3 then do
xi = xc
yi = yc
julia = 'fractal 0 'xi' 'yi' 'ctyp
reset xres yres 10000 16
palette 32 2 070f19 7B98F1 010015 2D62F3 22010b 4FDCE5 1f1306 618647 041d00 E3E099 220d21 556A5B 210708 37A40D 1a1b11 090e2f 0d021c 4B2841 21001e 7D72C3 1c0806 1F6C35 051207 B19617 071b08 B370DF 200703 A57A2B 0f0102 07345D 1b0611
b = explore(xres,yres,0.0,0.0,2.5,julia)
reset 6400 4800 50000 16
palette 32 2 070f19 7B98F1 010015 2D62F3 22010b 4FDCE5 1f1306 618647 041d00 E3E099 220d21 556A5B 210708 37A40D 1a1b11 090e2f 0d021c 4B2841 21001e 7D72C3 1c0806 1F6C35 051207 B19617 071b08 B370DF 200703 A57A2B 0f0102 07345D 1b0611
view xc yc w
say 'view 'xc' 'yc' 'w
say julia
julia
address system 'echo -e "Software\n\t fractal julia\n\t xi='xi' yi='yi'\n\t xc='xc' yc='yc' w='w'" > /tmp/fractal.txt'
'save(|pnmtopng -compression 9 -text /tmp/fractal.txt > julia-out.png)'
end
exit

explore:
arg xres,yres,x0,y0,w0,fractal

xc = x0
yc = y0
dx = 0
dy = 0
b = 1
xr = xres
yr = yres
do while b = 1
w = w0
xc = xc + dx * ((xr / xres) - 0.5)
yc = yc + dy * ((yr / yres) - 0.5)
view xc yc w
fractal
text = 'view 'xc' 'yc' 'w
'save(/tmp/fractal.pnm)'
address system 'display "/tmp/fractal.pnm"' with output stem out.
w0 = w0 / 2
xr = out.1
yr = out.2
b = out.3
dx = w0 * xres / yres
dy = w0
if w0 < 1e-12 then b = 2
if w0 < 1e-11 then say 'getting close to the extreme'
end
return b

