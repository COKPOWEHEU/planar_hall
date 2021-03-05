#!/usr/bin/gnuplot -persist
set encoding utf8
#set terminal postscript eps enhanced color solid
#set output "CoPt_vs_angle.ps"
set key opaque

set xlabel "B, T" 
set zlabel "R_X_Y, Ohm"
set ylabel "angle"
set grid
#set xrange [-0.02:0.02]

alp_deg = 40 #угол между легкой осью и внешним полем
bet_deg = 22.5 #угол между легкой осью и током
Ha = 300

set xrange [-1000:1000]


alp = alp_deg*pi/180
bet = bet_deg*pi/180
phi(x, Ha) = atan2(x*cos(alp), x*sin(alp)+Ha) - bet
phi2(x, Ha) = (phi(x, Ha)>0 ? phi(x, Ha) : phi(x, Ha)+2*pi)

plot ( phi(x, Ha) )*180/pi axis x1y1 title "{/Symbol f}" with lines lc "red"   lw 2, \
     sin(2*( phi(x, Ha)))  axis x1y2 title "sin(2{/Symbol f})" with linespoints lc "red"   lw 2 pt 2 ps 2 pi 5, \
     (phi2(x, -Ha)*180/pi) axis x1y1 title "{/Symbol y}" with lines lc "green" lw 2, \
     sin(2*( phi(x, -Ha))) axis x1y2 title "sin(2{/Symbol y})" with linespoints lc "green" lw 2 pt 6 ps 2 pi 5 \
     
     
