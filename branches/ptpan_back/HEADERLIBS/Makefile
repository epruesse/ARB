
all: eigen \
     boost

clean:
	rm -rf eigen/
	rm -rf boost/

eigen: eigen.tgz
	tar -zxvf $<
	touch eigen/

boost:boost.tar.gz
	tar -zxvf $<
	touch boost/

