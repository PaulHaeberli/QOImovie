all: qiomdemo qiomcat

qiomdemo: qoimdemo.c
	cc qoimdemo.c -o qoimdemo

qiomcat: qoimcat.c
	cc qoimcat.c -o qoimcat

allcpp: qoimdemo.cpp
	c++ qoimdemo.cpp -o qoimdemo

clean:
	rm -f qoimdemo
	rm -f qoimcat
	rm -f out.qoim
	rm -f 3.qoim
	-rm -f TEST*.png

testall:
	./qoimdemo -toqoim testimages/* out.qoim
	./qoimdemo -print out.qoim
	./qoimdemo -topng out.qoim TEST
	./qoimcat out.qoim out.qoim out.qoim 3.qoim

