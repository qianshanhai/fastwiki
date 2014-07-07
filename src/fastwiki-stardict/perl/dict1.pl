# fastwiki-stardict perl script
# $title  : current title
# $content: current content 
# find_title($t) : find title if exist, and return index

@t = split /\n/, $content;
$content = "<b>$title</b> ";
$t[0] =~ s/^\*//;
for (@t) {
	chomp;
	if (/^(\s*)([a-zA-Z].*?)\s*$/) {
		if (($n = find_title($2)) >= 0) {
			$content .= "$1<a href='$n#$2'>$2</a><br/>\n"; 
			next;
		}
	}
	$content .= "$_<br/>\n";
}
