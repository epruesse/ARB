# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx


CPP_OBJECTS =  NT_edconf.o NT_join.o NT_sort.o NT_cb.o NT_main.o NT_extern.o\
	NT_validNames.o NT_validNameParser.o \
	NT_validManual.o \
	NT_ins_col.o NT_import.o NT_concatenate.o\
	ad_spec.o ad_trees.o  ad_ali.o ad_ext.o ad_transpro.o \
	AP_consensus.o AP_cprofile.o AP_pos_var_pars.o \
	ETC_check_gcg.o AP_csp_2_gnuplot.o AP_conservProfile2Gnuplot.o

CPP_SOURCES = $(CPP_OBJECTS:.o=.cxx)

$(MAIN): nt_date_dummy $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

nt_date.h: $(CPP_SOURCES)
	echo '#define DATE' `date '+"%y_%m_%d"'`  >nt_date.h

nt_date_dummy:
	$(MAKE) nt_date.h

proto:
	rm -f nt_internal.h
	../MAKEBIN/aisc_mkpt -A -E -F NT_,AP_ $(CPP_SOURCES) >nt_internal.h

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_SOURCES)

# DO NOT DELETE THIS LINE -- make depend depends on it.

