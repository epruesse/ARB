
all: eigen \
     boost

clean:
	rm -rf eigen/
	rm -rf boost/

eigen: eigen.tgz
	tar -zxf $<
	touch eigen/

boost:boost.tar.gz
	tar -zxf $<
	touch boost/

