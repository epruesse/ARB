# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben
.SUFFIXES: .o .cxx

CPP_OBJECTS = AWTC_next_neighbours.o AWTC_rename.o AWTC_names_admin.o \
	AWTC_seq_search.o AWTC_fast_aligner.o AWTC_ClustalV.o AWTC_constructSequence.o \
	AWTC_submission.o
# AWTC_names_client.o
$(MAIN): $(CPP_OBJECTS)
	$(AR) $(MAIN) $(CPP_OBJECTS)
.cxx.o:
	$(CPP) $(cflags) -c $<  $(CPPINCLUDES)
depend:
	$(MAKEDEPEND) $(MAKEDEPENDINC) $(CPP_OBJECTS:.o=.cxx)
# DO NOT DELETE

AWTC_next_neighbours.o: /usr/include/stdio.h /usr/include/features.h
AWTC_next_neighbours.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
AWTC_next_neighbours.o: /usr/include/bits/types.h /usr/include/libio.h
AWTC_next_neighbours.o: /usr/include/_G_config.h
AWTC_next_neighbours.o: /usr/include/bits/stdio_lim.h /usr/include/stdlib.h
AWTC_next_neighbours.o: /usr/include/sys/types.h /usr/include/time.h
AWTC_next_neighbours.o: /usr/include/endian.h /usr/include/bits/endian.h
AWTC_next_neighbours.o: /usr/include/sys/select.h /usr/include/bits/select.h
AWTC_next_neighbours.o: /usr/include/bits/sigset.h
AWTC_next_neighbours.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
AWTC_next_neighbours.o: /usr/include/string.h /usr/include/memory.h
AWTC_next_neighbours.o: /trance1/ARB/source/ARB/INCLUDE/arbdb.h
AWTC_next_neighbours.o: /trance1/ARB/source/ARB/INCLUDE/ad_prot.h
AWTC_next_neighbours.o: /trance1/ARB/source/ARB/INCLUDE/arbdbt.h
AWTC_next_neighbours.o: /trance1/ARB/source/ARB/INCLUDE/ad_t_prot.h
AWTC_next_neighbours.o: /trance1/ARB/source/ARB/INCLUDE/servercntrl.h
AWTC_next_neighbours.o: /trance1/ARB/source/ARB/INCLUDE/PT_com.h
AWTC_next_neighbours.o: /trance1/ARB/source/ARB/INCLUDE/client.h
AWTC_next_neighbours.o: ./awtc_next_neighbours.hxx
AWTC_rename.o: /usr/include/stdio.h /usr/include/features.h
AWTC_rename.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
AWTC_rename.o: /usr/include/bits/types.h /usr/include/libio.h
AWTC_rename.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
AWTC_rename.o: /usr/include/stdlib.h /usr/include/sys/types.h
AWTC_rename.o: /usr/include/time.h /usr/include/endian.h
AWTC_rename.o: /usr/include/bits/endian.h /usr/include/sys/select.h
AWTC_rename.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
AWTC_rename.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
AWTC_rename.o: /usr/include/string.h /usr/include/malloc.h
AWTC_rename.o: /trance1/ARB/source/ARB/INCLUDE/arbdb.h
AWTC_rename.o: /trance1/ARB/source/ARB/INCLUDE/ad_prot.h
AWTC_rename.o: /trance1/ARB/source/ARB/INCLUDE/arbdbt.h
AWTC_rename.o: /trance1/ARB/source/ARB/INCLUDE/ad_t_prot.h
AWTC_rename.o: /trance1/ARB/source/ARB/INCLUDE/aw_root.hxx
AWTC_rename.o: /trance1/ARB/source/ARB/INCLUDE/aw_device.hxx
AWTC_rename.o: /trance1/ARB/source/ARB/INCLUDE/aw_window.hxx awtc_rename.hxx
AWTC_rename.o: /trance1/ARB/source/ARB/INCLUDE/names_client.h
AWTC_rename.o: /trance1/ARB/source/ARB/INCLUDE/servercntrl.h
AWTC_rename.o: /trance1/ARB/source/ARB/INCLUDE/client.h
AWTC_names_admin.o: /usr/include/stdio.h /usr/include/features.h
AWTC_names_admin.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
AWTC_names_admin.o: /usr/include/bits/types.h /usr/include/libio.h
AWTC_names_admin.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
AWTC_names_admin.o: /usr/include/stdlib.h /usr/include/sys/types.h
AWTC_names_admin.o: /usr/include/time.h /usr/include/endian.h
AWTC_names_admin.o: /usr/include/bits/endian.h /usr/include/sys/select.h
AWTC_names_admin.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
AWTC_names_admin.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
AWTC_names_admin.o: /trance1/ARB/source/ARB/INCLUDE/arbdb.h
AWTC_names_admin.o: /trance1/ARB/source/ARB/INCLUDE/ad_prot.h
AWTC_names_admin.o: /trance1/ARB/source/ARB/INCLUDE/aw_root.hxx
AWTC_names_admin.o: /trance1/ARB/source/ARB/INCLUDE/aw_device.hxx
AWTC_names_admin.o: /trance1/ARB/source/ARB/INCLUDE/aw_window.hxx
AWTC_names_admin.o: /trance1/ARB/source/ARB/INCLUDE/awt.hxx awtc_rename.hxx
AWTC_seq_search.o: /usr/include/memory.h /usr/include/features.h
AWTC_seq_search.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
AWTC_seq_search.o: /usr/include/stdio.h /usr/include/bits/types.h
AWTC_seq_search.o: /usr/include/libio.h /usr/include/_G_config.h
AWTC_seq_search.o: /usr/include/bits/stdio_lim.h /usr/include/stdlib.h
AWTC_seq_search.o: /usr/include/sys/types.h /usr/include/time.h
AWTC_seq_search.o: /usr/include/endian.h /usr/include/bits/endian.h
AWTC_seq_search.o: /usr/include/sys/select.h /usr/include/bits/select.h
AWTC_seq_search.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
AWTC_seq_search.o: /usr/include/alloca.h /usr/include/limits.h
AWTC_seq_search.o: /usr/include/bits/posix1_lim.h
AWTC_seq_search.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
AWTC_seq_search.o: /usr/include/bits/posix2_lim.h /usr/include/ctype.h
AWTC_seq_search.o: /usr/include/string.h
AWTC_seq_search.o: /trance1/ARB/source/ARB/INCLUDE/aw_root.hxx
AWTC_seq_search.o: /trance1/ARB/source/ARB/INCLUDE/arbdb.h
AWTC_seq_search.o: /trance1/ARB/source/ARB/INCLUDE/ad_prot.h
AWTC_seq_search.o: ./awtc_seq_search.hxx awtc_assert.hxx
AWTC_fast_aligner.o: /usr/include/stdio.h /usr/include/features.h
AWTC_fast_aligner.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
AWTC_fast_aligner.o: /usr/include/bits/types.h /usr/include/libio.h
AWTC_fast_aligner.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
AWTC_fast_aligner.o: /usr/include/stdlib.h /usr/include/sys/types.h
AWTC_fast_aligner.o: /usr/include/time.h /usr/include/endian.h
AWTC_fast_aligner.o: /usr/include/bits/endian.h /usr/include/sys/select.h
AWTC_fast_aligner.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
AWTC_fast_aligner.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
AWTC_fast_aligner.o: /usr/include/ctype.h /usr/include/memory.h
AWTC_fast_aligner.o: /usr/include/math.h /usr/include/bits/huge_val.h
AWTC_fast_aligner.o: /usr/include/bits/mathdef.h
AWTC_fast_aligner.o: /usr/include/bits/mathcalls.h /usr/include/string.h
AWTC_fast_aligner.o: /usr/include/limits.h /usr/include/bits/posix1_lim.h
AWTC_fast_aligner.o: /usr/include/bits/local_lim.h
AWTC_fast_aligner.o: /usr/include/linux/limits.h
AWTC_fast_aligner.o: /usr/include/bits/posix2_lim.h
AWTC_fast_aligner.o: /trance1/ARB/source/ARB/INCLUDE/arbdb.h
AWTC_fast_aligner.o: /trance1/ARB/source/ARB/INCLUDE/ad_prot.h
AWTC_fast_aligner.o: /trance1/ARB/source/ARB/INCLUDE/arbdbt.h
AWTC_fast_aligner.o: /trance1/ARB/source/ARB/INCLUDE/ad_t_prot.h
AWTC_fast_aligner.o: /trance1/ARB/source/ARB/INCLUDE/aw_root.hxx
AWTC_fast_aligner.o: /trance1/ARB/source/ARB/INCLUDE/aw_window.hxx
AWTC_fast_aligner.o: /trance1/ARB/source/ARB/INCLUDE/aw_awars.hxx
AWTC_fast_aligner.o: ./awtc_next_neighbours.hxx ./awtc_seq_search.hxx
AWTC_fast_aligner.o: awtc_assert.hxx awtc_ClustalV.hxx awtc_fast_aligner.hxx
AWTC_ClustalV.o: /usr/include/ctype.h /usr/include/features.h
AWTC_ClustalV.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
AWTC_ClustalV.o: /usr/include/bits/types.h /usr/include/endian.h
AWTC_ClustalV.o: /usr/include/bits/endian.h /usr/include/stdlib.h
AWTC_ClustalV.o: /usr/include/sys/types.h /usr/include/time.h
AWTC_ClustalV.o: /usr/include/sys/select.h /usr/include/bits/select.h
AWTC_ClustalV.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
AWTC_ClustalV.o: /usr/include/alloca.h /usr/include/memory.h
AWTC_ClustalV.o: /usr/include/malloc.h /usr/include/string.h
AWTC_ClustalV.o: /usr/include/stdio.h /usr/include/libio.h
AWTC_ClustalV.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
AWTC_ClustalV.o: /usr/include/limits.h /usr/include/bits/posix1_lim.h
AWTC_ClustalV.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
AWTC_ClustalV.o: /usr/include/bits/posix2_lim.h
AWTC_ClustalV.o: /trance1/ARB/source/ARB/INCLUDE/arbdb.h
AWTC_ClustalV.o: /trance1/ARB/source/ARB/INCLUDE/ad_prot.h
AWTC_ClustalV.o: /trance1/ARB/source/ARB/INCLUDE/aw_root.hxx
AWTC_ClustalV.o: awtc_ClustalV.hxx ./awtc_seq_search.hxx awtc_assert.hxx
AWTC_constructSequence.o: /usr/include/stdio.h /usr/include/features.h
AWTC_constructSequence.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
AWTC_constructSequence.o: /usr/include/bits/types.h /usr/include/libio.h
AWTC_constructSequence.o: /usr/include/_G_config.h
AWTC_constructSequence.o: /usr/include/bits/stdio_lim.h /usr/include/stdlib.h
AWTC_constructSequence.o: /usr/include/sys/types.h /usr/include/time.h
AWTC_constructSequence.o: /usr/include/endian.h /usr/include/bits/endian.h
AWTC_constructSequence.o: /usr/include/sys/select.h
AWTC_constructSequence.o: /usr/include/bits/select.h
AWTC_constructSequence.o: /usr/include/bits/sigset.h
AWTC_constructSequence.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
AWTC_constructSequence.o: /usr/include/memory.h /usr/include/ctype.h
AWTC_constructSequence.o: /usr/include/string.h /usr/include/sys/time.h
AWTC_constructSequence.o: /usr/include/bits/time.h
AWTC_constructSequence.o: /trance1/ARB/source/ARB/INCLUDE/arbdb.h
AWTC_constructSequence.o: /trance1/ARB/source/ARB/INCLUDE/ad_prot.h
AWTC_constructSequence.o: /trance1/ARB/source/ARB/INCLUDE/aw_root.hxx
AWTC_constructSequence.o: /trance1/ARB/source/ARB/INCLUDE/aw_window.hxx
AWTC_constructSequence.o: awtc_fast_aligner.hxx ./awtc_seq_search.hxx
AWTC_constructSequence.o: awtc_assert.hxx awtc_constructSequence.hxx
AWTC_submission.o: /usr/include/stdio.h /usr/include/features.h
AWTC_submission.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
AWTC_submission.o: /usr/include/bits/types.h /usr/include/libio.h
AWTC_submission.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
AWTC_submission.o: /usr/include/stdlib.h /usr/include/sys/types.h
AWTC_submission.o: /usr/include/time.h /usr/include/endian.h
AWTC_submission.o: /usr/include/bits/endian.h /usr/include/sys/select.h
AWTC_submission.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
AWTC_submission.o: /usr/include/sys/sysmacros.h /usr/include/alloca.h
AWTC_submission.o: /usr/include/memory.h /usr/include/string.h
AWTC_submission.o: /trance1/ARB/source/ARB/INCLUDE/arbdb.h
AWTC_submission.o: /trance1/ARB/source/ARB/INCLUDE/ad_prot.h
AWTC_submission.o: /trance1/ARB/source/ARB/INCLUDE/arbdbt.h
AWTC_submission.o: /trance1/ARB/source/ARB/INCLUDE/ad_t_prot.h
AWTC_submission.o: /trance1/ARB/source/ARB/INCLUDE/aw_root.hxx
AWTC_submission.o: /trance1/ARB/source/ARB/INCLUDE/aw_device.hxx
AWTC_submission.o: /trance1/ARB/source/ARB/INCLUDE/aw_window.hxx
AWTC_submission.o: /trance1/ARB/source/ARB/INCLUDE/aw_awars.hxx
