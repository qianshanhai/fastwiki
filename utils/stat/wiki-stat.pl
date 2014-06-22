@list = ();

@tmp = ();

for $year (2008 .. 2012) {
	for $month (1 .. 12) {
		push @tmp, [$year, $month];
	}
}

for $month (1 .. 4) {
	push @tmp, [2013, $month];
}

for (@tmp) {
	($year, $month) = @$_;

	$max_day = 31;
	if ($month == 2) {
		$max_day = 28;
	} elsif ($month == 4 || $month == 6 || $month == 9 || $month == 11) {
		$max_day = 30;
	}

	for $day (1 .. $max_day) {
		for $hour (0 .. 23) {
			$m = sprintf "%02d", $month;
			$base_url = "http://dumps.wikimedia.org/other/pagecounts-raw/$year/$year-$m";
			$fname = sprintf "pagecounts-$year$m%02d-%02d0000.gz", $day, $hour;
			push @list, [$base_url, $fname];
		}
	}
}

for (sort { $b->[1] cmp $a->[1] } @list) {
	($base_url, $fname) = @$_;
	print "$base_url $fname\n";
}
