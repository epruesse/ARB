CC = g++ -g

all:	all_spec_names write_all_species read_all_sequences


%:	%.c
#	$(CC) -I${ARBHOME}/INCLUDE -L${ARBHOME}/lib -o $@ $< -lARBDB -lsocket -lgen -lposix4
	$(CC) -I${ARBHOME}/INCLUDE -L${ARBHOME}/lib -o $@ $< -lARBDB 

#-lnsl -L/usr/ucblib -lucb -lposix4
