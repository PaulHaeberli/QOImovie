
all: QOImovie.cpp
	c++ QOImovie.cpp -o QOImovie

clean:
	rm -f QOImovie
	rm -f out.qoim
	-rm -f TEST*.png


test:
	./QOImovie -toqoim testimages/* out.qoim
	./QOImovie -print out.qoim
	./QOImovie -topng out.qoim TEST
