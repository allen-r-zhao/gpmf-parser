extrGPS : extrGPS.c ../GPMF_parser.h
		gcc -o extrGPS extrGPS.o GPMF_parser.o GPMF_mp4rader.o GPMF_print.o
		
gpmfdemo : GPMF_demo.o GPMF_parser.o GPMF_mp4reader.o GPMF_print.o
		gcc -o gpmfdemo GPMF_demo.o GPMF_parser.o GPMF_mp4reader.o GPMF_print.o

extrGPS.o : extrGPS.c ../GPMF_parser.h
		gcc -c extrGPS.c
GPMF_demo.o : GPMF_demo.c ../GPMF_parser.h
		gcc -c GPMF_demo.c
GPMF_mp4reader.o : GPMF_mp4reader.c ../GPMF_parser.h
		gcc -c GPMF_mp4reader.c
GPMF_print.o : GPMF_print.c ../GPMF_parser.h
		gcc -c GPMF_print.c
GPMF_parser.o : ../GPMF_parser.c ../GPMF_parser.h
		gcc -c ../GPMF_parser.c
clean :
		rm extrGPS gpmfdemo extrGPS.o GPMF_demo.o GPMF_parser.o GPMF_mp4reader.o GPMF_print.o
