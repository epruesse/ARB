#!/usr/local/bin/perl

# Copyright (C) 1993-1995 J. Adachi,  All rights reserved.  Jun 06 1995

# Usage: egetcds [Genbank_file]

while(<>) { 
			if (/^LOCUS/) {
				($lo) = /^LOCUS\s+(\w+)/;
			} elsif (/^DEFINITION/) {
				($de) = /^DEFINITION\s+(\w.+)$/;
			} elsif (/^ACCESSION/) {
				($ac) = /^ACCESSION\s+(\w+)/;
			} elsif (/^KEYWORDS/) {
				($kw) = /^KEYWORDS\s+(\w+)/;
			} elsif (/^SOURCE/) {
				($so) = /^SOURCE\s+(\w.+)$/;
			} elsif (/^\s*ORGANISM\s/) {
				($or1) = /^\s*ORGANISM\s+(\w+)/;
				($or2) = /^\s*ORGANISM\s+\w+\s+(\w+)/;
				($or3) = /^\s*ORGANISM\s+\w+\s+\w+\s(\w+)/;
				$name  = substr($or1, 0, 3);
				$name .= substr($or2, 0, 2);
				$name .= $os3;
			} elsif (/^\s*CDS\s/) {
				if ( /^\s+CDS\s+(\S.+)$/ ) {
					chop;
					($temp1) = $1;
					$cds .= "," if ($cds ne "");
					$cds .= $temp1;
					while(/,$/) {
						$_ = <>;
						chop;
						($temp2) = /^\s+(\w.+)$/;
						$cds .= $temp2;
					}
				}
			} elsif (/^ORIGIN/) {
				$seq = "";
				while(<>) {
					last if /^\/\//;
					s/\s+//g;
					s/\d+//g;
					$seq .= $_;
				}
				$seq =~ tr/tcag/TCAG/;
				#print "#", $cds,"\n";
				$cds =~ s/[^\d.,]//g;
				#print "#", $cds,"\n";
				@codeing = split(',',$cds);
				foreach $code (@codeing) {
					($begin,$end) = split('\.\.',$code);
					#print $begin," ",$end,"\n";
					$begin--;
					$subseq .= substr($seq, $begin, $end-$begin);
				}
				$subseq = $seq if $cds eq "";
				$subseq =~ tr/acgt/ACGT/; # 1995.11.15 added
				$len = length($subseq);

				print "# ",$de,"\n";
				print $name," ",$or1," ",$or2," # ",$ac," ", $so, "\n";
				print $len, "\n";
				$offset = 0;
				while ($line = substr($subseq, $offset, 60)) {
					print $line, "\n";
					$offset += 60;
				}
				$subseq = "";
				$cds = "";
				$de = "";
				print "\n";
			}
}
