/***********************************************************************************/
/*
    This file is part of TUMessenger.

    TUMessenger is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    TUMessenger is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with TUMessenger.  If not, see <http://www.gnu.org/licenses/>.
*/
/***********************************************************************************/

#include <wx/wx.h>
#include <wx/file.h>
#include <curl/curl.h>
#include "TUConnection.h"
#include "TUBuddylist.h"
#include "TUExFuncs.h"

static int copy_to_string(char *data, size_t size, size_t nmemb, std::string *buffer)
{
  int result = 0;
  if (buffer != NULL)
  {
    buffer->append(data, size * nmemb);
    result = size * nmemb;
  }
  return result;
}

int tu_login(CURLSH *share, wxString username, wxString password)
{
    std::string buffer;
    CURL *tusock = curl_easy_init();

    char *temp2 = curl_easy_escape(tusock, password.mb_str(wxConvUTF8), 0);
    wxString PassConverted(temp2, wxConvUTF8);
    free(temp2);

    wxString login = wxT("benutzer=") + username + wxT("&passwort=") + PassConverted;

    std::string temp = std::string(login.mb_str());

    curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);

    curl_easy_setopt(tusock, CURLOPT_SHARE, share);
    curl_easy_setopt(tusock, CURLOPT_URL, "https://m.team-ulm.de/login.php");
    curl_easy_setopt(tusock, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(tusock, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(tusock, CURLOPT_COOKIEFILE, "l/q/b/d");
    curl_easy_setopt(tusock, CURLOPT_TIMEOUT, 7);
    curl_easy_setopt(tusock, CURLOPT_FOLLOWLOCATION, true);
    curl_easy_setopt(tusock, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(tusock, CURLOPT_WRITEFUNCTION, copy_to_string);
    curl_easy_setopt(tusock, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(tusock, CURLOPT_POSTFIELDS, temp.c_str());
    curl_easy_setopt(tusock, CURLOPT_USERAGENT, ";)");
    curl_easy_perform(tusock);
    curl_easy_cleanup(tusock);

    wxString ende(buffer.c_str(), wxConvUTF8);
    if ( ende.Contains(wxT("<a href=\"/Logout\">Logout</a>")) )
    {
        return LOGINSUCCESSFULL;
    }
    else if ( !ende.Contains(wxT("Datenschutz")) )
    {
        return NOCONNECTION;
    }

    return WRONGLOGIN;
}

wxString tu_getpage(CURLSH *share, wxString url)
{
    std::string buffer;
    std::string temp = std::string(url.mb_str());

    CURL *tusock = curl_easy_init();
    curl_easy_setopt(tusock, CURLOPT_SHARE, share);
    curl_easy_setopt(tusock, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(tusock, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(tusock, CURLOPT_URL, temp.c_str());
    curl_easy_setopt(tusock, CURLOPT_TIMEOUT, 7);
    curl_easy_setopt(tusock, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(tusock, CURLOPT_FOLLOWLOCATION, true);
    curl_easy_setopt(tusock, CURLOPT_WRITEFUNCTION, copy_to_string);
    curl_easy_setopt(tusock, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(tusock, CURLOPT_USERAGENT, ";)");
    curl_easy_perform(tusock);
    curl_easy_cleanup(tusock);

    wxString fin(buffer.c_str(), wxConvUTF8);
    return fin;
}

wxString tu_sendmsg(CURLSH *share, wxString BuddyName, wxString Message)
{
    wxString NewMsgHtm = tu_getpage(share, wxT("https://m.team-ulm.de/NeueNachricht/"));
    wxString token = FilterString(NewMsgHtm, wxT("type=\"hidden\" id=\"token\" value=\""), wxT("\""));
    std::string returnhtm;
    CURL *tusock = curl_easy_init();

    char *temp2 = curl_easy_escape(tusock, Message.mb_str(wxConvUTF8), 0);
    wxString MsgConverted(temp2, wxConvUTF8);
    free(temp2);

    wxString Content = wxT("recipient=") + BuddyName + wxT("&token=") + token + wxT("&message=") + MsgConverted + wxT("&send=Nachricht+senden");
    struct curl_slist *headerlist = NULL; headerlist = curl_slist_append(headerlist, "Expect:");

    curl_easy_setopt(tusock, CURLOPT_SHARE, share);
    curl_easy_setopt(tusock, CURLOPT_URL, "https://m.team-ulm.de/NachrichtSenden/");
    curl_easy_setopt(tusock, CURLOPT_TIMEOUT, 7);
    curl_easy_setopt(tusock, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(tusock, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(tusock, CURLOPT_FOLLOWLOCATION, true);
    curl_easy_setopt(tusock, CURLOPT_WRITEFUNCTION, copy_to_string);
    curl_easy_setopt(tusock, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(tusock, CURLOPT_WRITEDATA, &returnhtm);
    curl_easy_setopt(tusock, CURLOPT_USERAGENT, ";)");
    curl_easy_setopt(tusock, CURLOPT_POSTFIELDS, (const char*)Content.mb_str(wxConvUTF8));
    curl_easy_setopt(tusock, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_perform(tusock);
    curl_easy_cleanup(tusock);

    wxString fin(returnhtm.c_str(), wxConvUTF8);
    return fin;
}

bool tu_setstatus(CURLSH *share, wxString status)
{
    std::string buffer;
    CURL *tusock = curl_easy_init();

    char *temp2 = curl_easy_escape(tusock, status.mb_str(wxConvUTF8), 0);
    wxString StatConverted(temp2, wxConvUTF8);
    free(temp2);

    wxString stathttp = wxT("newStatus=") + StatConverted + wxT("&save=Status+%C3%A4ndern");

    std::string temp = std::string(stathttp.mb_str());

    curl_easy_setopt(tusock, CURLOPT_SHARE, share);
    curl_easy_setopt(tusock, CURLOPT_URL, "https://m.team-ulm.de/index.php");
    curl_easy_setopt(tusock, CURLOPT_COOKIEFILE, "l/q/b/d");
    curl_easy_setopt(tusock, CURLOPT_TIMEOUT, 7);
    curl_easy_setopt(tusock, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(tusock, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(tusock, CURLOPT_FOLLOWLOCATION, true);
    curl_easy_setopt(tusock, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(tusock, CURLOPT_WRITEFUNCTION, copy_to_string);
    curl_easy_setopt(tusock, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(tusock, CURLOPT_POSTFIELDS, temp.c_str());
    curl_easy_setopt(tusock, CURLOPT_USERAGENT, ";)");
    curl_easy_perform(tusock);
    curl_easy_cleanup(tusock);

    wxString ende(buffer.c_str(), wxConvUTF8);

    if ( ende.Contains(wxT("</html>")) )
        return true;
    else return false;
}

void tu_filedownload(wxString url, wxString Downloadlocation)
{
    FILE *file = fopen(Downloadlocation.c_str(),"wb");
    std::string temp = std::string(url.mb_str());

    CURL *tusock = curl_easy_init();
    curl_easy_setopt(tusock, CURLOPT_URL, temp.c_str());
    curl_easy_setopt(tusock, CURLOPT_TIMEOUT, 7);
    curl_easy_setopt(tusock, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(tusock, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(tusock, CURLOPT_USERAGENT, ";)");
    curl_easy_perform(tusock);
    curl_easy_cleanup(tusock);
    fclose(file);
}

wxString tu_gettestpage()
{
    std::string buffer;

    CURL *tusock = curl_easy_init();
    curl_easy_setopt(tusock, CURLOPT_URL, "https://m.team-ulm.de/");
    curl_easy_setopt(tusock, CURLOPT_TIMEOUT, 3);
    curl_easy_setopt(tusock, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(tusock, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(tusock, CURLOPT_WRITEFUNCTION, copy_to_string);
    curl_easy_setopt(tusock, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(tusock, CURLOPT_WRITEDATA, &buffer);
    curl_easy_perform(tusock);
    curl_easy_cleanup(tusock);

    wxString retn(buffer.c_str(), wxConvUTF8);
    return retn;
}

wxString tu_getversionstring()
{
    std::string buffer;

    CURL *tusock = curl_easy_init();
    // Die Versionsnummer kann sich nämlich irgendwann unterscheiden^^
    #ifdef __WXMSW__
        curl_easy_setopt(tusock, CURLOPT_URL, "http://tumessenger.ouned.de/neuver.php?os=win");
    #elif defined __WXGTK__
        curl_easy_setopt(tusock, CURLOPT_URL, "http://tumessenger.ouned.de/neuver.php?os=linux");
    #elif defined __WXMAC__
        curl_easy_setopt(tusock, CURLOPT_URL, "http://tumessenger.ouned.de/neuver.php?os=mac");
    #endif
    curl_easy_setopt(tusock, CURLOPT_TIMEOUT, 4);
    curl_easy_setopt(tusock, CURLOPT_WRITEFUNCTION, copy_to_string);
    curl_easy_setopt(tusock, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(tusock, CURLOPT_WRITEDATA, &buffer);
    curl_easy_perform(tusock);
    curl_easy_cleanup(tusock);

    wxString retn(buffer.c_str(), wxConvUTF8);
    return retn;
}
