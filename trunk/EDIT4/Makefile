# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx

CPP_OBJECTS = ED4_dump.o ED4_nds.o ED4_secedit.o \
	 	ED4_main.o ED4_mini_classes.o ED4_edit_string.o ED4_base.o ED4_manager.o ED4_terminal.o \
	 	ED4_root.o EDB_root_bact.o ED4_window.o ED4_members.o ED4_no_class.o ED4_list.o ED4_text_terminals.o \
		ED4_cursor.o ED4_tools.o ED4_search.o ED4_block.o ED4_visualizeSAI.o edit_naligner.o 

$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c -o $@ $(@:.o=.cxx)  $(CPPINCLUDES)

depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)


# DO NOT DELETE THIS LINE -- make depend depends on it.
