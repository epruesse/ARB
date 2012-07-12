help.make:
	@echo	'options'
	@echo	'	nameserver		create nameserver from template'
	@echo	'	tcpdat			create arb_tcp.dat from template'
	@echo	''
	@echo	'   all             all above'
	@echo	'   clean           cleanup'

all: tcpdat nameserver

# ----------------------------------------
# READ THIS!
# 
# Please be very careful here to NOT
# touch the following files/directories if
# they are links:
# 
#	./arb_tcp.dat
#	./nas
#	./pts
#
# ----------------------------------------

tcpdat:
	(	(test -e arb_tcp.dat || test -h arb_tcp.dat) \
		&& echo "lib/arb_tcp.dat is a link - skipped" ) \
		|| cp -p arb_tcp_org.dat arb_tcp.dat

nameserver:
	( test -h nas && echo "lib/nas is a link - skipped" ) || (cd nas;$(MAKE) all)

clean:
	rm -f *.so
	( test -h nas && echo "lib/nas is a link - skipped" ) || (cd nas;$(MAKE) clean)
	( test -h pts && echo "lib/pts is a link - skipped" ) || rm -rf pts
	( test -h arb_tcp.dat && echo "lib/arb_tcp.dat is a link - skipped" ) || rm -f arb_tcp.dat
