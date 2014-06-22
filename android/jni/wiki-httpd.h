#ifndef __WIKI_FASTWIKI_HTTPD_H
#define __WIKI_FASTWIKI_HTTPD_H

#define WIKI_START_HTML \
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n" \
"<html>\n" \
"	<head>\n" \
"	<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf8\">\n" \
"	<meta http-equiv=\"CACHE-CONTROL\" content=\"NO-CACHE\">\n" \
"	<meta http-equiv=\"PRAGMA\" content=\"NO-CACHE\"> \n" \
"\n" \
"	<style type=\"text/css\">\n" \
"		body {\n" \
"			background:%s;\n" \
"			color:%s;\n" \
"			font-size:%dpt;\n" \
"		}\n" \
"		ul {\n" \
"			margin-top:0px;\n" \
"			margin-left:10px;\n" \
"			padding:6px;\n" \
"		}\n" \
"		ol {\n" \
"			margin-top:0px;\n" \
"			margin-left:26px;\n" \
"			padding:6px;\n" \
"		}\n" \
"		img {\n" \
"			height:auto;\n" \
"			width:auto;\n" \
"		}\n" \
"		a {\n" \
"			color:%s\n" \
"		}\n" \
"		a:link {\n" \
"			text-decoration: none;\n" \
"		}\n" \
"		a:visited {\n" \
"			text-decoration: none;\n" \
"		}\n" \
"		a:hover {\n" \
"			text-decoration: none;\n" \
"		}\n" \
"	</style>\n" \
"	<script language=javascript>\n" \
"		function set_font_size(n)\n" \
"		{\n" \
"			if (n > 3) {\n" \
"				document.body.style.fontSize = n + \"pt\";\n" \
"			}\n" \
"		}\n" \
"\n" \
"	function $(x) { return document.getElementById(x); }\n" \
"\n" \
"	function http_get(s)\n" \
"	{\n" \
"		var xmlHttp = new XMLHttpRequest();\n" \
"		xmlHttp.open( \"GET\", \"http://127.0.0.1:%d/\" + s, false);\n" \
"		xmlHttp.send(null);\n" \
"		return xmlHttp.responseText;\n" \
"	}\n" \
"\n" \
"	function last_pos()\n" \
"	{\n" \
"		var x = %d;\n" \
"		var o_h = %d;\n" \
"		var h = document.body.scrollHeight;\n" \
"\n" \
"		if (x > 20) {\n" \
"			document.body.scrollTop=(x *h /o_h);\n" \
"		}\n" \
"	}\n" \
"	function set_pos()\n" \
"	{\n" \
"		var x = document.body.scrollTop || window.pageYOffset || document.documentElement.scrollTop;\n" \
"\n" \
"		android.real_set_pos(x, document.body.scrollHeight, document.documentElement.clientHeight);\n" \
"	}\n" \
"\n" \
"	function get_data()\n" \
"	{\n" \
"		var id;\n" \
"		var m = %d;\n" \
"\n" \
"		for (var i = 0; i < m; i++) {\n" \
"			id = \"main\" + i;\n" \
"			$(id).innerHTML = http_get(\"curr:\" + i);\n" \
"		}\n" \
"		last_pos();\n" \
"	}\n" \
"\n" \
"	function next_page()\n" \
"	{\n" \
"		var x = document.body.scrollTop || window.pageYOffset || document.documentElement.scrollTop;\n" \
"		var y = document.body.scrollTop + document.documentElement.clientHeight - 20;\n" \
"\n" \
"		document.body.scrollTop = y > 0 ? y : 0;\n" \
"\n" \
"		if (document.body.scrollTop + document.documentElement.clientHeight >= document.body.scrollHeight) {\n" \
"			android.has_last_page();\n" \
"		}\n" \
"\n" \
"		android.real_set_pos(y, document.body.scrollHeight, document.documentElement.clientHeight);\n" \
"	}\n" \
"\n" \
"	function back_page()\n" \
"	{\n" \
"		var x = window.pageYOffset||document.documentElement.scrollTop||document.body.scrollTop; \n" \
"		var y = document.body.scrollTop - window.screen.availHeight + 20;\n" \
"		document.body.scrollTop = y > 0 ? y : 0;\n" \
"		android.real_set_pos(y, document.body.scrollHeight, window.screen.availHeight);\n" \
"	}\n" \
"\n" \
"</script>\n" \
"</head>\n" \
"\n" \
"<body>\n" \
"\n" \


#define WIKI_HTTP_END_HTML \
"\n" \
"<div id=main0></div>\n" \
"<div id=main1></div>\n" \
"<div id=main2></div>\n" \
"<div id=main3></div>\n" \
"<div id=main4></div>\n" \
"<div id=main5></div>\n" \
"<div id=main6></div>\n" \
"<div id=main7></div>\n" \
"<div id=main8></div>\n" \
"<div id=main9></div>\n" \
"<div id=main10></div>\n" \
"<div id=main11></div>\n" \
"<div id=main12></div>\n" \
"<div id=main13></div>\n" \
"<div id=main14></div>\n" \
"<div id=main15></div>\n" \
"<div id=main16></div>\n" \
"<div id=main17></div>\n" \
"<div id=main18></div>\n" \
"<div id=main19></div>\n" \
"<div id=main20></div>\n" \
"\n" \
"</body>\n" \
"</html>\n" \
"\n" \




#endif
