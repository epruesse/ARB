#!/bin/bash

FULLLIB=$1
SYMLIST=$2

# temp workaround for using gcc 4.8 on dwarf 3 system
( nm -C -l $FULLLIB > $SYMLIST ) 2>&1 | grep -v 'this reader only handles version 2 and 3 information'
if (( ${PIPESTATUS[0]} )); then false; else true; fi

