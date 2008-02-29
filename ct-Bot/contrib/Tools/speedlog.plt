#!/gnuplot
#
# c't-Bot
# 
# This program is free software; you can redistribute it
# and/or modify it under the terms of the GNU General
# Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your
# option) any later version. 
# This program is distributed in the hope that it will be 
# useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
# PURPOSE. See the GNU General Public License for more details.
# You should have received a copy of the GNU General Public 
# License along with this program; if not, write to the Free 
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307, USA.
# 
#
#     Speedlog Plotdatei fuer c't-Bot
#     
#     Stellt die Ergebnisse der slog.txt in grafischer Form dar
#
#     Datum: 3. November 2007
#     Autor: Harald W. Leschner (hari@h9l.net)
#
#Dieser Schalter aktieviert die Ausgabe der EPS Dateien, wenn man z.B. mit LaTeX die
#Bilder drucken moechte.     
#    
#set terminal postscript eps enhanced color

GNUTERM = "win"

set clip one
unset clip two
set bar 1.000000
set border 31 front linetype -1 linewidth 1.000
set xdata
set ydata
set zdata
set x2data
set y2data
set timefmt x "%d/%m/%y,%H:%M"
set timefmt y "%d/%m/%y,%H:%M"
set timefmt z "%d/%m/%y,%H:%M"
set timefmt x2 "%d/%m/%y,%H:%M"
set timefmt y2 "%d/%m/%y,%H:%M"
set timefmt cb "%d/%m/%y,%H:%M"
set boxwidth
set style fill  empty border
set style rectangle back fc  lt -3 fillstyle  solid 1.00 border -1
set dummy x,y
set format x "% g"
set format y "% g"
set format x2 "% g"
set format y2 "% g"
set format z "% g"
set format cb "% g"
set angles radians
set grid nopolar
set grid xtics nomxtics ytics nomytics noztics nomztics \
nox2tics nomx2tics noy2tics nomy2tics nocbtics nomcbtics
set grid layerdefault   linetype 0 linewidth 1.000,  linetype 0 linewidth 1.000

unset key

unset label
unset arrow
set style increment default
unset style line
unset style arrow
set style histogram clustered gap 2 title  offset character 0, 0, 0
unset logscale
set offsets 0, 0, 0, 0
set pointsize 1
set encoding default
unset polar
unset parametric
unset decimalsign
set view 60, 30, 1, 1
set samples 100, 100
set isosamples 10, 10
set surface
unset contour
set clabel '%8.3g'
set mapping cartesian
set datafile separator whitespace
unset hidden3d
set cntrparam order 4
set cntrparam linear
set cntrparam levels auto 5
set cntrparam points 5

set size ratio 0 1,1
set origin 0,0

set style data lines
set style function lines

set xzeroaxis linetype -2 linewidth 1.000
set yzeroaxis linetype -2 linewidth 1.000
set zzeroaxis linetype -2 linewidth 1.000
set x2zeroaxis linetype -2 linewidth 1.000
set y2zeroaxis linetype -2 linewidth 1.000

set ticslevel 0.5
set mxtics default
set mytics default
set mztics default
set mx2tics default
set my2tics default
set mcbtics default

set xtics border in scale 1,0.5 mirror rotate by 90  offset character 0, 0, 0
set xtics autofreq 
set ytics border in scale 1,0.5 mirror norotate  offset character 0, 0, 0
set ytics autofreq 
set ztics border in scale 1,0.5 nomirror norotate  offset character 0, 0, 0
set ztics autofreq 
set nox2tics
set noy2tics
set cbtics border in scale 1,0.5 mirror norotate  offset character 0, 0, 0
set cbtics autofreq

set rrange [ * : * ] noreverse nowriteback  # (currently [0.000000:10.0000] )
set trange [ * : * ] noreverse nowriteback  # (currently [-5.00000:5.00000] )
set urange [ * : * ] noreverse nowriteback  # (currently [-5.00000:5.00000] )
set vrange [ * : * ] noreverse nowriteback  # (currently [-5.00000:5.00000] )

set xlabel "Zeit [ms]" 
set xlabel  offset character 0, 0, 0 font "" textcolor lt -1 norotate
set x2label "" 
set x2label  offset character 0, 0, 0 font "" textcolor lt -1 norotate
set xrange [ 217.801 : 1776.58 ] noreverse nowriteback
set x2range [ 204.422 : 1667.45 ] noreverse nowriteback

set ylabel "Wert [PWM]" 
set ylabel  offset character 0, 0, 0 font "" textcolor lt -1 rotate by 90
set y2label "" 
set y2label  offset character 0, 0, 0 font "" textcolor lt -1 rotate by 90
set yrange [ -76.4141 : 516.211 ] noreverse nowriteback
set y2range [ -55.3430 : 440.846 ] noreverse nowriteback

set zlabel "" 
set zlabel  offset character 0, 0, 0 font "" textcolor lt -1 norotate
set zrange [ * : * ] noreverse nowriteback  # (currently [-10.0000:10.0000] )

set cblabel "" 
set cblabel  offset character 0, 0, 0 font "" textcolor lt -1 norotate
set cbrange [ * : * ] noreverse nowriteback  # (currently [-10.0000:10.0000] )

set zero 1e-008
set lmargin -1
set bmargin -1
set rmargin -1
set tmargin -1
set locale "C"
set pm3d explicit at s
set pm3d scansautomatic
set pm3d interpolate 1,1 flush begin noftriangles nohidden3d corners2color mean
set palette positive nops_allcF maxcolors 0 gamma 1.5 color model RGB 
set palette rgbformulae 7, 5, 15
set colorbox default
set colorbox vertical origin screen 0.9, 0.2, 0 size screen 0.05, 0.6, 0 bdefault
set loadpath 
set fontpath 
set fit noerrorvariables

set output "plots.eps"
set multiplot layout 2,1

set style data lines
set title "Regelung Motor LINKS" 
plot 'slog.txt' index 0 using 0:1 title 1, 'slog.txt' index 0 using 0:2 title 2 , 'slog.txt' index 0 using 0:3 title 3 , 'slog.txt' index 0 using 0:4 title 4 smooth frequency


set title "Regelung Motor RECHTS" 
plot 'slog.txt' index 1 using 0:1 title 1, 'slog.txt' index 1 using 0:2 title 2 , 'slog.txt' index 1 using 0:3 title 3 , 'slog.txt' index 1 using 0:4 title 4 smooth frequency

unset multiplot

set timestamp top 
set timestamp "Erstellt am %d.%m.%y um %H:%M:%S"

pause -1 "Speedlog Plots finished"

set output "links.eps"
set style data lines
set title "Regelung Motor LINKS" 
plot 'slog.txt' index 0 using 0:1 title 1, 'slog.txt' index 0 using 0:2 title 2 , 'slog.txt' index 0 using 0:3 title 3 , 'slog.txt' index 0 using 0:4 title 4 smooth frequency

pause -1 "Speedlog Plots finished"

set output "rechts.eps"
set style data lines
set title "Regelung Motor RECHTS" 
plot 'slog.txt' index 1 using 0:1 title 1, 'slog.txt' index 1 using 0:2 title 2 , 'slog.txt' index 1 using 0:3 title 3 , 'slog.txt' index 1 using 0:4 title 4 smooth frequency

pause -1 "Speedlog Plots finished"

#pause mouse any "Any key or button will terminate"
#    EOF