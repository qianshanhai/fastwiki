#!/usr/bin/perl

#zh2Hans
#zh2CN

# use to produce wiki_zh_auto.h
# input: ZhConversion.php , this file in mediawiki/includes

# usage: perl convert_zh.pl ZhConversion.php > wiki_zh_auto.h

if (@ARGV != 1) {
	print "usage: perl $0 ZhConversion.php > wiki_zh_auto.h\n";
	exit;
}

@hans = ();

while (<>) {
	if (/zh2Hans|zh2CN/) {
		while (<>) {
			last if /^\);/;
			if (/'(.*?)'.*?'(.*?)'/) {
				($x, $y, $n) = ($1, $2, length($1));
				push @hans, [$n, qq/\t"$x\t$y\\n"/];
			}
		}
	}
}

$total = @hans;

print "#ifndef __WIKI_ZH_AUTO_H\n#define __WIKI_ZH_AUTO_H\n\n";
print "#define __ZH_CONVERT_2HANS \\\n";

print qq{\t"$total\\n" \\\n};

for (sort { $a->[0] <=> $b->[0] } @hans) {
	print $_->[1], " \\\n";
}

print qq{\t"\\x0" \n\n};
print "\n#endif\n";
