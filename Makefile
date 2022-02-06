
allc: qoimdemo.c
	cc qoimdemo.c -o qoimdemo

allcpp: qoimdemo.cpp
	c++ qoimdemo.cpp -o qoimdemo

clean:
	rm -f qoimdemo
	rm -f out.qoim
	-rm -f TEST*.png

testall:
	./qoimdemo -toqoim testimages/* out.qoim
	./qoimdemo -print out.qoim
	./qoimdemo -topng out.qoim TEST

