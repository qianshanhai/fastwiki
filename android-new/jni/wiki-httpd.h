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
"		sup.reference {unicode-bidi:\"-moz-isolate\";unicode-bidi:\"-webkit-isolate\";unicode-bidi:\"isolate\"}\n" \
"		ol.references > li:target,sup.reference:target,cite:target{background-color:#DEF}sup.reference{font-weight:normal;font-style:normal}\n" \
"		sup.reference a{white-space:nowrap}\n" \
"		table.navbox{border:1px solid #aaa;width:100%%;margin:auto;clear:both;font-size:88%%;text-align:center;padding:1px}\n" \
"		table.navbox + table.navbox{margin-top:-1px}\n" \
"		.navbox-title,.navbox-abovebelow,table.navbox th{text-align:center;padding-left:1em;padding-right:1em}\n" \
"		.navbox-group{white-space:nowrap;text-align:right;font-weight:bold;padding-left:1em;padding-right:1em}\n" \
"		.navbox,.navbox-subgroup{background:#fdfdfd}\n" \
"		.navbox-list{border-color:#fdfdfd}\n" \
"		.navbox-title,table.navbox th{background:#ccccff}\n" \
"		.navbox-abovebelow,.navbox-group,.navbox-subgroup .navbox-title{background:#ddddff}\n" \
"		.navbox-subgroup .navbox-group,.navbox-subgroup .navbox-abovebelow{background:#e6e6ff}\n" \
"		.navbox-even{background:#f7f7f7}\n" \
"		.navbox-odd{background:transparent}\n" \
"		.collapseButton{float:right;font-weight:normal;text-align:right;width:auto}\n" \
"		.navbox .collapseButton{width:6em}\n" \
"		div.thumbinner{border:1px solid #cccccc;padding:3px !important;background-color:White;font-size:94%%;text-align:center;overflow:hidden}\n" \
"\n" \
"		body {\n" \
"			%s\n" \
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
"\n" \
"	function goto_top(last_pos, last_height)\n" \
"	{\n" \
"		var x = document.body.scrollTop || window.pageYOffset || document.documentElement.scrollTop;\n" \
"		var h = document.body.scrollHeight;\n" \
"\n" \
"		if (x == 0) {\n" \
"			document.body.scrollTop = last_pos * last_height / h;\n" \
"		} else {\n" \
"			document.body.scrollTop = 0;\n" \
"		}\n" \
"	}\n" \
"\n" \
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
"</script>\n" \
"</head>\n" \
"\n" \
"<body %s>\n" \
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
"<script language=javascript>\n" \
"	var timeOutEvent=0;\n" \
"	function gtouchstart(v){  \n" \
"		timeOutEvent = setTimeout(\"longPress(\\\"\" + v + \"\\\")\", 500);\n" \
"		return false;  \n" \
"	}  \n" \
"	function gtouchend(){  \n" \
"		clearTimeout(timeOutEvent);\n" \
"		if(timeOutEvent!=0){  \n" \
"		}  \n" \
"		return false;  \n" \
"	}  \n" \
"	function gtouchmove(){  \n" \
"		clearTimeout(timeOutEvent);\n" \
"		timeOutEvent = 0;  \n" \
"	}  \n" \
"\n" \
"	function longPress(v){  \n" \
"		timeOutEvent = 0;  \n" \
"		android.translate(v);\n" \
"	}  \n" \
"\n" \
"	var d = document.getElementsByTagName(\"k\");\n" \
"\n" \
"	for (var i = 0; i < d.length; i++) {\n" \
"		d[i].ontouchstart = touchHandler;\n" \
"		d[i].ontouchend =  touchHandler;\n" \
"		d[i].ontouchmove = touchHandler;\n" \
"	}\n" \
"\n" \
"	function touchHandler(e) {\n" \
"		if (e.type == \"touchstart\") {\n" \
"			gtouchstart(e.target.innerHTML);\n" \
"		} else if (e.type == \"touchmove\") {\n" \
"			gtouchmove();\n" \
"		} else if (e.type == \"touchend\") {\n" \
"			gtouchend();\n" \
"		}\n" \
"	}\n" \
"\n" \
"	var last_top = document.body.scrollTop || window.pageYOffset || document.documentElement.scrollTop;\n" \
"\n" \
"	function super_bookmark()\n" \
"	{\n" \
"		var curr_top = document.body.scrollTop || window.pageYOffset || document.documentElement.scrollTop;\n" \
"		if (curr_top != last_top) {\n" \
"			android.debug(\"pos: \" + curr_top + \"\\n\");\n" \
"			set_pos();\n" \
"			last_top = curr_top;\n" \
"		}\n" \
"		setTimeout(\"super_bookmark()\", 5000);\n" \
"	}\n" \
"\n" \
"	setTimeout(\"super_bookmark()\", 5000);\n" \
"\n" \
"</script>\n" \
"\n" \
"</body>\n" \
"</html>\n" \
"\n" \




#endif
