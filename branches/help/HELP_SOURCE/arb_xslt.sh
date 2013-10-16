#!/bin/bash

xslt() {
    set -x
    xsltproc "$@"
}

format_error_messages() {
    perl -pne 's/^compilation\s+error:\s+file\s+([^\s]+)\s+line\s+([0-9]+)\s+/\1:\2: Error: /'
}
        
xslt "$@" 2>&1 | format_error_messages
exit ${PIPESTATUS[0]}
