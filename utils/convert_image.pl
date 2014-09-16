#!/usr/bin/perl
# Author: qianshanhai
# usage: perl convert_image.pl <todir> <files...>

my $identify = "identify"; # must have this command
my $convert = "convert";   # must have this command

# replace m_x and m_y to you value
# it convert all image file to m_x*m_y
my $m_x = 480;
my $m_y = 800;

sub usage {
	print "usage: $0 <800x480> <todir> <files...>\n";
	exit;
}

if (@ARGV != 3) {
	usage();
}

my $xy = shift;

($m_x, $m_y) = get_x_y($xy);

$todir = shift;

unless (-d $todir) {
	usage();
}

sub get_x_y {
	my $f = shift;
	my @t = split /x/, $f;
	return ($t[0], $t[1]);
}

for my $file (@ARGV) {
	if ($file =~ /\.(png|jpg|jpeg)$/i) {
		my $type = $1;
		my $fname = $file;
		$fname =~ s|.*/||g;
		my $new_file = "$todir/$fname";

		my @ident = get_ident($file);
		if ($ident[0] == 0 || $ident[1] == 0) {
			print "warn: not ident $file\n";
			next;
		}
		my ($tx, $ty) = count_size(@ident);
		my $tmp = ".fw.image.$type";
		`$convert -resize ${tx}x$ty $file $tmp`;
		`$convert  -crop ${m_x}x$m_y+0+0 $tmp $new_file`;
		unlink $tmp;
	}
}

sub get_ident {
	my $file = shift;
	my $tmp = `$identify $file`;
	if ($tmp =~ /(\d+)x(\d+)/) {
		return ($1, $2);
	}
	return (0, 0);
}

sub count_size {
	my $y = int($m_x / ($_[0] / $_[1])) + 1;
	my $x = int($m_y / ($_[1] / $_[0])) + 1;

	if ($x > $m_x) { 
		return ($x, $m_y);
	} else {
		return ($m_x, $y);
	}
}
