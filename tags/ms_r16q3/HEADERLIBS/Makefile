
all: eigen

clean:
	rm -rf eigen/

EIGEN_PATCH=eigen_fix_gcc_610_compile.patch

eigen: eigen.tgz $(EIGEN_PATCH) Makefile
	tar -zxvf $<
	patch -p0 < $(EIGEN_PATCH)
	touch eigen/

