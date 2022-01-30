/* macro for Fractal */
reset 3200 2400 4000 4
view 0.0 0.0 2.6
julia 0.355534 '-0.337292'
'save(|pnmtojpeg -quality 90 > julia.jpg)'
exit
