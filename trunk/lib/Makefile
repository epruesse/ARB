help.make:
	@echo	'options'
	@echo	'	nameserver		create nameserver from template'
	@echo	'	tcpdat			create arb_tcp.dat from template'
	@echo	''
	@echo	'   all             all above'

all: tcpdat nameserver

tcpdat:
		test arb_tcp.dat || cp -p arb_tcp_org.dat arb_tcp.dat

nameserver:
		(cd nas;$(MAKE) all)

