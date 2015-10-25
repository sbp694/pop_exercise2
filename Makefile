all: 2dc 

2dc: 2dc.cpp ppma_io.a
	gcc -O3 $^ -o $@ -lm -lrt

ppma_io.a: ppma_io.o 
	ar rs $@ $<
