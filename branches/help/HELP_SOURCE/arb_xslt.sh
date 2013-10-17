#!/bin/bash

xslt() {
    set -x
    xsltproc "$@"
}

format_error_messages() {
    perl -pne 's/^compilation\s+error:\s+file\s+([^\s]+)\s+line\s+([0-9]+)\s+/\1:\2: Error: /'
}
        
( xslt "$@" ) 2> >( format_error_messages >&2 )
exit ${PIPESTATUS[0]}
