#!/bin/bash

if [ "y$1" = "y" ]; then
	echo "usage: $0 <date>"
	exit;
fi

date=$1

dir="/data/hace/download/baidu/百度云/fastwiki/数学公式"

if [ ! -d $dir ]; then
	mkdir $dir/$date
fi

mv $date/math/fastwiki.math.* $dir/$date/
