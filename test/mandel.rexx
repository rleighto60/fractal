/* macro for Fractal */
reset 3200 2400 4000 4
view '-0.5' 0.0 2.5
i = 1
mandel i
'save(|ppmlabel -x 10 -y 20 -text "view -0.5 0.0 2.5" -text "mandel 'i'" |pnmtojpeg -quality 90 > mandel.jpg)'
exit
