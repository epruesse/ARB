
all: names.dat
	rm -f *%
	@chmod 777 .
	@cp -p names.dat names.dat.make_owner
	@rm -f names.dat
	@mv names.dat.make_owner names.dat
	@chmod 666 names.dat

names.dat: names.dat.template
		rm -f $@
		cp $< $@

clean:
	rm -f names.dat
