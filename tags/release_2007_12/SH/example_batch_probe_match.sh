#!/bin/bash
#
# This is an example script showing how to match multiple probes
# with one call.
#
# Start a shell using 'ARB_NTREE/Tools/Start XTERM' and run this 
# script from there.
#     
# Before running this script, start the targetted PT_Server from
# inside ARB - otherwise this script will not terminate for a long time.


# specify your probe match parameters here:    
PARAMS='serverid=2 matchmismatches=1'

# Note: Call 'arb_probe' from the command line to see other supported parameters.

match() {
	echo "Matching $1"
	arb_probe $PARAMS matchsequence=$1
}

match_all() {
        # add/modify lines below to suit your needs:
	match CCTCAGTACGAA
	match AACCGGTTAACC
	match ACTGACTGGGCU
	match ACCTGGACTTTT
}

match_all | sed -e 's/\x01/\x0a/g'

