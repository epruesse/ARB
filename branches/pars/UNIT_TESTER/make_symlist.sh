#!/bin/bash
FULLLIB=$1
SYMLIST=$2

# nm is unable to demangle names on os x. use c++filt instead for the job
DUMP_SYMBOLS="nm -l $FULLLIB | c++filt > $SYMLIST"

# temp workaround for using gcc 4.8 on dwarf 3 system
( eval "$DUMP_SYMBOLS" ) 2>&1 \
    | grep -v "Dwarf Error: Invalid or unhandled FORM value" \
    | grep -v "this reader only handles version 2 and 3 information" \
    | grep -v "this reader can not handle sizes greater than '8'" \
    | grep -v "this reader can only handle address sizes '2', '4' and '8'" \

if (( ${PIPESTATUS[0]} )); then false; else true; fi

