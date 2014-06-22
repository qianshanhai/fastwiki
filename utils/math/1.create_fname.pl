#!/usr/bin/perl

use LWP;
use LWP::UserAgent;

my $all_date_file = "all-date.txt";
my %date_hash = ();
my $curr_date = curr_date();

my $ua = LWP::UserAgent->new;
$ua->agent("Mozilla/5.0 ");
$ua->timeout(10);
$ua->proxy(['http'], 'http://172.20.23.4:60123');

init_max_date();

$fmath = "$curr_date/fmath.sh";
mkdir $curr_date, 0755;
mkdir "$curr_date/math", 0755;

while (<>) {
	chomp;
	next if $_ eq //;
	$lang = $_ . "wiki";
	$res = $ua->get("http://dumps.wikimedia.org/$lang/");
	if ($res->is_success) {
		my $date = parse($res->decoded_content);
		next if $date le $date_hash{$lang};
		my $fname = "$lang-$date-pages-articles.xml.bz2"; 
		my $base = "http://dumps.wikimedia.org/$lang/$date";
		my $url = "$base/$fname";
		LOG("$curr_date/fname.txt", "$fname\n");
		LOG($fmath, "wget -c $url\n");
		LOG($fmath, "/root/math/fastwiki-math 0 $fname $lang-$date-math.tmp.txt\n");
		LOG($fmath, "rm -f $fname\n");
		LOG($all_date_file, "$lang\t$date\n");
	}
}

sub parse {
	my $m = shift;
	my $date = q//;

	while ($m =~ s|<a href="(\d{8})/">(\d{8})</a>||) {
		if ($1 eq $2) {
			if ($1 gt $date) {
				$date = $1;
			}
		}
	}

	return $date;
}

sub LOG {
	my $file = shift;
	my $m = shift;
	open FH, ">>$file" or die $!;
	print FH $m;
	close FH;
}

sub init_max_date {
	open FH, $all_date_file or die $!;
	while (<FH>) {
		chomp;
		my @m = split /\s+/;
		if ($m[1] gt $date_hash{$m[0]}) {
			$date_hash{$m[0]} = $m[1];
		}
	}
	close FH;
}

sub curr_date {
	my @m = ();
	@m[0..5] = localtime(shift || time);  # 只取前面 6 个元素
	@m[4, 5] = (++$m[4], $m[5] + 1900);
	sprintf qq/%04d%02d%02d/, reverse @m;
}
