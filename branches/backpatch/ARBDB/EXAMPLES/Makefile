CC = g++ -g

all:	all_spec_names write_all_species read_all_sequences


%:	%.c
	$(CC) -I${ARBHOME}/INCLUDE -L${ARBHOME}/lib -o $@ $< $(ARBDB_LIB) 

#-lnsl -L/usr/ucblib -lucb -lposix4
