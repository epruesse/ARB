#!/usr/local/bin/perl

# Copyright (C) 1993-1995 J. Adachi,  All rights reserved.  Mar 08 1994

# Usage: egetcds [EMBL_file]

while(<>) { 
			if (/^ID/) {
				($id) = /^ID\s+(\w+)/;
			} elsif (/^AC/) {
				($ac) = /^AC\s+(\w+)/;
			} elsif (/^DE/) {
				($de) = /^DE\s+(\w.+)$/ if $de eq "";
			} elsif (/^OS/) {
				($os1) = /^OS\s+(\w+)/;
				($os2) = /^OS\s+\w+\s+(\w+)/;
				($os3) = /^OS\s+\w+\s+\w+\s(\w+)/;
				$name  = substr($os1, 0, 3);
				$name .= substr($os2, 0, 2);
				$name .= $os3;
			} elsif (/^FT/) {
				if ( /^FT\s+CDS\s+(\S.+)$/ ) {
					chop;
					($temp1) = $1;
					$cds .= "," if ($cds ne "");
					$cds .= $temp1;
					while(/,$/) {
						$_ = <ARGV>;
						chop;
						($temp2) = /^FT\s+(\w.+)$/;
						$cds .= $temp2;
					}
				}
			} elsif (/^SQ/) {
				$seq = "";
				while(<>) {
					last if /^\/\//;
					s/\s+//g;
					s/\d+//g;
					$seq .= $_;
				}
				$cds =~ s/^\D+//;
				$cds =~ s/\)//;
				$cds =~ s/\<//;
				$cds =~ s/\>//;
				#print $cds,"\n";
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
				print $name," ",$len," ",$os1," ",$os2," # ",$ac,"\n";
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
