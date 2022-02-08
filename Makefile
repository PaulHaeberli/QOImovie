
all: qoimutil imgproc qoimcat

qoimutil: qoimutil.c
	cc qoimutil.c -o qoimutil

imgproc: imgproc.c
	cc imgproc.c -o imgproc

qoimcat: qoimcat.c
	cc qoimcat.c -o qoimcat

allcpp: qoimutil.cpp
	c++ qoimutil.cpp -o qoimutil

clean:
	rm -f qoimutil imgproc qoimcat
	-rm -f *.qoim
	-rm -f TEST*.png

pyramid:
	./qoimutil -toqoim testimages/* level0.qoim
	imgproc level0.qoim level1.qoim zoom 0.5 0.5
	imgproc level1.qoim level2.qoim zoom 0.5 0.5
	imgproc level2.qoim level3.qoim zoom 0.5 0.5
	imgproc level3.qoim level4.qoim zoom 0.5 0.5
	imgproc level4.qoim level5.qoim zoom 0.5 0.5
	imgproc level5.qoim level6.qoim zoom 0.5 0.5
	imgproc level6.qoim level7.qoim zoom 0.5 0.5
	./qoimutil -print level0.qoim
	./qoimutil -print level1.qoim
	./qoimutil -print level2.qoim
	./qoimutil -print level3.qoim
	./qoimutil -print level4.qoim
	./qoimutil -print level5.qoim
	./qoimutil -print level6.qoim
	./qoimutil -print level7.qoim
	./qoimcat level*.qoim pyramid.qoim
	./qoimutil -print pyramid.qoim

testall:
	./qoimutil -toqoim testimages/* out.qoim
	./qoimutil -print out.qoim
	./qoimutil -topng out.qoim TEST
	./qoimcat out.qoim out.qoim out.qoim 3.qoim

