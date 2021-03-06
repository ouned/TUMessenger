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
#include <wx/taskbar.h>
#include <wx/htmllbox.h>
#include <wx/popupwin.h>
#include <wx/power.h>
#include <curl/curl.h>
#include <vector>

#ifdef __WXMSW__
    #include "TUWinSeven.h"
#endif

#define MALE 1
#define FEMALE 2
#define NOGENDER 3
#define ONLINE true
#define OFFLINE false
#define NOAGE wxT("-1")
#define NOSTATUS wxT("")

class TrayIconClass; class TUBuddylist; class TUChatwindow;

class TUMsgSender: public wxThread
{
    public:
        TUBuddylist *Buddylist;
        wxArrayString Msg;
        virtual void* Entry();
        int SleepF(int secs);
};

class TrayIconClass: public wxTaskBarIcon
{
    public:
        TrayIconClass(){};
        TUBuddylist *parent;

    private:
        virtual wxMenu *CreatePopupMenu();
        void OnClick(wxTaskBarIconEvent&);
        void Quit(wxCommandEvent&);
        void SoundSwitch(wxCommandEvent &event);
        #ifdef __WXMAC__
            void OpenBlist(wxCommandEvent &event);
        #endif

    DECLARE_EVENT_TABLE()
};

class TUWorkerThread: public wxThread
{
    public:
        TUWorkerThread() : wxThread(wxTHREAD_JOINABLE){};
        TUBuddylist *parent;
        bool GetBuddysInstantly;
        int FromSuspended;
        wxString username;
        wxString password;

        virtual void* Entry();
        void GetMsgs();
        int SleepF(int secs);
};

class TUPicLoader: public wxThread
{
    public:
        TUPicLoader() : wxThread(wxTHREAD_JOINABLE){};
        TUBuddylist *parent;
        wxArrayString ids;
        virtual void* Entry();
};

struct VMessage
{
    wxString username;
    wxString userid;
    wxString msgid;
    wxString message;
    wxString subject;
};

DECLARE_EVENT_TYPE(NEW_MESSAGE_EVT, -4)

#define NEW_MESSAGE_EVT(fn)                                             \
	DECLARE_EVENT_TABLE_ENTRY( NEW_MESSAGE_EVT, wxID_ANY, wxID_ANY,   \
	(wxObjectEventFunction)(wxEventFunction)&fn, (wxObject*) NULL ),

class MessageEvent: public wxEvent
{
    public:
        MessageEvent();
        virtual wxEvent *Clone() const { return new MessageEvent(*this); };
        VMessage VMsg;

	DECLARE_DYNAMIC_CLASS(MessageEvent)
};

struct VBuddy
{
    bool connection;
    wxString username;
    wxString id;
    int gender;
    wxString age;
    wxString status;
};

class BuddyInfoPopup: public wxPopupWindow
{
    public:
        BuddyInfoPopup(TUBuddylist *parent);
        void InitGui();

        wxString username;
        bool picuse;
        wxString pathtopic;
        wxString age;
        int gender;
        wxString status;
};

class AboutDialog: public wxDialog
{
    public:
        AboutDialog(TUBuddylist *par);
};

class TUBuddylist: public wxFrame
{
    public:
        TUBuddylist(const wxString &title, const wxPoint &pos, const wxSize &size, long style);
        void OnStart();

        bool FromStartWindow;
        bool startHidden;
        bool NoPosChange;
        bool ChuckNorrisMode;
        int CurrMode;
        bool backend;

        TUChatwindow *ChatWin;
        bool ChatWindowOpen;
        bool SoundsEnabled;
        wxMenuItem *SoundsItem;
        TrayIconClass *TrayIcon;

        wxPoint Position;
        TUWorkerThread *Worker;
        TUPicLoader *PicWorker;
        std::vector<wxArrayString> MsgQueue;
        CURLSH *tulink;
        wxString username;
        wxTimer *MsgSendTimer;
        wxTimer *PopupTimer;
        wxTimer *PopupKillTimer;
        TUMsgSender *MsgSender;
        int lastMsgSent;

        wxArrayString Smilies;

        #ifdef __WXMSW__
            bool WinSevenTaskbarEnabled;
            ITaskbarList3 *m_taskbarInterface;
        #endif

        void Quit(wxCommandEvent&);

    private:
        wxMenuBar *MenuBar;
        wxMenu *FileMenu;
        wxMenuItem *ChatItem;
        wxMenuItem *BIconItem;
        wxMenuItem *UserChItem;
        wxMenuItem *EndItem;
        wxMenu *ViewMenu;
        wxMenuItem *LargeViewItem;
        wxMenuItem *SmallViewItem;
        wxMenu *SettingsMenu;
        wxMenuItem *SettingsItem;
        wxMenu *HelpMenu;
        wxMenuItem *UpdateItem;
        wxMenuItem *AboutItem;
        wxMenuItem *WebItem;

        wxSimpleHtmlListBox *Buddylist;
        std::vector<VBuddy> VBuddylist;
        wxStatusBar *StatBar;
        wxTextCtrl *StatusBox;
        wxBitmapButton *StatusButton;

        BuddyInfoPopup *Popup;
        wxPoint OldPos;
        unsigned long BuddyPopupNum;

        bool PopupOpen;
        bool PicsUsable;
        bool rWas;

        void SetStatus();
        void PositionChanged(wxMoveEvent &event);
        void OnClose(wxCloseEvent& event);
        void StartNBChat(wxCommandEvent&);
        void ClickLargeView(wxCommandEvent&);
        void ClickSmallView(wxCommandEvent&);
        void OpenAbout(wxCommandEvent &event);
        void UpdateClicked(wxCommandEvent &event);
        void ReloadBPics(wxCommandEvent &event);
        void WebsiteClicked(wxCommandEvent &event);
        void ThreadReport(wxCommandEvent &evt);
        void BuddylistMouseOver(wxHtmlCellEvent &event);
        void ShowPopup(wxTimerEvent &event);
        void KillPopup(wxTimerEvent &event);
        void SendMsg(wxTimerEvent &event);
        void NewChat(wxCommandEvent &event);
        void OnNewMsg(MessageEvent &event);
        void SoundSwitch(wxCommandEvent &event);
        void UserChEvt(wxCommandEvent &event);
        void UsrStatusChanged(wxCommandEvent &event);
        void StatButtonClicked(wxCommandEvent& event) { SetStatus(); };
        void EnableOfflineMode(bool reason);
        void DisableOfflineMode();

        #ifdef __WXMSW__
            WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam);
            void Suspending(wxPowerEvent &event);
            void Resuming(wxPowerEvent &event);
        #endif

    DECLARE_EVENT_TABLE()
};
