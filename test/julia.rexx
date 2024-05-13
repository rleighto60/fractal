/* macro for Fractal */
reset 1600 1200 4000 16
view 0.0 0.0 2.6
fractal 0 0.355534 '-0.337292'
'save(|../bin/ifftoppm|pnmtopng -compression 9 > julia.png)'
exit
