#!/usr/local/bin/perl

# Usage: njt2tpl [machine_readable_tree_with_branch_length]

while (<>) {
	next if /^\s*$/;
	s/:-?[0-9]+\.[0-9]+//g;
	print;
}
