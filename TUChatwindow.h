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
#include <wx/aui/auibook.h>
#include <wx/richtext/richtextctrl.h>
#include "wxiepanel.h"
#include "webview.h"

class TUBuddylist;

class TUChatwindow: public wxFrame
{
    public:
        virtual bool Show(bool show);

        TUChatwindow(wxWindow *par, bool FromNewMsg);
        TUBuddylist *parent;

        wxString UserName;
        wxAuiNotebook *Chats;

    private:
        void OnClose(wxCloseEvent& event);
        void OnCloseChat(wxAuiNotebookEvent &event);
        void OnMiddleClick(wxAuiNotebookEvent &event);
        void OnChangedTab(wxAuiNotebookEvent &event);
        void SetFocusEvent(wxCommandEvent &event);
        void OnActivate(wxActivateEvent &event);

        void EndChatWindow();

        bool frNewMsg;

    DECLARE_EVENT_TABLE()
};

class BuddyChatWin: public wxWindow
{
    public:
        BuddyChatWin(wxWindow *par);
        void InitGui(wxString username, wxString UserPicPath, wxString status, bool connection);
        TUChatwindow *parent;
        wxString BuddyID;
        wxString InnerHTM;

        wxGenericStaticBitmap *UserPic;
        wxGenericStaticBitmap *StatusImage;
        wxStaticText *Userstatus;
        bool ChatTextEmpty;
        bool NewMessage;
        #ifdef __WXMSW__
            wxWebView *ChatTextNew;
            wxHtmlWindow *ChatText;
        #else
            wxHtmlWindow *ChatText;
        #endif
        wxTextCtrl *EnterText;

        void AddMsg(wxString str);
        void MarkMessage(wxString oldstr, wxString newstr);
    private:
        void OnEnterInChatText(wxKeyEvent& event);
        void SendMessage(wxCommandEvent &event);
        void OnKeyDown(wxKeyEvent &event);
        void OnPressKeyChatText(wxKeyEvent &event);
        void AntiCrashKeyPress(wxCommandEvent &event);
        void OnLinkClicked(wxHtmlLinkEvent &event);

        int MsgID;

    DECLARE_EVENT_TABLE()
};
