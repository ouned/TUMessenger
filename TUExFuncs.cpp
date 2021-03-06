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

wxString FilterString(wxString original, wxString beginning, wxString end)
{
    wxString zwi = original.Remove(0, original.Find(beginning) + (int)beginning.Length());
    return zwi.Remove(zwi.Find(end));
}

wxString FilterStringB(wxString original, wxString beginning, wxString end)
{
    wxString zwi = original.Remove(0, original.rfind(beginning) + (int)beginning.Length());
    return zwi.Remove(zwi.Find(end));
}

// Viel zu einfach.. sollte aber die meisten naps davon abhalten ;)
wxString EncryptString(wxString string)
{
    int i = 0;
    wxString temp = string;

    while ( i < (int)string.Length() ) // ZuMachen: for-schleife
    {
        if ( i == 0 )
        {
            temp[i] = (int)string[i] - 1;
        }
        else if ( i + 1 == (int)string.Length() )
        {
            temp[i] = (int)string[i] - 1;
        }
        else
        {
            temp[i] = (int)string[i] + 1;
        }

        i++;
    }

    return temp;
}

wxString DecryptString(wxString string)
{
    int i = 0;
    wxString temp = string;

    while ( i < (int)string.Length() )
    {
        if ( i == 0 )
        {
            temp[i] = (int)string[i] + 1;
        }
        else if ( i + 1 == (int)string.Length() )
        {
            temp[i] = (int)string[i] + 1;
        }
        else
        {
            temp[i] = (int)string[i] - 1;
        }

        i++;
    }

    return temp;
}

wxString GetBuildInfo()
{
    wxVersionInfo wxbuild = wxGetLibraryVersionInfo();
    wxString buildstring;

    buildstring += wxString::Format(wxT("wxWidgets-%i.%i.%i-"), wxbuild.GetMajor(), wxbuild.GetMinor(), wxbuild.GetMicro());
    #ifdef __WXMSW__
        #ifdef WIN32
            buildstring += wxT("wxMSW-win32");
        #else
            buildstring += wxT("wxMSW-win64");
        #endif
    #elif defined __WXGTK__
        #ifdef __i386__
            buildstring += wxT("wxGTK-i386");
        #elif defined __amd64__
            buildstring += wxT("wxGTK-amd64");
        #else
            buildstring += wxT("wxGTK-unknown");
        #endif
    #elif defined __WXMAC__
        #ifdef __i386__
            buildstring += wxT("wxMAC-i386");
        #elif defined __amd64__
            buildstring += wxT("wxMAC-amd64");
        #else
            buildstring += wxT("wxMAC-unknown");
        #endif
    #endif

    return buildstring;
}
