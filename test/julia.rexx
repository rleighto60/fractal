/* macro for Fractal */
reset 3200 2400 4000 16
do i=0 to 2
view 0.0 0.0 2.6
fractal 0 i 0.355534 '-0.337292'
'save(|ppmlabel -x 10 -y 20 -text "view 0.0 0.0 2.6" -text "julia 'i' 0.355534 -0.337292" |pnmtopng -compression 9 > julia'i'.png)'
end
exit
