GCC = gcc217
#GCC = gcc

all: ft

clean:
	rm -f *.o ft meminfo*.out

ft: dynarray.o path.o checkerFT.o nodeFT.o ft.o ft_client.o
	$(GCC) dynarray.o path.o checkerFT.o nodeFT.o ft.o ft_client.o -o ft

dynarray.o: dynarray.c dynarray.h
	$(GCC) -c dynarray.c dynarray.h

path.o: path.c dynarray.h path.h a4def.h
	$(GCC) -c path.c dynarray.h path.h a4def.h

ft_client.o: ft_client.c ft.h a4def.h
	$(GCC) -c ft_client.c ft.h a4def.h

checkerFT.o: checkerFT.c dynarray.h checkerFT.h nodeFT.h path.h a4def.h
	$(GCC) -c checkerFT.c dynarray.h checkerFT.h nodeFT.h path.h a4def.h

nodeFT.o: nodeFT.c dynarray.h checkerFT.h nodeFT.h path.h a4def.h
	$(GCC) -c nodeFT.c dynarray.h checkerFT.h nodeFT.h path.h a4def.h

ft.o: ft.c dynarray.h checkerFT.h nodeFT.h ft.h path.h a4def.h
	$(GCC) -c ft.c dynarray.h checkerFT.h nodeFT.h ft.h path.h a4def.h