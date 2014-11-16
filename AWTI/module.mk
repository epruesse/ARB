module:=awti
awti_TARGETS :=$(dir)/AWTI.a
awti_LDADD   := genom_import gui_aliview seqio
awti_HEADERS += awti_export.hxx awti_import.hxx
