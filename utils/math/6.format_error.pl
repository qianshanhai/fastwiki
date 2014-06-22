# PUB: 20130219/math/fastwiki.math.az.20130215 count=1606, error=32
# usage: perl 6.format_error.pl log.1 > math_count_error.txt

while (<>) {
	if (/^PUB.*?(fastwiki.*?)\s+count=(\d+).*?error=(\d+)/) {
		printf "%-32s total:%-8s error:%-8s success=%.2f%%\r\n",
		$1, $2, $3, $2 <= 0 ? 100 : ($2-$3) * 100 / $2;
	}
}
