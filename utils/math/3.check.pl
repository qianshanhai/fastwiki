#!/usr/bin/perl

$dir = shift;

while (<>) {
    if (/(.*?wiki)-(\d{8})/) {
        unless (-s "$dir/$1-$2-math.tmp.txt" > 0) {
            print;
        }
    }
}
