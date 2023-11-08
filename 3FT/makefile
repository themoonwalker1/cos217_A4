#GCC = gcc217
GCC = gcc
#FLAGS = -g

all: ft

clean:
	rm -f *.o ft meminfo*.out

ft: dynarray.o path.o checkerFT.o nodeFT.o ft.o ft_client.o
	$(GCC) $(FLAGS) dynarray.o path.o checkerFT.o nodeFT.o ft.o ft_client.o -o ft

dynarray.o: dynarray.c dynarray.h
	$(GCC) $(FLAGS) -c $<

path.o: path.c dynarray.h path.h a4def.h
	$(GCC) $(FLAGS) -c $<

ft_client.o: ft_client.c ft.h a4def.h
	$(GCC) $(FLAGS) -c $<

checkerFT.o: checkerFT.c dynarray.h checkerFT.h nodeFT.h path.h a4def.h
	$(GCC) $(FLAGS) -c $<

nodeFT.o: nodeFT.c dynarray.h checkerFT.h nodeFT.h path.h a4def.h
	$(GCC) $(FLAGS) -c $<

ft.o: ft.c dynarray.h checkerFT.h nodeFT.h ft.h path.h a4def.h
	$(GCC) $(FLAGS) -c $<