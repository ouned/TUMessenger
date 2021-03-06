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
#include <curl/curl.h>

#define LOGINSUCCESSFULL -1
#define WRONGLOGIN -2
#define NOCONNECTION -3
#define BUDDY_HTML -4
#define PICSFINISHED -5
#define VERSION_CHECK -6
#define MSGSEND_REPORT -7
#define TREPORT_CLOST -8
#define TREPORT_CBACK -9
#define STATUS_CHANGED -10

#define CONNLOST true
#define SUSPENDED false

#define NORMAL 10
#define OFFMODE 11

int tu_login(CURLSH *share, wxString username, wxString password);
wxString tu_getpage(CURLSH *share, wxString url);
bool tu_setstatus(CURLSH *share, wxString status);
void tu_filedownload(wxString url, wxString Downloadlocation);
wxString tu_sendmsg(CURLSH *share, wxString BuddyName, wxString Message);
wxString tu_getversionstring();
wxString tu_gettestpage();
