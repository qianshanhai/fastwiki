#!/usr/bin/perl

open FH, "fastwiki-httpd.html" or die $!;

$flag = 0;

$start_html = q//;
$end_html = q//;

while (<FH>) {
	chomp;
	s/"/\\"/g;
	if (/SPLIT/) {
		$flag = 1;
		next;
	}
	if ($flag == 0) {
		$start_html .= qq/"$_\\n" \\\n/;
	} else {
		$end_html .= qq/"$_\\n" \\\n/;
	}
}

open OUT, ">fastwiki-httpd.h" or die $!;

print OUT "#ifndef __FASTWIKI_HTTPD_H\n#define __FASTWIKI_HTTPD_H\n\n";
print OUT "#define HTTP_START_HTML \\\n$start_html\n\n";
print OUT "#define HTTP_END_HTML \\\n$end_html\n\n";

print OUT "\n\n#endif\n";
