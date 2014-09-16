#ifndef __FASTWIKI_HTTPD_H
#define __FASTWIKI_HTTPD_H

#define HTTP_START_HTML \
"<html>\n" \
"	<head>\n" \
"		<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\">\n" \
"		<style>\n" \
"		 .xx{background-color:white;}\n" \
"		 a:link {\n" \
"			 text-decoration: none;\n" \
"		 }\n" \
"		 </style>\n" \
"		 <title>Fastwiki</title>\n" \
"		<script language=javascript>\n" \
"			function $(x) { return document.getElementById(x); }\n" \
"\n" \
"			function http_get(s)\n" \
"			{\n" \
"				var xmlHttp = CreateAJAX();\n" \
"				xmlHttp.open( \"GET\", \"/\" + s, false);\n" \
"				xmlHttp.send(null);\n" \
"				return xmlHttp.responseText;\n" \
"			}\n" \
"\n" \
"			function CreateAJAX()\n" \
"			{\n" \
"				if (typeof(XMLHttpRequest) != \"undefined\")\n" \
"					return new XMLHttpRequest();\n" \
"\n" \
"				if(window.ActiveXObject){\n" \
"					var objs=[\n" \
"						\"MSXML2.XMLHttp.5.0\",\n" \
"						\"MSXML2.XMLHttp.4.0\",\n" \
"						\"MSXML2.XMLHttp.3.0\",\n" \
"						\"MSXML2.XMLHttp\",\n" \
"						\"Microsoft.XMLHTTP\"\n" \
"					];\n" \
"					var xmlhttp;\n" \
"					for (var i = 0; i < objs.length; i++){\n" \
"						try{\n" \
"							xmlhttp = new ActiveXObject(objs[i]);\n" \
"							return xmlhttp;\n" \
"						} catch(e){\n" \
"						} \n" \
"					}\n" \
"				}  \n" \
"			}\n" \
"\n" \
"			function get_data(va)\n" \
"			{\n" \
"				var s;\n" \
"				if (this.f1.ft.checked) {\n" \
"					$(\"test\").style.border = \"solid 0px black\";\n" \
"					$(\"test\").style.display = \"none\"; \n" \
"					$(\"test\").innerHTML = \"\";\n" \
"					return;\n" \
"				}\n" \
"\n" \
"				va = $(\"in1\").value;\n" \
"\n" \
"				if (va == \"\") {\n" \
"					$(\"test\").innerHTML = \"\";\n" \
"					$(\"test\").style.border = \"solid 0px black\";\n" \
"					return;\n" \
"				}\n" \
"				\n" \
"				$(\"test\").style.display = \"block\"; \n" \
"				$(\"test\").style.border = \"solid 1px black\";\n" \
"				\n" \
"				s = http_get(\"match?key=\" + va);\n" \
"				$(\"test\").innerHTML = s;\n" \
"			}\n" \
"\n" \
"			function _submit()\n" \
"			{\n" \
"				if (this.f1.ft.checked) {\n" \
"					document.location = \"/search?key=\" + $(\"in1\").value + \"&ft=on\";\n" \
"					return;\n" \
"				}\n" \
"\n" \
"				get_data($(\"in1\").value);\n" \
"			}\n" \
"\n" \
"			function get(url)\n" \
"			{\n" \
"				var s;\n" \
"\n" \
"				$(\"test\").style.border = \"solid 0px black\";\n" \
"\n" \
"				s = http_get(url);\n" \
"				$(\"test\").innerHTML = \"\";\n" \
"				$(\"main\").innerHTML = s;\n" \
"			}\n" \
"\n" \
"			document.onkeydown = function(e){\n" \
"				e = e || event;\n" \
"				if (e.keyCode == 27) {\n" \
"					$(\"test\").style.border = \"solid 0px black\";\n" \
"					$(\"test\").style.display = \"none\"; \n" \
"				}\n" \
"			};\n" \
"\n" \
"		</script>\n" \
"	</head>\n" \
"	<body>\n" \
"		<form method=GET action=search name=f1>\n" \
"			<table border=0>\n" \
"				<tr><td> Fastwiki: </td>\n" \
"					<td><input id=in1 type=text name=key autocomplete=\"off\" size=40 >\n" \
"						<input type=button onclick=\"_submit()\" value=submit> \n" \
"						(Full Text Search: <input type=\"checkbox\" name=ft %s />)\n" \
"						<br/>\n" \
"					</td>\n" \
"				</tr>\n" \
"				<tr><td>&nbsp;</td>\n" \
"					<td>\n" \
"						<div id=test class=xx style=\"display:block;position:absolute;\"></div>\n" \
"					</td>\n" \
"				</tr>\n" \
"			</table>\n" \
"		</form>\n" \
"\n" \
"		<div id=main>\n" \


#define HTTP_END_HTML \
"		</div>\n" \
"\n" \
"		<script language=javascript>\n" \
"			/*\n" \
"			if (navigator.userAgent.indexOf(\"MSIE\") > 0){\n" \
"				$('in1').onpropertychange = function(){\n" \
"					get_data(this.value);\n" \
"				};\n" \
"			}\n" \
"			*/\n" \
"\n" \
"			$('in1').oninput = function(){\n" \
"				get_data(this.value);\n" \
"			};\n" \
"\n" \
"			$('in1').value = \"%s\";\n" \
"		</script>\n" \
"</body>\n" \
"</html>\n" \




#endif
