OBJECT = \
	nil_pal.o nil_simrank.o

all: $(OBJECT:%.o=$(ARBHOME)/bin/%)

$(ARBHOME)/bin/%: %.f
	$(F77) $(f77_flags) -f -o $@ $< $(F77LIB)

depend:
	@echo "No dependencies generated for NIELS_F77"
