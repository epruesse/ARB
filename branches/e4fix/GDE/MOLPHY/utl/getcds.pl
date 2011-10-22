#!/usr/local/bin/perl

# Usage: getcds [EMBL_file]

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
		while(<ARGV>) {
			last if /^\/\//;
			s/\s+//g;
			s/\d+//g;
			$seq .= $_;
		}
	}

	if (eof) {
		$cds =~ s/^\D+//;
		$cds =~ s/\)//;
		$cds =~ s/\<//;
		$cds =~ s/\>//;
#		print $cds,"\n";
		@codeing = split(',',$cds);
		foreach $code (@codeing) {
			($begin,$end) = split('\.\.',$code);
#			print $begin," ",$end,"\n";
			$begin--;
			$subseq .= substr($seq, $begin, $end-$begin);
		}
		$subseq = $seq if $cds eq "";
		$subseq =~ tr/acgt/ACGT/; # 1995.11.15 added
		$len = length($subseq);
		print "# ",$de,"\n";
		print $id," ",$len,"  ",$os1," ",$os2,"  ",$ac,"\n";
		$offset = 0;
		while ($line = substr($subseq, $offset, 60)) {
			print $line, "\n";
			$offset += 60;
		}
		$subseq = "";
		$cds = "";
		$de = "";
#		$offset = 0;
#		while ($line = substr($seq, $offset, 60)) {
#			print $line, "\n";
#			$offset += 60;
#		}
#		print "\n";
	}

}
