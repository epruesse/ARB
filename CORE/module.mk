module:=core
core_TARGETS := lib/libCORE.so
core_HEADERS := arb_assert.h arb_core.h arb_cs.h arb_diff.h arb_file.h arb_handlers.h 
core_HEADERS += arb_match.h arb_mem.h arb_misc.h arb_msg.h arb_msg_fwd.h arb_pathlen.h 
core_HEADERS += arb_progress.h arb_signal.h arb_sort.h arb_strarray.h arb_strbuf.h 
core_HEADERS += arb_string.h BufferedFileReader.h MultiFileReader.h FileContent.h 
core_HEADERS += pos_range.h 

#lib/libCORE.so: LDFLAGS+=$(ARB_GLIB_LIBS)
