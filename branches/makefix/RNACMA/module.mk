module:=cma
cma_TARGETS:=bin/arb_rnacma
cma_CPPFLAGS:=-IHEADERLIBS/
cma_LIBADD:=db

bin/arb_rnacma:HAS_MAIN=1

$(cma_OBJECTS:%.o=%.d):	testfile

testfile:
	echo xxx
	touch testfile
