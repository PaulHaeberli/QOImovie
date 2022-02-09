
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
	-rm -f tmp/*

test:
	./qoimutil -toqoim testimages/* tmp/out.qoim
	./qoimutil -topng tmp/out.qoim tmp/TEST
	./qoimutil -print tmp/out.qoim
	./qoimutil -benchmark tmp/out.qoim

pyramid:
	./qoimutil -toqoim testimages/* tmp/level0.qoim
	./imgproc tmp/level0.qoim tmp/level1.qoim zoom 0.5 0.5
	./imgproc tmp/level1.qoim tmp/level2.qoim zoom 0.5 0.5
	./imgproc tmp/level2.qoim tmp/level3.qoim zoom 0.5 0.5
	./imgproc tmp/level3.qoim tmp/level4.qoim zoom 0.5 0.5
	./imgproc tmp/level4.qoim tmp/level5.qoim zoom 0.5 0.5
	./imgproc tmp/level5.qoim tmp/level6.qoim zoom 0.5 0.5
	./imgproc tmp/level6.qoim tmp/level7.qoim zoom 0.5 0.5
	./qoimutil -print tmp/level0.qoim
	./qoimutil -print tmp/level1.qoim
	./qoimutil -print tmp/level2.qoim
	./qoimutil -print tmp/level3.qoim
	./qoimutil -print tmp/level4.qoim
	./qoimutil -print tmp/level5.qoim
	./qoimutil -print tmp/level6.qoim
	./qoimutil -print tmp/level7.qoim
	./qoimcat tmp/level*.qoim tmp/pyramid.qoim
	./qoimutil -print tmp/pyramid.qoim

randseg:
	./qoimutil -randseg tmp/out.qoim tmp/RANDSEG00.qoim 5
	./qoimutil -randseg tmp/out.qoim tmp/RANDSEG01.qoim 5
	./qoimutil -randseg tmp/out.qoim tmp/RANDSEG02.qoim 5
	./qoimutil -randseg tmp/out.qoim tmp/RANDSEG03.qoim 5
	./qoimutil -randseg tmp/out.qoim tmp/RANDSEG04.qoim 5
	./qoimutil -randseg tmp/out.qoim tmp/RANDSEG05.qoim 5
	./qoimutil -print tmp/RANDSEG05.qoim
	./qoimcat tmp/RANDSEG*.qoim tmp/RAND.qoim
	./qoimutil -print tmp/RAND.qoim

