/* macro for Fractal */
reset 1600 1200 4000 16
view 0.0 0.0 2.6
fractal 0 0.355534 '-0.337292'
'save(|ppmlabel -x 10 -y 20 -text "view 0.0 0.0 2.6" -text "julia 0.355534 -0.337292" |pnmtopng -compression 9 > julia.png)'
exit
