
all: QOImovie.cpp
	c++ QOImovie.cpp -o QOImovie

clean:
	rm -f QOImovie
	rm -f out.qoim
	-rm -f TEST*.png

testall:
        ./QOImovie -toqoim testimages/* out.qoim
        ./QOImovie -print out.qoim
        ./QOImovie -topng out.qoim TEST
        ./QOImovie -readbenchmark out.qoim
