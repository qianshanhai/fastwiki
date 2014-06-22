#!/usr/bin/perl
use Encode;

$lang = q//;

%tmp = ();
%first = ();
@all = ();

$flag = 0;

$idx = 0;

open OUT, ">wiki_local_auto.h" or die $!;
open FH, "msg.txt" or die $!;

print OUT "#ifndef __WIKI_LOCAL_AUTO\n#define __WIKI_LOCAL_AUTO\n\n";

$line = q//;

while (<FH>) {
	chomp;
	s/^\s+//g;
	next if /^$/;

	if (/^#(.*)/) {
		output();
		$lang = $1;
		%tmp = ();
		next;
	}

	$_ = encode_utf8(decode("gb2312", $_));

	if (/(.*?)\s+(".*?")/) {
		$tmp{$1} = $2;
		if ($flag == 0) {
			$first{$1} = $idx++;
		}
	}
}

output();

$line .= "#define ALL_LANG \\\n";
for (@all) {
	$line .= "\t{\"$$_[0]\", $$_[1]}, \\\n";
}
$line .= "\t{NULL, NULL}\n\n";

#print "enum { \n";
#for (sort { $first{$a} <=> $first{$b} } keys %first) {
#	print "\t$_,\n";
#}
#print "};\n\n";

print OUT $line;

print OUT "#endif\n";

sub output {
	return if $lang eq "";

	my $name = "__local_$lang";

	push @all, [$lang, $name];
	$line .= "struct local $name\[\] = { \n";

	for (sort { $first{$a} <=> $first{$b} } keys %tmp) {
		$line .= "\t{\"$_\", $tmp{$_}}, \n";
	}
	$line .= "\t{NULL, NULL} \n};\n\n";

	$flag = 1;
}
