#!/usr/bin/gnuplot -persist
set encoding utf8
#set terminal postscript eps enhanced color solid
#set output "CoPt_vs_angle.ps"
set key opaque
set key right bottom
#set term png size 600, 400
#set output "figure.png"

set xlabel "H" # font "Helvetica,20"
set ylabel "{/Symbol f}, deg" # font "Helvetica,20"
set y2label "sin (2 {/Symbol f})"
set grid
#set xrange [-0.02:0.02]

alp_deg = 70 #угол между легкой осью и внешним полем
bet_deg = 30 #угол между легкой осью и током
Ha = 100

set xrange [-1000:1000]


alp = alp_deg*pi/180
bet = bet_deg*pi/180
phi(x, Ha) = atan2(x*cos(alp)+Ha, x*sin(alp)) - bet
phi2(x, Ha) = (phi(x, Ha)>-pi ? phi(x, Ha) : phi(x, Ha)+2*pi)

plot -( phi2(x, Ha) )*180/pi axis x1y1 title "{/Symbol f}_+" with lines lc "red"   lw 2, \
     -sin(2*( phi(x, Ha)))  axis x1y2 title "sin(2{/Symbol f}_+)" with linespoints lc "red"   lw 2 pt 2 ps 2 pi 5, \
     -(phi(x, -Ha)*180/pi) axis x1y1 title "{/Symbol f}_-" with lines lc "green" lw 2, \
     -sin(2*( phi(x, -Ha))) axis x1y2 title "sin(2{/Symbol f}_-)" with linespoints lc "green" lw 2 pt 6 ps 2 pi 5 \
     
     
