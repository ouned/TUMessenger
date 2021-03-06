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

#define VERSIONSTRING wxT("3.0 Beta 9")

#ifdef __WXMSW__
    #define GetDataPath() wxStandardPaths().GetDataDir() + wxT("/Daten")
#elif defined __WXGTK__ || defined __WXMAC__
    #define GetDataPath() wxStandardPaths().Get().GetDataDir()
#endif

wxString FilterString(wxString original, wxString beginning, wxString end);
wxString FilterStringB(wxString original, wxString beginning, wxString end);
wxString EncryptString(wxString string);
wxString DecryptString(wxString string);

wxString GetBuildInfo();

#define BROWSERBACKEND true
#define WXHTMLBACKEND false

#define HTMLTEMPLATE wxT("<html><head><style type=\"text/css\">body{border: 0px;font-family:Arial;}</style><script type=\"text/javascript\">function getDocHeight(){var D = document;return Math.max(Math.max(D.body.scrollHeight, D.documentElement.scrollHeight),Math.max(D.body.offsetHeight, D.documentElement.offsetHeight),Math.max(D.body.clientHeight, D.documentElement.clientHeight));}</script></head><body onLoad=\"window.scrollBy(0, getDocHeight());\"><div style=\"padding: 0px; margin: 0px; position: absolute; top: 0px; left: 0px; width: 100%\"></div></body></html>")
