// index.html page for Web server

#ifndef __INDEX_HTML_H__
#define __INDEX_HTML_H__

#ifndef WEBSERVER
#error "This page can only be included from hpptd.c"
#endif

static char http_index_template[] = 
  "<html><head>"
  "<meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-8859-1\">"
  "<title>Moonlight</title>"
  "<style type=\"text/css\">"
  "<!--td {font-size: 10pt;}-->"
  "</style>"
  "</head><body topmargin=\"40\" bgcolor=\"#fefeef\">"
  "<table align=\"center\" bgcolor=\"#999999\" border=\"0\" cellpadding=\"1\" "
  "cellspacing=\"1\" width=\"400\">"
  "  <tbody><tr>"
  "   <td align=\"center\" bgcolor=\"#f3f3f3\" height=\"32\" valign=\"center\">"
  "	  <font color=\"red\" size=\"+1\"><b>Moonlight configuration</b></font></td>"
  "  </tr>"
  "  <tr>"
  "    <td align=\"center\" bgcolor=\"#ffffff\"><br>"
  "      <form action=\"moonlight.cgi\" name=\"form1\" method=\"post\">"
  "      <table bgcolor=\"#666666\" border=\"0\" cellpadding=\"1\" cellspacing=\"1\" width=\"350\">"
  "        <tbody><tr>"
  "          <td bgcolor=\"#fefeef\"><table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"350\">"
  "              <tbody><tr>"
  "                <td align=\"center\" height=\"22\" width=\"150\">Source IP</td>"
  "                <td height=\"27\" width=\"200\"><input name=\"sip\" size=\"20\" value=\"$00$\" type=\"text\"></td>"
  "              </tr>"
  "              <tr>"
  "                <td align=\"center\" height=\"22\" width=\"150\">Gateway IP</td>"
  "                <td height=\"27\"><input name=\"gwip\" size=\"20\" value=\"$01$\" type=\"text\"></td>"
  "              </tr>"
  "              <tr>"
  "                <td align=\"center\" height=\"22\" width=\"150\">Subnet Mask</td>"
  "                <td height=\"27\"><input name=\"sn\" size=\"20\" value=\"$02$\" type=\"text\"></td>"
  "              </tr>"
  "              <tr>"
  "                <td align=\"center\" height=\"22\" width=\"150\">DNS Server IP</td>"
  "                <td height=\"27\"><input name=\"dns\" size=\"20\" value=\"$03$\" type=\"text\"></td>"
  "              </tr>"
  "              <tr>"
  "                <td align=\"center\" height=\"22\" width=\"150\">MAC Address</td>"
  "                <td height=\"27\"><input name=\"hwaddr\" size=\"20\" value=\"$04$\" type=\"text\"></td>"
  "              </tr>"
  "              <tr>"
  "                <td align=\"center\" height=\"22\" width=\"150\">Board name</td>"
  "                <td height=\"27\"><input name=\"bname\" type=\"text\" size=\"20\" value=\"$05$\"></td>"
  "              </tr>"
  "              <tr>"
  "                <td align=\"center\" height=\"22\" width=\"150\">Use DHCP</td>"
  "                <td height=\"27\"><input name=\"isdhcp\" $06$=\"\" type=\"checkbox\"></td>"
  "              </tr>"
  "                <tr><td align=\"center\" height=\"22\" width=\"150\">Username</td>"
  "                <td height=\"27\"><input name=\"username\" size=\"20\" value=\"$07$\" type=\"text\"></td>"
  "              </tr>"
  "              <tr>"
  "                <td align=\"center\" height=\"22\" width=\"150\">Password</td>"
  "                <td height=\"27\"><input name=\"password\" size=\"20\" value=\"$08$\" type=\"password\"></td>"
  "              </tr>"
  "            </tbody></table></td>"
  "        </tr>"
  "      </tbody></table>"
  "      <br>"
  "      	 <input value=\"Network Config\" type=\"submit\"> "
  "      </form>"
  "    </td>"
  "  </tr>"
  "</tbody></table>"
  "<br>"
  "<table align=\"center\" bgcolor=\"#999999\" border=\"0\" cellpadding=\"1\" cellspacing=\"1\" width=\"400\">"
  "  <tbody><tr>"
  "    <td align=\"center\" bgcolor=\"#f3f3f3\" height=\"32\" valign=\"center\">"
  "	<font color=\"red\" size=\"+1\"><b>Moonlight application info</b></font></td>"
  "  </tr>"
  "  <tr>"
  "    <td align=\"center\" bgcolor=\"#ffffff\"><br>"
  "      <table bgcolor=\"#666666\" border=\"0\" cellpadding=\"1\" cellspacing=\"1\" width=\"350\">"
  "        <tbody><tr>"
  "          <td bgcolor=\"#fefeef\"><table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"350\">"
  "              <tbody><tr>"
  "                <td align=\"center\" height=\"22\" width=\"150\">Application name</td>"
  "                <td height=\"27\" width=\"200\"><b>$09$</b></td>"
  "              </tr>"
  "              <tr>"
  "                <td align=\"center\" height=\"22\" width=\"150\">Application base URL</td>"
  "                <td height=\"27\"><b>$10$</b></td>"
  "              </tr>"
  "            </tbody></table></td>"
  "        </tr>"
  "      </tbody></table>"
  "	  <br>"
  "    </td>"
  "  </tr>"
  "</tbody></table>"
  "<p align=\"center\"><font size=\"+1\" color=\"red\">$11$</font></p>"
  "</body></html>";

// Main page parameters
enum
{
  HTTPD_PAR_SOURCE_IP,
  HTTPD_PAR_GATEWAY_IP,
  HTTPD_PAR_SUBNET_MASK,
  HTTPD_PAR_DNS_SERVER_IP,
  HTTPD_PAR_MAC_ADDRESS,
  HTTPD_PAR_BOARD_NAME,
  HTTPD_PAR_USE_DHCP,
  HTTPD_PAR_USERNAME,
  HTTPD_PAR_PASSWORD,
  HTTPD_PAR_APPNAME,
  HTTPD_PAR_APPURL,
  HTTPD_PAR_MESSAGE
};
  
#endif
  
