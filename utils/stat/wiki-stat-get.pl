#!/usr/bin/perl

$out_dir = $ENV{WIKI_STAT_INDIR};
$out_tmp = $ENV{WIKI_STAT_TMP};

%hash = ();

init_hash();

while (<>) {
	chomp;
	($base_url, $fname) = split / /;

	if ($hash{$fname} == 1) {
		next;
	}
	if (wget($base_url, $fname)) {
		rename("$out_tmp/$fname", "$out_dir/$fname");
	}
	write_hash($fname);
}

sub init_hash {
	open FH, "wget.hash" or die $1;
	while (<FH>) {
		chomp;
		$hash{$_} = 1;
	}
	close FH;
}

sub write_hash {
	my $fname = shift;
	open FH, ">>wget.hash" or die $!;
	print FH "$fname\n";
	close FH;
}

sub wget {
	my ($url, $f) = @_;
	my $file = "$out_tmp/$f";
	unlink "wget-log";
	system("wget -b -c $url/$f -O $file");
	my $try = 0;
	while (1) {
		$old_size = -s $file;
		sleep(2);
		$try++;
		$new_size = -s $file;
		if ($new_size == $old_size) {
			if (check_wget_log("wget-log")) {
				return 1;
			}
			if ($try > 5 and $new_size == 0) {
				return 0;
			}
			if ($try > 30) {
				return 0;
			}
		}
	}
}

sub check_wget_log {
	my $file = shift;
	my $n = 0;
	open FH, $file or die $!;
	while (<FH>) {
		if (/ 100\% /) {
			$n = 1;
			last;
		}
	}
	close FH;
	return $n;
}
