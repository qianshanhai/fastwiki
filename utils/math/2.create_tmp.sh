#!/bin/bash

if [ "y$1" = "y" ]; then
	echo "usage: $0 <date>"
	exit;
fi

date=$1
prog=/root/math/fastwiki-math

cd $date

for i in `cat fname.txt`; do
	lang=`perl -e '$_=shift;s/wiki-.*//;print $_' $i`
	wiki_date=`perl -e '$_=shift;/wiki-(\d{8})/;print $1' $i`
	tmp="${lang}wiki-$wiki_date-math.tmp.txt"
	if [ ! -f $tmp ]; then
		wget -c http://dumps.wikimedia.org/${lang}wiki/$wiki_date/$i
		$prog 0 $i $tmp
		rm -f $i
	fi
done

