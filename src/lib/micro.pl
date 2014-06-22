#!/usr/bin/perl

# used to produce some micro

if (@ARGV != 2) {
	print "usage: perl $0 <var> <string>\n"
		. "       perl $0 p '<page>'\n";
	exit;
}

$x = shift;
$y = shift;

@t = split //, $y;
$i = 0;

for (@t) {
	$lower = lc($_);
	$upper = uc($_);
	if ($lower eq $upper) {
		$tmp = qq/${i}\[$x\] == '$lower' \n/;
	} else {
		$tmp = qq/(${i}\[$x\] == '$lower' || ${i}\[$x\] == '$upper') \n/;
	}
	if ($line eq "") {
		$line = $tmp;
	} else {
		$line .= qq/\t\t&& $tmp/;
	}
	$i++;
}

print "$line\n";
