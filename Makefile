
all: QOImovie.cpp
	c++ QOImovie.cpp -o QOImovie

clean:
	rm -f QOImovie
	rm -f out.qoim
	-rm -f TEST*.png


testtoqoim:
        ./QOImovie -toqoim testimages/* out.qoim

testprint:
        ./QOImovie -print out.qoim

testtopng:
        ./QOImovie -topng out.qoim TEST

testreadbenchmark:
        ./QOImovie -readbenchmark out.qoim


testall:
        ./QOImovie -toqoim testimages/* out.qoim
        ./QOImovie -print out.qoim
        ./QOImovie -topng out.qoim TEST
        ./QOImovie -readbenchmark out.qoim
