/* macro for Fractal */
reset 3200 2400 4000 4
view 0.0 0.0 2.6
i = 1
julia 0.355534 '-0.337292' i
'save(|ppmlabel -x 10 -y 20 -text "view 0.0 0.0 2.6" -text "julia 0.355534 -0.337292 'i'" |pnmtojpeg -quality 90 > julia.jpg)'
exit
