
all: qomutil imgproc qomcat

qomutil: qomutil.c
	cc qomutil.c -o qomutil

imgproc: imgproc.c
	cc imgproc.c -o imgproc

qomcat: qomcat.c
	cc qomcat.c -o qomcat

allcpp: qomutil.cpp
	c++ qomutil.cpp -o qomutil

clean:
	rm -f qomutil imgproc qomcat
	-rm -f tmp/*

test:
	./qomutil -toqom testimages/* tmp/out.qom
	./qomutil -topng tmp/out.qom tmp/TEST
	./qomutil -print tmp/out.qom
	./qomutil -benchmark tmp/out.qom

pyramid:
	./qomutil -toqom testimages/* tmp/level0.qom
	./imgproc tmp/level0.qom tmp/level1.qom zoom 0.5 0.5
	./imgproc tmp/level1.qom tmp/level2.qom zoom 0.5 0.5
	./imgproc tmp/level2.qom tmp/level3.qom zoom 0.5 0.5
	./imgproc tmp/level3.qom tmp/level4.qom zoom 0.5 0.5
	./imgproc tmp/level4.qom tmp/level5.qom zoom 0.5 0.5
	./imgproc tmp/level5.qom tmp/level6.qom zoom 0.5 0.5
	./imgproc tmp/level6.qom tmp/level7.qom zoom 0.5 0.5
	./qomutil -print tmp/level0.qom
	./qomutil -print tmp/level1.qom
	./qomutil -print tmp/level2.qom
	./qomutil -print tmp/level3.qom
	./qomutil -print tmp/level4.qom
	./qomutil -print tmp/level5.qom
	./qomutil -print tmp/level6.qom
	./qomutil -print tmp/level7.qom
	./qomcat tmp/level*.qom tmp/pyramid.qom
	./qomutil -print tmp/pyramid.qom

randseg:
	./qomutil -randseg tmp/out.qom tmp/RANDSEG00.qom 5
	./qomutil -randseg tmp/out.qom tmp/RANDSEG01.qom 5
	./qomutil -randseg tmp/out.qom tmp/RANDSEG02.qom 5
	./qomutil -randseg tmp/out.qom tmp/RANDSEG03.qom 5
	./qomutil -randseg tmp/out.qom tmp/RANDSEG04.qom 5
	./qomutil -randseg tmp/out.qom tmp/RANDSEG05.qom 5
	./qomutil -print tmp/RANDSEG05.qom
	./qomcat tmp/RANDSEG*.qom tmp/RAND.qom
	./qomutil -print tmp/RAND.qom

