test: Main.o Filesystem.o File.o INode.o Superblock.o Address.o
	g++ Main.o Filesystem.o File.o INode.o Superblock.o Address.o -o test

File.o: File.cpp
	g++ -c File.cpp

INode.o: INode.cpp
	g++ -c INode.cpp

Main.o: main.cpp
	g++ -c main.cpp -o Main.o

Filesystem.o: Filesystem.cpp
	g++ -c Filesystem.cpp

Superblock.o: Superblock.cpp
	g++ -c Superblock.cpp

Address.o: Address.cpp
	g++ -c Address.cpp

clean:
	-rm -f *.o