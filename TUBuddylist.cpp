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
#include <wx/generic/statbmpg.h>
#include <wx/stdpaths.h>
#include <wx/fileconf.h>
#include <wx/sound.h>
#include <wx/artprov.h>
#include <wx/image.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/htmllbox.h>
#include <wx/display.h>
#include <wx/hyperlink.h>
#include "TUConnection.h"
#include "TUBuddylist.h"
#include "TUExFuncs.h"
#include "TUChatwindow.h"
#include "TUStartFrame.h"

#ifdef __WXMAC__
    const long TRAYMENU_OPENBLIST = wxNewId();
#endif
const long TRAYMENU_SOUNDS = wxNewId();

const long FILEMENU_STARTCHAT = wxNewId();
const long FILEMENU_BICON = wxNewId();
const long FILEMENU_USERCHITEM = wxNewId();
const long VIEWMENU_LARGEVIEW = wxNewId();
const long VIEWMENU_SMALLVIEW = wxNewId();
const long SETTINGSMENU_SOUNDS = wxNewId();
const long HELPMENU_UPDATE = wxNewId();
const long HELPMENU_WEB = wxNewId();
const long BUDDYLIST_CONTROL = wxNewId();
const long STATUS_TEXT = wxNewId();
const long STATUS_BUTTON = wxNewId();
const long POPUPTIMER = wxNewId();
const long POPUPKILLTIMER = wxNewId();
const long MSGTIMER = wxNewId();

DEFINE_EVENT_TYPE( NEW_MESSAGE_EVT );
IMPLEMENT_DYNAMIC_CLASS( MessageEvent, wxEvent )
DECLARE_EVENT_TYPE(EVENT_THREADREPORT, -2)
DEFINE_EVENT_TYPE(EVENT_THREADREPORT)

wxCriticalSection *CurlLocker = new wxCriticalSection();

BEGIN_EVENT_TABLE(TrayIconClass, wxTaskBarIcon)
    #ifdef __WXMAC__
        EVT_MENU(TRAYMENU_OPENBLIST, TrayIconClass::OpenBlist)
    #elif defined __WXGTK__
        EVT_TASKBAR_LEFT_DOWN(TrayIconClass::OnClick)
    #elif defined __WXMSW__
        EVT_TASKBAR_LEFT_DCLICK(TrayIconClass::OnClick)
    #endif
    EVT_MENU(wxID_EXIT, TrayIconClass::Quit)
    EVT_MENU(TRAYMENU_SOUNDS, TrayIconClass::SoundSwitch)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(TUBuddylist, wxFrame)
    EVT_MENU(wxID_EXIT, TUBuddylist::Quit)
    EVT_MENU(wxID_ABOUT, TUBuddylist::OpenAbout)
    EVT_MENU(FILEMENU_STARTCHAT, TUBuddylist::StartNBChat)
    EVT_MENU(FILEMENU_BICON, TUBuddylist::ReloadBPics)
    EVT_MENU(VIEWMENU_LARGEVIEW, TUBuddylist::ClickLargeView)
    EVT_MENU(VIEWMENU_SMALLVIEW, TUBuddylist::ClickSmallView)
    EVT_MENU(SETTINGSMENU_SOUNDS, TUBuddylist::SoundSwitch)
    EVT_MENU(HELPMENU_UPDATE, TUBuddylist::UpdateClicked)
    EVT_MENU(HELPMENU_WEB, TUBuddylist::WebsiteClicked)
    EVT_MENU(FILEMENU_USERCHITEM, TUBuddylist::UserChEvt)
    EVT_COMMAND(wxID_ANY, EVENT_THREADREPORT, TUBuddylist::ThreadReport)
    EVT_HTML_CELL_HOVER(BUDDYLIST_CONTROL, TUBuddylist::BuddylistMouseOver)
    EVT_TIMER(POPUPTIMER, TUBuddylist::ShowPopup)
    EVT_TIMER(POPUPKILLTIMER, TUBuddylist::KillPopup)
    EVT_TIMER(MSGTIMER, TUBuddylist::SendMsg)
    EVT_LISTBOX_DCLICK(BUDDYLIST_CONTROL, TUBuddylist::NewChat)
    NEW_MESSAGE_EVT(TUBuddylist::OnNewMsg)
    EVT_CLOSE(TUBuddylist::OnClose)
    EVT_TEXT(STATUS_TEXT, TUBuddylist::UsrStatusChanged)
    EVT_BUTTON(STATUS_BUTTON, TUBuddylist::StatButtonClicked)
    EVT_TEXT_ENTER(STATUS_TEXT, TUBuddylist::StatButtonClicked)
    #ifdef __WXMSW__
        EVT_POWER_SUSPENDED(TUBuddylist::Suspending)
        EVT_POWER_RESUME(TUBuddylist::Resuming)
    #endif
END_EVENT_TABLE()

TUBuddylist::TUBuddylist(const wxString &title, const wxPoint &pos, const wxSize &size, long style) : wxFrame(NULL, wxID_ANY, title, pos, size, style)
{
    TrayIcon = new TrayIconClass();
    TrayIcon->parent = this;
    wxFlexGridSizer *MainSizer = new wxFlexGridSizer(2, 1, 0, 0);
    #ifdef __WXGTK__
        MainSizer->SetMinSize(wxSize(150, 250));
        this->SetIcon(wxIcon(GetDataPath() + wxT("/TUKlein.png"), wxBITMAP_TYPE_PNG));
    #elif defined __WXMSW__
        this->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_MENUBAR));
        MainSizer->SetMinSize(wxSize(150, 300));
        this->SetIcons(wxIconBundle(GetDataPath() + wxT("/TUTask.ico"), wxBITMAP_TYPE_ICO));
    #elif defined __WXMAC__
        MainSizer->SetMinSize(wxSize(150, 250));
        this->SetIcon(wxIcon(GetDataPath() + wxT("/TUKlein.png"), wxBITMAP_TYPE_PNG));
    #endif

    MenuBar = new wxMenuBar();
    FileMenu = new wxMenu();
    ViewMenu = new wxMenu();
    HelpMenu = new wxMenu();

    #ifdef __WXMAC__
        FileMenu->Append(wxID_EXIT); FileMenu->Append(wxID_ABOUT);
    #endif

    MenuBar->Append(FileMenu, wxT("Buddys"));

    ChatItem = new wxMenuItem(FileMenu, FILEMENU_STARTCHAT, wxT("Chat starten..."), wxT("Chat mit Person außerhalb der Buddyliste starten"), wxITEM_NORMAL);
    ChatItem->SetBitmap(wxBitmap(GetDataPath() + wxT("/chatstarten.png"), wxBITMAP_TYPE_PNG));
    FileMenu->Append(ChatItem);

    BIconItem = new wxMenuItem(FileMenu, FILEMENU_BICON, wxT("Profilbilder aktualisieren"), wxT("Die Buddyicons neu herunterladen"), wxITEM_NORMAL);
    BIconItem->SetBitmap(wxBitmap(GetDataPath() + wxT("/profbladen.png"), wxBITMAP_TYPE_PNG));
    FileMenu->Append(BIconItem);
    BIconItem->Enable(false);

    FileMenu->AppendSeparator();
    UserChItem = new wxMenuItem(FileMenu, FILEMENU_USERCHITEM, wxT("Benutzer wechseln..."), wxT("Ausloggen und mit anderem Benutzernamen einloggen"), wxITEM_NORMAL);
    UserChItem->SetBitmap(wxBitmap(GetDataPath() + wxT("/bewechs.png"), wxBITMAP_TYPE_PNG));
    FileMenu->Append(UserChItem);
    #ifndef __WXMAC__
        EndItem = new wxMenuItem(FileMenu, wxID_EXIT, wxT("Beenden"), wxT("Den TU Messenger beenden"), wxITEM_NORMAL);
        #ifdef __WXMSW__
            EndItem->SetBitmap(wxBitmap(GetDataPath() + wxT("/beenden.png"), wxBITMAP_TYPE_PNG));
        #endif
        FileMenu->Append(EndItem);
    #endif
    MenuBar->Append(ViewMenu, wxT("Ansicht"));
    LargeViewItem = new wxMenuItem(ViewMenu, VIEWMENU_LARGEVIEW, wxT("Große Ansicht"), wxT("Die Ansicht für kleinere Buddylisten"), wxITEM_RADIO);
    ViewMenu->Append(LargeViewItem);
    SmallViewItem = new wxMenuItem(ViewMenu, VIEWMENU_SMALLVIEW, wxT("Kleine Ansicht"), wxT("Die Ansicht für größere Buddylisten"), wxITEM_RADIO);
    ViewMenu->Append(SmallViewItem);
    LargeViewItem->Check();
    SettingsMenu = new wxMenu();
    MenuBar->Append(SettingsMenu, wxT("Einstellungen"));
    SoundsItem = new wxMenuItem(SettingsMenu, SETTINGSMENU_SOUNDS, wxT("Klänge"), wxT("Klänge anschalten / abschalten"), wxITEM_CHECK);
    SettingsMenu->Append(SoundsItem);
    SettingsMenu->AppendSeparator();
    SettingsItem = new wxMenuItem(SettingsMenu, wxID_PREFERENCES, wxT("Einstellungen"), wxT("Einstellungen aufrufen"), wxITEM_NORMAL);
    #if defined __WXMSW__ || defined __WXMAC__
        SettingsItem->SetBitmap(wxBitmap(GetDataPath() + wxT("/einstellungen.png"), wxBITMAP_TYPE_PNG));
    #endif
    SettingsMenu->Append(SettingsItem);
    SettingsItem->Enable(false);
    MenuBar->Append(HelpMenu, wxT("Hilfe"));
    UpdateItem = new wxMenuItem(HelpMenu, HELPMENU_UPDATE, wxT("Updates suchen"), wxT("Nach Aktualisierungen suchen"), wxITEM_NORMAL);
    UpdateItem->SetBitmap(wxBitmap(GetDataPath() + wxT("/aktualisieren.png"), wxBITMAP_TYPE_PNG));
    HelpMenu->Append(UpdateItem);
    HelpMenu->AppendSeparator();
    WebItem = new wxMenuItem(HelpMenu, HELPMENU_WEB, wxT("Webseite"), wxT("Zur TUMessenger Webseite"), wxITEM_NORMAL);
    WebItem->SetBitmap(wxBitmap(GetDataPath() + wxT("/webseite.png"), wxBITMAP_TYPE_PNG));
    HelpMenu->Append(WebItem);
    #ifndef __WXMAC__
        AboutItem = new wxMenuItem(HelpMenu, wxID_ABOUT, wxT("Über"), wxT("TUMessenger Infos"), wxITEM_NORMAL);
        #if defined __WXMSW__
            AboutItem->SetBitmap(wxBitmap(GetDataPath() + wxT("/ueber.png"), wxBITMAP_TYPE_PNG));
        #endif
        HelpMenu->Append(AboutItem);
    #endif
    SetMenuBar(MenuBar);

    StatBar = new wxStatusBar(this, wxID_ANY);
    #ifdef __WXMSW__
        int Widths[2] = {-3, -1};
    #elif defined __WXGTK__ || defined __WXMAC__
        int Widths[2] = {-2, -1};
    #endif
    StatBar->SetFieldsCount(2, Widths);
    StatBar->SetStatusText(wxT("Login.."), 1);
    SetStatusBar(StatBar);

    Buddylist = new wxSimpleHtmlListBox(this, BUDDYLIST_CONTROL, wxDefaultPosition, wxDefaultSize, 0, NULL, wxBORDER_NONE);
    MainSizer->Add(Buddylist, 1, wxEXPAND|wxALIGN_LEFT|wxALIGN_TOP, 1);
    wxFlexGridSizer *StatusSizer = new wxFlexGridSizer(0, 2, 0, 0);
    StatusBox = new wxTextCtrl(this, STATUS_TEXT, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    StatusSizer->Add(StatusBox, 1, wxEXPAND|wxALIGN_LEFT|wxALIGN_BOTTOM, 1);
    #ifdef __WXMSW__
        StatusButton = new wxBitmapButton(this, STATUS_BUTTON, wxBitmap(GetDataPath() + wxT("/status.png"), wxBITMAP_TYPE_PNG), wxDefaultPosition, wxSize(StatusBox->GetSize().GetHeight(), StatusBox->GetSize().GetHeight()));
    #elif defined __WXGTK__ || defined __WXMAC__
        StatusButton = new wxBitmapButton(this, STATUS_BUTTON, wxArtProvider::GetBitmap(wxART_TICK_MARK, wxART_MENU), wxDefaultPosition, wxSize(StatusBox->GetSize().GetHeight(), StatusBox->GetSize().GetHeight()));
    #endif
    StatusSizer->Add(StatusButton, 1, wxSHAPED|wxALIGN_RIGHT|wxALIGN_BOTTOM, 5);
    MainSizer->Add(StatusSizer, 1, wxTOP|wxBOTTOM|wxEXPAND|wxALIGN_LEFT|wxALIGN_BOTTOM, 3);
    SetSizer(MainSizer);
    MainSizer->SetSizeHints(this);

    MainSizer->AddGrowableRow(0, wxBOTH);
    MainSizer->AddGrowableCol(0, wxBOTH);
    StatusSizer->AddGrowableCol(0, wxHORIZONTAL);
    StatusButton->Enable(false);

    PopupTimer = new wxTimer();
    PopupTimer->SetOwner(this, POPUPTIMER);
    PopupKillTimer = new wxTimer();
    PopupKillTimer->SetOwner(this, POPUPKILLTIMER);
    MsgSendTimer = new wxTimer();
    MsgSendTimer->SetOwner(this, MSGTIMER);
    MsgSender = NULL;
    PicWorker = NULL;

    #ifdef __WXMSW__
        if ( wxPlatformInfo::Get().CheckOSVersion(6, 1) )
        {
            HRESULT hr = CoCreateInstance(CLSID_TaskbarList, 0, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, reinterpret_cast<void**>(&m_taskbarInterface));
            if (SUCCEEDED(hr))
            {
                hr = m_taskbarInterface->HrInit();
                if (FAILED(hr))
                {
                    m_taskbarInterface->Release();
                    m_taskbarInterface = NULL;
                }
            }
        }
    #endif

    Connect(GetId(), wxEVT_MOVE, (wxObjectEventFunction)&TUBuddylist::PositionChanged);
}

wxMenu *TrayIconClass::CreatePopupMenu()
{
    wxMenu *Menu = new wxMenu();
    #ifdef __WXMAC__
        wxMenuItem *OpenBuddylistItem = new wxMenuItem(Menu, TRAYMENU_OPENBLIST, wxT("Buddyliste anzeigen"), wxT("Die Buddyliste anzeigen"), wxITEM_NORMAL);
        Menu->Append(OpenBuddylistItem);
    #endif
    wxMenuItem *SoundsItem = new wxMenuItem(Menu, TRAYMENU_SOUNDS, wxT("Klänge"), wxT("Klänge anschalten / abschalten"), wxITEM_CHECK);
    Menu->Append(SoundsItem);
    SoundsItem->Check(parent->SoundsItem->IsChecked());
    #ifndef __WXMAC__
        wxMenuItem *EndItem = new wxMenuItem(Menu, wxID_EXIT, wxT("Beenden"), wxT("Den TU Messenger beenden"), wxITEM_NORMAL);
        #ifdef __WXMSW__
            EndItem->SetBitmap(wxBitmap(GetDataPath() + wxT("/beenden.png"), wxBITMAP_TYPE_PNG));
        #endif
        Menu->Append(EndItem);
    #endif

    return Menu;
}

#ifdef __WXMAC__
void TrayIconClass::OpenBlist(wxCommandEvent &event)
{
    if ( parent->IsShown() == true )
    {
        parent->Position = parent->GetPosition();
        parent->NoPosChange = true;
        parent->Hide();
    }
    else
    {
        parent->Show();
        parent->Raise();
    }
}
#endif

void TUBuddylist::OnClose(wxCloseEvent& event)
{
    #ifdef __WXMSW__
        if ( wxPlatformInfo::Get().CheckOSVersion(6, 1) && IsProgramPinned() == true )
        {
            this->Iconize(true);
        }
        else
        {
            Position = this->GetPosition();
            NoPosChange = true;
            this->Hide();
        }
    #else
        Position = this->GetPosition();
        NoPosChange = true;
        this->Hide();
    #endif
}

void TrayIconClass::OnClick(wxTaskBarIconEvent&)
{
    if ( parent->IsIconized() )
    {
        parent->Iconize(false);
    }
    else
    {
         if ( parent->IsShown() == true )
        {
            #ifdef __WXMSW__
                if ( wxPlatformInfo::Get().CheckOSVersion(6, 1) && IsProgramPinned() == true )
                {
                    parent->Iconize(true);
                }
                else
                {
                    parent->Position = parent->GetPosition();
                    parent->NoPosChange = true;
                    parent->Hide();
                }
            #else
                parent->Position = parent->GetPosition();
                parent->NoPosChange = true;
                parent->Hide();
            #endif
        }
        else
        {
            parent->Show();
            parent->Raise();

            #ifdef __WXMSW__
                if ( parent->WinSevenTaskbarEnabled == true && wxPlatformInfo::Get().CheckOSVersion(6, 1) )
                {
                    HICON hIcon;
                    if ( parent->CurrMode == NORMAL )
                        hIcon = (HICON)LoadImage(NULL, (GetDataPath() + wxT("/on_overlay.ico")).c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
                    else
                        hIcon = (HICON)LoadImage(NULL, (GetDataPath() + wxT("/off_overlay.ico")).c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);

                    parent->m_taskbarInterface->SetOverlayIcon(parent->GetHandle(), hIcon, L"Online");
                }
            #endif
        }
    }
}

void TUBuddylist::Quit(wxCommandEvent&)
{
    if ( MsgQueue.size() != 0 )
    {
        int retn = wxMessageBox(wxT("Der TUMessenger hat noch nicht alle ausstehenden Nachrichten versendet.\nWenn du den TUMessenger jetzt beendest gehen noch nicht versendete Nachrichten verloren.\n\nMöchtest du den TUMessenger trotzdem beenden?"), wxT("Noch nicht alle Nachrichten verschickt"), wxYES_NO | wxICON_WARNING | wxCENTRE, this);
        if ( retn == wxNO )
            return;
    }

    if ( Position.x < 10000 && Position.x > -10000 && !this->IsIconized() )
    {
        wxFileConfig *file = new wxFileConfig(wxT(""), wxT(""), wxStandardPaths().Get().GetUserDataDir() + wxT("/einstellungen.conf"), wxT(""), wxCONFIG_USE_LOCAL_FILE);
        file->Write(wxT("BuddylistePosX"), Position.x);
        file->Write(wxT("BuddylistePosY"), Position.y);
        file->Write(wxT("BuddylisteWidth"), this->GetSize().GetWidth());
        file->Write(wxT("BuddylisteHeight"), this->GetSize().GetHeight());
        if ( ChatWindowOpen == true && !ChatWin->IsIconized() )
        {
            file->Write(wxT("ChatfensterPosX"), ChatWin->GetPosition().x);
            file->Write(wxT("ChatfensterPosY"), ChatWin->GetPosition().y);
            file->Write(wxT("ChatfensterWidth"), ChatWin->GetSize().GetWidth());
            file->Write(wxT("ChatfensterHeight"), ChatWin->GetSize().GetHeight());
        }
        delete file;
    }

    MsgSendTimer->Stop(); PopupKillTimer->Stop(); PopupTimer->Stop();

    if ( Worker != NULL && Worker->IsRunning() )
        Worker->Delete();

    if ( PicWorker != NULL && PicWorker->IsRunning() )
        PicWorker->Delete();

    if ( MsgSender != NULL && MsgSender->IsRunning() )
        MsgSender->Delete();

    tu_getpage(tulink, wxT("https://m.team-ulm.de/Logout"));
    curl_share_cleanup(tulink);
    if ( ChatWindowOpen == true ) { ChatWin->Destroy(); }
    TrayIcon->Destroy(); this->Destroy();
}

void TrayIconClass::Quit(wxCommandEvent&)
{
    if ( parent->MsgQueue.size() != 0 )
    {
        int retn = wxMessageBox(wxT("Der TUMessenger hat noch nicht alle ausstehenden Nachrichten versendet.\nWenn du den TUMessenger jetzt beendest gehen noch nicht versendete Nachrichten verloren.\n\nMöchtest du den TUMessenger trotzdem beenden?"), wxT("Noch nicht alle Nachrichten verschickt"), wxYES_NO | wxICON_WARNING | wxCENTRE, parent);
        if ( retn == wxNO )
            return;
    }

    if ( parent->Position.x < 10000 && parent->Position.x > -10000 && !parent->IsIconized() )
    {
        wxFileConfig *file = new wxFileConfig(wxT(""), wxT(""), wxStandardPaths().Get().GetUserDataDir() + wxT("/einstellungen.conf"), wxT(""), wxCONFIG_USE_LOCAL_FILE);
        file->Write(wxT("BuddylistePosX"), parent->Position.x);
        file->Write(wxT("BuddylistePosY"), parent->Position.y);
        file->Write(wxT("BuddylisteWidth"), parent->GetSize().GetWidth());
        file->Write(wxT("BuddylisteHeight"), parent->GetSize().GetHeight());
        if ( parent->ChatWindowOpen == true && !parent->ChatWin->IsIconized() )
        {
            file->Write(wxT("ChatfensterPosX"), parent->ChatWin->GetPosition().x);
            file->Write(wxT("ChatfensterPosY"), parent->ChatWin->GetPosition().y);
            file->Write(wxT("ChatfensterWidth"), parent->ChatWin->GetSize().GetWidth());
            file->Write(wxT("ChatfensterHeight"), parent->ChatWin->GetSize().GetHeight());
        }
        delete file;
    }

    parent->MsgSendTimer->Stop(); parent->PopupKillTimer->Stop(); parent->PopupTimer->Stop();

    if ( parent->Worker != NULL && parent->Worker->IsRunning() )
        parent->Worker->Delete();

    if ( parent->PicWorker != NULL && parent->PicWorker->IsRunning() )
        parent->PicWorker->Delete();

    if ( parent->MsgSender != NULL && parent->MsgSender->IsRunning() )
        parent->MsgSender->Delete();

    tu_getpage(parent->tulink, wxT("https://m.team-ulm.de/Logout"));
    curl_share_cleanup(parent->tulink);
    if ( parent->ChatWindowOpen == true ) { parent->ChatWin->Destroy(); }
    parent->Destroy(); this->Destroy();
}

void TUBuddylist::PositionChanged(wxMoveEvent &event)
{
    if ( NoPosChange == true )
    {
        this->SetPosition(Position);
        NoPosChange = false;
    }
    else
    {
        Position = this->GetPosition();
    }
}

void lock_function(CURL *handle, curl_lock_data data, curl_lock_access access, void *userptr)
{
    CurlLocker->Enter();
}

void unlock_function(CURL *handle, curl_lock_data data, void *userptr)
{
    CurlLocker->Leave();
}

void *TUWorkerThread::Entry()
{
    int fails = 0;

    if ( GetBuddysInstantly == false && FromSuspended != 3 )
    {
        int result = tu_login(parent->tulink, username, password);

        wxCommandEvent *event = new wxCommandEvent(EVENT_THREADREPORT, wxID_ANY);
        event->SetInt(result);
        wxQueueEvent(parent, event);
    }
    if ( TestDestroy() ) return 0;

    wxString CurrVer = tu_getversionstring();
    if ( CurrVer.Contains(wxT("3")) )
    {
        wxCommandEvent *evt = new wxCommandEvent(EVENT_THREADREPORT, wxID_ANY);
        evt->SetInt(VERSION_CHECK);
        evt->SetString(CurrVer);
        wxQueueEvent(parent, evt);
    }
    if ( TestDestroy() ) return 0;

    wxString BuddyHtmlOld;
    wxString oldStatus = wxT("\n");
    while (1)
    {
        wxString BuddyHtml = tu_getpage(parent->tulink, wxT("https://m.team-ulm.de/index.php?offBuddies=1"));
        if ( BuddyHtml.Contains(wxT("</html>")) == false || BuddyHtml.Contains(wxT(">Username:<br")) || !BuddyHtml.Contains(wxT("Buddyliste")) ) // Irgendwas spinnt
        {
            if ( fails == 2 )
            {
                wxCommandEvent *evt = new wxCommandEvent(EVENT_THREADREPORT, wxID_ANY);
                evt->SetInt(TREPORT_CLOST);
                wxQueueEvent(parent, evt);

                while (1)
                {
                    wxString tphtm = tu_gettestpage();

                    if ( tphtm.Contains(wxT("</html>")) )
                    {
                        tu_login(parent->tulink, username, password);
                        BuddyHtmlOld = wxEmptyString;
                        wxCommandEvent *evtt = new wxCommandEvent(EVENT_THREADREPORT, wxID_ANY);
                        evtt->SetInt(TREPORT_CBACK);
                        wxQueueEvent(parent, evtt);

                        wxString status;
                        if ( !BuddyHtml.Contains(wxT("(<a href=\"/Status/Kommentare/\">")) )
                            status = wxEmptyString;
                        else
                            status = FilterString(BuddyHtml, wxT("</strong> "), wxT("\n   "));

                        wxCommandEvent *statevt = new wxCommandEvent(EVENT_THREADREPORT, wxID_ANY);
                        statevt->SetInt(STATUS_CHANGED);
                        statevt->SetString(status);
                        wxQueueEvent(parent, statevt);
                        oldStatus = status;
                        break;
                    }
                    if ( SleepF(5) == 1 ) return 0;
                }
                continue;
            }
            if ( SleepF(1) == 1 ) return 0;

            fails++;
            continue;
        }
        fails = 0;

        if ( BuddyHtml == BuddyHtmlOld )
        {
            if ( SleepF(5) == 1 ) return 0;
            GetMsgs();
            if ( SleepF(5) == 1 ) return 0;
            continue;
        }
        BuddyHtmlOld = BuddyHtml;

        wxCommandEvent *event = new wxCommandEvent(EVENT_THREADREPORT, wxID_ANY);
        event->SetInt(BUDDY_HTML);
        event->SetString(BuddyHtml);
        wxQueueEvent(parent, event);

        wxString status;
        if ( !BuddyHtml.Contains(wxT("(<a href=\"/Status/Kommentare/\">")) )
            status = wxEmptyString;
        else
            status = FilterString(BuddyHtml, wxT("</strong> "), wxT("\n   "));

        if ( oldStatus != status )
        {
            wxCommandEvent *statevt = new wxCommandEvent(EVENT_THREADREPORT, wxID_ANY);
            statevt->SetInt(STATUS_CHANGED);
            statevt->SetString(status);
            wxQueueEvent(parent, statevt);

            oldStatus = status;
        }

        if ( SleepF(5) == 1 ) return 0;
        GetMsgs();
        if ( SleepF(5) == 1 ) return 0;
    }
    return 0;
}

int TUWorkerThread::SleepF(int secs)
{
    for ( int tts = secs * 1000 / 50; tts > 0; tts-- )
    {
        if ( TestDestroy() ) return 1;
        wxMilliSleep(50);
    }

    return 0;
}

void TUWorkerThread::GetMsgs()
{
    wxString MsgHtml = tu_getpage(parent->tulink, wxT("https://m.team-ulm.de/Nachrichten"));

    if ( !MsgHtml.Contains(wxT("<p style=\"margin-left:5px;\">keine neuen Nachrichten</p>")) && MsgHtml.Contains(wxT(">Datenschutz</a></small>")) )
    {
        int MsgCount = wxAtoi(FilterString(MsgHtml, wxT("  ("), wxT(")")));

        for ( int MsgNum = 0; MsgNum < MsgCount; MsgNum++ )
        {
            wxString ThisMsg = FilterStringB(MsgHtml, wxT("<div class=\"messageHeader\">"), wxT("\">als gelesen markieren</a>   </div>"));

            MessageEvent *evt = new MessageEvent();
            evt->VMsg.userid = FilterString(ThisMsg, wxT("<a href=\"/Profil/"), wxT("\">"));
            evt->VMsg.username = FilterString(ThisMsg, evt->VMsg.userid + wxT("\">"), wxT("</a>"));
            evt->VMsg.message = FilterString(ThisMsg, wxT("<div class=\"messageBody\">\n   "), wxT("  </div>"));
            evt->VMsg.msgid = FilterString(ThisMsg, wxT("<a href=\"/Gelesen/"), wxT("\">als gelesen markieren"));
            if ( ThisMsg.Contains(wxT("<div class=\"messageSubject\">")) ) evt->VMsg.subject = FilterString(ThisMsg, wxT("<div class=\"messageSubject\">\n    <span><strong>"), wxT("</strong></span>"));

            MsgHtml = MsgHtml.Remove(MsgHtml.rfind(wxT("<div class=\"messageHeader\">")));

            if ( evt->VMsg.message.Contains(wxT("PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN")) ) continue;

            wxString TempNewHt = wxEmptyString;
            while ( (TempNewHt.Contains(wxT("<a href=\"/Antworten/") + evt->VMsg.msgid + wxT("\">beantworten</a>")) == true) || (TempNewHt.Contains(wxT("Datenschutz")) == false) ) { TempNewHt = tu_getpage(parent->tulink, wxT("https://m.team-ulm.de/Gelesen/") + evt->VMsg.msgid); }
            wxQueueEvent(parent, evt);
        }
    }
}

void *TUPicLoader::Entry()
{
    if ( !wxDir::Exists(wxStandardPaths().Get().GetUserDataDir() + wxT("/Profilbilder")) )
    {
        wxFileName::Mkdir(wxStandardPaths().Get().GetUserDataDir() + wxT("/Profilbilder"));
    }

    for ( int i = 0; i < (int)ids.GetCount(); i++ )
    {
        if ( TestDestroy() ) return 0;
        tu_filedownload(wxT("http://www.team-ulm.de/fotos/profil/medium/") + ids[i] + wxT(".jpg"), wxStandardPaths().Get().GetUserDataDir() + wxT("/Profilbilder/") + ids[i] + wxT(".jpg"));
        if ( TestDestroy() ) return 0;
    }

    wxCommandEvent *event = new wxCommandEvent(EVENT_THREADREPORT, wxID_ANY);
    event->SetInt(PICSFINISHED);
    if ( parent ) wxQueueEvent(parent, event);

    return 0;
}

void TUBuddylist::OnStart()
{
    ChatWindowOpen = false; PopupOpen = false; ChuckNorrisMode = false; lastMsgSent = 0;

    wxFileConfig *file = new wxFileConfig(wxT(""), wxT(""), wxStandardPaths().Get().GetUserDataDir() + wxT("/einstellungen.conf"), wxT(""), wxCONFIG_USE_LOCAL_FILE);

    NoPosChange = false;
    if ( file->Read(wxT("BuddylistePosX")) != wxEmptyString )
    {
        SetPosition(wxPoint(wxAtoi(file->Read(wxT("BuddylistePosX"))), wxAtoi(file->Read(wxT("BuddylistePosY")))));
        SetSize(wxSize(wxAtoi(file->Read(wxT("BuddylisteWidth"))), wxAtoi(file->Read(wxT("BuddylisteHeight")))));
    }
    else
    {
        SetSize(wxSize(255, 560));
        Center();
    }
    if ( !startHidden )
        Show(true);

    if ( file->Read(wxT("Ansicht")) == wxT("2") )
    {
        SmallViewItem->Check();
        Buddylist->SetMargins(-3, -3);
    }
    else
    {
        Buddylist->SetMargins(-2, -3);
    }

    if ( file->Read(wxT("Klaenge")) == wxT("Aus") )
    {
        SoundsEnabled = false;
        SoundsItem->Check(false);
    }
    else
    {
        SoundsEnabled = true;
        SoundsItem->Check(true);
    }
    if ( file->Read(wxT("IEBackend")) == wxT("An") )
    {
        backend = BROWSERBACKEND;
    }
    else
    {
        backend = WXHTMLBACKEND;
    }

    #ifdef __WXMSW__
    wxString wST = file->Read(wxT("Win7Taskbar"));
      if ( wST == wxT("An") || wST == wxEmptyString )
      {
          WinSevenTaskbarEnabled = true;
      }
      else
      {
          WinSevenTaskbarEnabled = false;
      }
    #endif

    Worker = new TUWorkerThread();
    if ( FromStartWindow == true )
    {
        curl_share_setopt(tulink, CURLSHOPT_LOCKFUNC, lock_function);
        curl_share_setopt(tulink, CURLSHOPT_UNLOCKFUNC, unlock_function);
        Worker->GetBuddysInstantly = true;
    }
    else
    {
        tulink = curl_share_init();
        curl_share_setopt(tulink, CURLSHOPT_LOCKFUNC, lock_function);
        curl_share_setopt(tulink, CURLSHOPT_UNLOCKFUNC, unlock_function);
        Worker->GetBuddysInstantly = false;
        Worker->username = this->username = file->Read(wxT("Autologin"));
        Worker->password = DecryptString(file->Read(Worker->username + wxT("Passwort")));
    }
    Worker->Create();
    Worker->parent = this;
    Worker->Run();

    delete file;

    #ifdef __WXGTK__
        TrayIcon->SetIcon(wxIcon(GetDataPath() + wxT("/TUKlein.png"), wxBITMAP_TYPE_PNG), username + wxT(" - Online"));
    #elif defined __WXMSW__
        TrayIcon->SetIcon(wxIcon(GetDataPath() + wxT("/TUKlein.ico"), wxBITMAP_TYPE_ICO), username + wxT(" - Online"));
    #endif

    // Smilieliste erstellen
    wxDir().GetAllFiles(GetDataPath() + wxT("/smilies"), &Smilies, wxEmptyString);
    bool PathStyle = true;
    #ifdef __WXMSW__
        if ( wxStandardPaths().GetDataDir().Contains(wxT("/")) == true ) PathStyle = false;
    #else
        if ( (GetDataPath()).Contains(wxT("/")) == true ) PathStyle = false;
    #endif
    for ( int FileNum = 0; (size_t)FileNum < Smilies.GetCount(); FileNum++ )
    {
        int BadPos;
        if ( PathStyle == true ) BadPos = Smilies[FileNum].rfind(wxT("\\"));
        else BadPos = Smilies[FileNum].rfind(wxT("/"));

        Smilies[FileNum] = Smilies[FileNum].Remove(0, BadPos + 1);
        Smilies[FileNum] = Smilies[FileNum].Remove(Smilies[FileNum].Length() - 4);
    }
}

void TUBuddylist::ThreadReport(wxCommandEvent &evt)
{
    if ( evt.GetInt() == WRONGLOGIN )
    {
        EnableOfflineMode(SUSPENDED);
        wxMessageBox(wxT("Es sind falsche Logindaten abgespeichert.\nBitte in den Einstellungen abändern."), wxT("Error: Falsche Logindaten"), wxICON_ERROR | wxOK);
    }
    else if ( evt.GetInt() == NOCONNECTION )
    {
        EnableOfflineMode(CONNLOST);
        wxMessageBox(wxT("Der TU Messenger kann keine Verbindung zu team-ulm.de aufbauen.\nBitte überprüfe deine Internetverbindung.\nDer TU Messenger befindet sich jetzt im Offline Modus."), wxT("Error: Keine Verbindung möglich"), wxICON_ERROR | wxOK);
    }
    else if ( evt.GetInt() == BUDDY_HTML )
    {
        if ( VBuddylist.size() == 0 ) // wxSimpleHtmllistbox lädt viele Buddys mit einem Arraystring CPU schonender und schneller.
        {
            CurrMode = NORMAL;
            #ifdef __WXMSW__
                if ( WinSevenTaskbarEnabled == true && wxPlatformInfo::Get().CheckOSVersion(6, 1) )
                {
                    HICON hIcon = (HICON)LoadImage(NULL, (GetDataPath() + wxT("/on_overlay.ico")).c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
                    m_taskbarInterface->SetOverlayIcon(this->GetHandle(), hIcon, L"Online");
                }
            #endif

            wxString BuddysHtml = evt.GetString();
            wxArrayString Add;
            wxArrayString IdsPicLoader;

            StatBar->SetStatusText(wxT("Online"), 1);

            if ( BuddysHtml.Contains(wxT(" online</strong></div>")) )
            {
                wxString OnBuddys = FilterString(BuddysHtml, wxT(" online</strong></div>"), wxT("<div class=\"dropdownDiv\" style=\"margin-top:20px;\""));
                wxString Buddy;

                bool scndP = false;
                wxStringTokenizer tkz(OnBuddys, wxT("\n"));
                while ( tkz.HasMoreTokens() )
                {
                    wxString part = tkz.GetNextToken();
                    if ( part.Contains(wxT("<div class=\"buddyBody\">")) && scndP == false )
                    {
                        Buddy = part;
                        scndP = true;
                        continue;
                    }
                    else if ( ( part.Contains(wxT("<div class=\"buddyStatus")) || part == wxT("   ") || part == wxT("          ") ) && scndP == true )
                    {
                        Buddy.Append(part);
                    }
                    else continue;

                    scndP = false;

                    VBuddy ThisBuddy;
                    ThisBuddy.connection = ONLINE;
                    ThisBuddy.id = FilterString(Buddy, wxT("<a href=\"/NeueNachricht/"), wxT("\"><i"));
                    ThisBuddy.username = FilterString(Buddy, wxT("</a><a href=\"/Profil/") + ThisBuddy.id + wxT("\">"), wxT("</a>"));
                    if ( Buddy.Contains(wxT("<img src=\"/grafiken/profil/frau.gif\"")) ) ThisBuddy.gender = FEMALE;
                    else if ( Buddy.Contains(wxT("<img src=\"/grafiken/profil/mann.gif\"")) ) ThisBuddy.gender = MALE;
                    else ThisBuddy.gender = NOGENDER;
                    if ( Buddy.Contains(wxT("<span style=\"color:#686868;\">")) ) ThisBuddy.age = FilterString(Buddy, wxT("<span style=\"color:#686868;\"> - "), wxT("</span>"));
                    else ThisBuddy.age = NOAGE;
                    if ( Buddy.Contains(wxT("<div class=\"buddyStatus\">")) ) ThisBuddy.status = FilterString(Buddy, wxT("\"buddyStatus\"><em>"), wxT("</em></div>"));
                    else ThisBuddy.status = NOSTATUS;

                    IdsPicLoader.Add(ThisBuddy.id);
                    VBuddylist.insert(VBuddylist.begin() + VBuddylist.size(), ThisBuddy);

                    if ( LargeViewItem->IsChecked() == true )
                    {
                        Add.Add(wxT("<table><tr><td><img src=\"") + GetDataPath() + wxT("/online.png") + wxT("\"></td><td><font size=\"3\">") + ThisBuddy.username + wxT("</font></td></tr></table>"));
                        // Vielleicht später irgendwann..
                        //Add.Add(wxT("<table width=\"100%\"><tr><td><img src=\"") + GetDataPath() + wxT("/online.png") +
                        //        wxT("\"></td><td width=\"100%\">") + ThisBuddy.username + wxT("</td><td align=right><img width=16 height=16 src=\"") + wxStandardPaths().Get().GetUserDataDir() + wxT("/Profilbilder/") + ThisBuddy.id + wxT(".jpg") + wxT("\"></td></tr></table>"));
                    }
                    else
                    {
                        Add.Add(wxT("<table><tr><td><img src=\"") + GetDataPath() + wxT("/online_klein.png") + wxT("\"></td><td><font size=\"2\">") + ThisBuddy.username + wxT("</font></td></tr></table>"));
                    }
                }
            }
            if ( BuddysHtml.Contains(wxT(" offline</strong>")) )
            {
                wxString OnBuddys = FilterString(BuddysHtml, wxT(" offline</strong>"), wxT(" \n<div class=\"dropdownDiv\" style=\"margin-top:20px;\""));
                wxString Buddy;

                bool scndP = false;
                wxStringTokenizer tkz(OnBuddys, wxT("\n"));
                while ( tkz.HasMoreTokens() )
                {
                    wxString part = tkz.GetNextToken();
                    if ( part.Contains(wxT("<div class=\"buddyBody\">")) && scndP == false )
                    {
                        Buddy = part;
                        scndP = true;
                        continue;
                    }
                    else if ( ( part.Contains(wxT("<div class=\"buddyStatus")) || part == wxT("   ") || part == wxT("          ") ) && scndP == true )
                    {
                        Buddy.Append(part);
                    }
                    else continue;

                    scndP = false;

                    VBuddy ThisBuddy;
                    ThisBuddy.connection = OFFLINE;
                    ThisBuddy.id = FilterString(Buddy, wxT("<a href=\"/NeueNachricht/"), wxT("\"><i"));
                    ThisBuddy.username = FilterString(Buddy, wxT("</a><a href=\"/Profil/") + ThisBuddy.id + wxT("\">"), wxT("</a>"));
                    if ( Buddy.Contains(wxT("<img src=\"/grafiken/profil/frau.gif\"")) ) ThisBuddy.gender = FEMALE;
                    else if ( Buddy.Contains(wxT("<img src=\"/grafiken/profil/mann.gif\"")) ) ThisBuddy.gender = MALE;
                    else ThisBuddy.gender = NOGENDER;
                    if ( Buddy.Contains(wxT("<span style=\"color:#686868;\">")) ) ThisBuddy.age = FilterString(Buddy, wxT("<span style=\"color:#686868;\"> - "), wxT("</span>"));
                    else ThisBuddy.age = NOAGE;
                    if ( Buddy.Contains(wxT("<div class=\"buddyStatus\">")) ) ThisBuddy.status = FilterString(Buddy, wxT("\"buddyStatus\"><em>"), wxT("</em></div>"));
                    else ThisBuddy.status = NOSTATUS;

                    IdsPicLoader.Add(ThisBuddy.id);
                    VBuddylist.insert(VBuddylist.begin() + VBuddylist.size(), ThisBuddy);

                    if ( LargeViewItem->IsChecked() == true )
                    {
                        Add.Add(wxT("<table><tr><td><img src=\"") + GetDataPath() + wxT("/offline.png") + wxT("\"></td><td><font size=\"3\">") + ThisBuddy.username + wxT("</font></td></tr></table>"));
                    }
                    else
                    {
                        Add.Add(wxT("<table><tr><td><img src=\"") + GetDataPath() + wxT("/offline_klein.png") + wxT("\"></td><td><font size=\"2\">") + ThisBuddy.username + wxT("</font></td></tr></table>"));
                    }
                }
            }

            PicWorker = new TUPicLoader();
            PicWorker->Create();
            PicWorker->parent = this;
            PicWorker->ids = IdsPicLoader;
            PicWorker->Run();

            Buddylist->Append(Add);

            if ( ChatWindowOpen == true )
            {
                for ( int ChatID = 0; (size_t)ChatID < ChatWin->Chats->GetPageCount(); ChatID++ )
                {
                    BuddyChatWin *Chat = (BuddyChatWin*)ChatWin->Chats->GetPage(ChatID);
                    wxString chName = ChatWin->Chats->GetPageText(ChatID);

                    for ( int cycle = 0; cycle < (int)VBuddylist.size(); cycle++ )
                    {
                        VBuddy Buddy = VBuddylist.at(cycle);

                        if ( Buddy.username == chName )
                        {
                            if ( Buddy.connection == ONLINE )
                                Chat->StatusImage->SetBitmap(wxBitmap(GetDataPath() + wxT("/online.png"), wxBITMAP_TYPE_PNG));
                            else
                                Chat->StatusImage->SetBitmap(wxBitmap(GetDataPath() + wxT("/offline.png"), wxBITMAP_TYPE_PNG));

                            Chat->Userstatus->SetLabel(wxHtmlEntitiesParser().Parse(Buddy.status));
                        }
                    }
                }
            }
        }
        else // Normale Prozedur
        {
            int CurrNum = 0;
            bool found = false;
            wxString BuddysHtml = evt.GetString();
            wxArrayString NewBuddys;

            if ( BuddysHtml.Contains(wxT(" online</strong></div>")) )
            {
                wxString OnBuddys = FilterString(BuddysHtml, wxT(" online</strong></div>"), wxT("<div class=\"dropdownDiv\" style=\"margin-top:20px;\""));
                wxString Buddy;

                bool scndP = false;
                wxStringTokenizer tkz(OnBuddys, wxT("\n"));
                while ( tkz.HasMoreTokens() )
                {
                    wxString part = tkz.GetNextToken();
                    if ( part.Contains(wxT("<div class=\"buddyBody\">")) && scndP == false )
                    {
                        Buddy = part;
                        scndP = true;
                        continue;
                    }
                    else if ( ( part.Contains(wxT("<div class=\"buddyStatus")) || part == wxT("   ") || part == wxT("          ") ) && scndP == true )
                    {
                        Buddy.Append(part);
                    }
                    else continue;

                    scndP = false;

                    VBuddy ThisBuddy;
                    ThisBuddy.id = FilterString(Buddy, wxT("<a href=\"/NeueNachricht/"), wxT("\"><i"));
                    NewBuddys.Add(ThisBuddy.id);

                    for ( int cycle = 0; cycle < (int)VBuddylist.size(); cycle++ )
                    {
                        VBuddy OldBuddy = VBuddylist.at(cycle);

                        if ( ThisBuddy.id == OldBuddy.id )
                        {
                            found = true;

                            if ( Buddy.Contains(wxT("<div class=\"buddyStatus\">")) ) ThisBuddy.status = FilterString(Buddy, wxT("\"buddyStatus\"><em>"), wxT("</em></div>"));
                            else ThisBuddy.status = NOSTATUS;

                            if ( OldBuddy.connection == OFFLINE ) // Buddy kommt Online
                            {
                                OldBuddy.connection = ONLINE;

                                VBuddylist.erase(VBuddylist.begin() + cycle);
                                VBuddylist.insert(VBuddylist.begin() + CurrNum, OldBuddy);

                                Buddylist->Delete(cycle);
                                if ( LargeViewItem->IsChecked() == true )
                                {
                                    Buddylist->Insert(wxT("<table><tr><td><img src=\"") + GetDataPath() + wxT("/online.png") + wxT("\"></td><td><font size=\"3\">") + OldBuddy.username + wxT("</font></td></tr></table>"), CurrNum);
                                }
                                else
                                {
                                    Buddylist->Insert(wxT("<table><tr><td><img src=\"") + GetDataPath() + wxT("/online_klein.png") + wxT("\"></td><td><font size=\"2\">") + OldBuddy.username + wxT("</font></td></tr></table>"), CurrNum);
                                }

                                if ( ChatWindowOpen == true )
                                {
                                    for ( int ChatID = 0; (size_t)ChatID < ChatWin->Chats->GetPageCount(); ChatID++ )
                                    {
                                        if ( ChatWin->Chats->GetPageText(ChatID) == OldBuddy.username )
                                        {
                                            BuddyChatWin *Chat = (BuddyChatWin*)ChatWin->Chats->GetPage(ChatID);
                                            Chat->StatusImage->SetBitmap(wxBitmap(GetDataPath() + wxT("/online.png"), wxBITMAP_TYPE_PNG));

                                            wxString tmp = wxT("<font color=\"#000000\" size=\"2\">(") + wxDateTime().Now().FormatISOTime() + wxT(") <b>") + OldBuddy.username + wxT(" hat sich angemeldet</b></font>");
                                            Chat->AddMsg(tmp);
                                        }
                                    }
                                }
                            }
                            if ( OldBuddy.status != ThisBuddy.status ) // Buddy hat seinen Status geändert
                            {
                                OldBuddy.status = ThisBuddy.status;

                                VBuddylist.erase(VBuddylist.begin() + CurrNum);
                                VBuddylist.insert(VBuddylist.begin() + CurrNum, OldBuddy);

                                if ( ChatWindowOpen == true )
                                {
                                    for ( int ChatID = 0; (size_t)ChatID < ChatWin->Chats->GetPageCount(); ChatID++ )
                                    {
                                        if ( ChatWin->Chats->GetPageText(ChatID) == OldBuddy.username )
                                        {
                                            BuddyChatWin *Chat = (BuddyChatWin*)ChatWin->Chats->GetPage(ChatID);
                                            Chat->Userstatus->SetLabel(wxHtmlEntitiesParser().Parse(OldBuddy.status));
                                        }
                                    }
                                }
                            }

                            break;
                        }
                    }
                    if ( found == false ) // Buddy wurde neu hinzugefügt
                    {
                        ThisBuddy.connection = ONLINE;
                        ThisBuddy.username = FilterString(Buddy, wxT("</a><a href=\"/Profil/") + ThisBuddy.id + wxT("\">"), wxT("</a>"));
                        if ( Buddy.Contains(wxT("<img src=\"/grafiken/profil/frau.gif\"")) ) ThisBuddy.gender = FEMALE;
                        else if ( Buddy.Contains(wxT("<img src=\"/grafiken/profil/mann.gif\"")) ) ThisBuddy.gender = MALE;
                        else ThisBuddy.gender = NOGENDER;
                        if ( Buddy.Contains(wxT("<span style=\"color:#686868;\">")) ) ThisBuddy.age = FilterString(Buddy, wxT("<span style=\"color:#686868;\"> - "), wxT("</span>"));
                        else ThisBuddy.age = NOAGE;
                        if ( Buddy.Contains(wxT("<div class=\"buddyStatus\">")) ) ThisBuddy.status = FilterString(Buddy, wxT("\"buddyStatus\"><em>"), wxT("</em></div>"));
                        else ThisBuddy.status = NOSTATUS;

                        VBuddylist.insert(VBuddylist.begin() + CurrNum, ThisBuddy);
                        if ( LargeViewItem->IsChecked() == true )
                        {
                            Buddylist->Insert(wxT("<table><tr><td><img src=\"") + GetDataPath() + wxT("/online.png") + wxT("\"></td><td><font size=\"3\">") + ThisBuddy.username + wxT("</font></td></tr></table>"), CurrNum);
                        }
                        else
                        {
                            Buddylist->Insert(wxT("<table><tr><td><img src=\"") + GetDataPath() + wxT("/online_klein.png") + wxT("\"></td><td><font size=\"2\">") + ThisBuddy.username + wxT("</font></td></tr></table>"), CurrNum);
                        }
                    }
                    found = false;

                    CurrNum++;
                }
            }
            if ( BuddysHtml.Contains(wxT(" offline</strong>")) )
            {
                wxString OnBuddys = FilterString(BuddysHtml, wxT(" offline</strong>"), wxT("<div class=\"dropdownDiv\" style=\"margin-top:20px;\""));
                wxString Buddy;

                bool scndP = false;
                wxStringTokenizer tkz(OnBuddys, wxT("\n"));
                while ( tkz.HasMoreTokens() )
                {
                    wxString part = tkz.GetNextToken();
                    if ( part.Contains(wxT("<div class=\"buddyBody\">")) && scndP == false )
                    {
                        Buddy = part;
                        scndP = true;
                        continue;
                    }
                    else if ( ( part.Contains(wxT("<div class=\"buddyStatus")) || part == wxT("   ") || part == wxT("          ") ) && scndP == true )
                    {
                        Buddy.Append(part);
                    }
                    else continue;

                    scndP = false;

                    VBuddy ThisBuddy;
                    ThisBuddy.id = FilterString(Buddy, wxT("<a href=\"/NeueNachricht/"), wxT("\"><i"));
                    NewBuddys.Add(ThisBuddy.id);

                    for ( int cycle = 0; cycle < (int)VBuddylist.size(); cycle++ )
                    {
                        VBuddy OldBuddy = VBuddylist.at(cycle);

                        if ( ThisBuddy.id == OldBuddy.id )
                        {
                            found = true;

                            if ( Buddy.Contains(wxT("<div class=\"buddyStatus\">")) ) ThisBuddy.status = FilterString(Buddy, wxT("\"buddyStatus\"><em>"), wxT("</em></div>"));
                            else ThisBuddy.status = NOSTATUS;

                            if ( OldBuddy.connection == ONLINE ) // Buddy geht Offline
                            {
                                OldBuddy.connection = OFFLINE;

                                VBuddylist.erase(VBuddylist.begin() + cycle);
                                VBuddylist.insert(VBuddylist.begin() + CurrNum, OldBuddy);

                                Buddylist->Delete(cycle);

                                if ( LargeViewItem->IsChecked() == true )
                                {
                                    Buddylist->Insert(wxT("<table><tr><td><img src=\"") + GetDataPath() + wxT("/offline.png") + wxT("\"></td><td><font size=\"3\">") + OldBuddy.username + wxT("</font></td></tr></table>"), CurrNum);
                                }
                                else
                                {
                                    Buddylist->Insert(wxT("<table><tr><td><img src=\"") + GetDataPath() + wxT("/offline_klein.png") + wxT("\"></td><td><font size=\"2\">") + OldBuddy.username + wxT("</font></td></tr></table>"), CurrNum);
                                }

                                if ( ChatWindowOpen == true )
                                {
                                    for ( int ChatID = 0; (size_t)ChatID < ChatWin->Chats->GetPageCount(); ChatID++ )
                                    {
                                        if ( ChatWin->Chats->GetPageText(ChatID) == OldBuddy.username )
                                        {
                                            BuddyChatWin *Chat = (BuddyChatWin*)ChatWin->Chats->GetPage(ChatID);
                                            Chat->StatusImage->SetBitmap(wxBitmap(GetDataPath() + wxT("/offline.png"), wxBITMAP_TYPE_PNG));

                                            wxString tmp = wxT("<font color=\"#000000\" size=\"2\">(") + wxDateTime().Now().FormatISOTime() + wxT(") <b>") + OldBuddy.username + wxT(" hat sich abgemeldet</b></font>");
                                            Chat->AddMsg(tmp);
                                        }
                                    }
                                }
                            }
                            if ( OldBuddy.status != ThisBuddy.status ) // Buddy hat seinen Status geändert
                            {
                                OldBuddy.status = ThisBuddy.status;

                                VBuddylist.erase(VBuddylist.begin() + CurrNum);
                                VBuddylist.insert(VBuddylist.begin() + CurrNum, OldBuddy);

                                if ( ChatWindowOpen == true )
                                {
                                    for ( int ChatID = 0; (size_t)ChatID < ChatWin->Chats->GetPageCount(); ChatID++ )
                                    {
                                        if ( ChatWin->Chats->GetPageText(ChatID) == OldBuddy.username )
                                        {
                                            BuddyChatWin *Chat = (BuddyChatWin*)ChatWin->Chats->GetPage(ChatID);
                                            Chat->Userstatus->SetLabel(wxHtmlEntitiesParser().Parse(OldBuddy.status));
                                        }
                                    }
                                }
                            }

                            break;
                        }
                    }
                    if ( found == false ) // Buddy wurde neu hinzugefügt
                    {
                        ThisBuddy.connection = OFFLINE;
                        ThisBuddy.username = FilterString(Buddy, wxT("</a><a href=\"/Profil/") + ThisBuddy.id + wxT("\">"), wxT("</a>"));
                        if ( Buddy.Contains(wxT("<img src=\"/grafiken/profil/frau.gif\"")) ) ThisBuddy.gender = FEMALE;
                        else if ( Buddy.Contains(wxT("<img src=\"/grafiken/profil/mann.gif\"")) ) ThisBuddy.gender = MALE;
                        else ThisBuddy.gender = NOGENDER;
                        if ( Buddy.Contains(wxT("<span style=\"color:#686868;\">")) ) ThisBuddy.age = FilterString(Buddy, wxT("<span style=\"color:#686868;\"> - "), wxT("</span>"));
                        else ThisBuddy.age = NOAGE;
                        if ( Buddy.Contains(wxT("<div class=\"buddyStatus\">")) ) ThisBuddy.status = FilterString(Buddy, wxT("\"buddyStatus\"><em>"), wxT("</em></div>"));
                        else ThisBuddy.status = NOSTATUS;

                        VBuddylist.insert(VBuddylist.begin() + CurrNum, ThisBuddy);
                        if ( LargeViewItem->IsChecked() == true )
                        {
                            Buddylist->Insert(wxT("<table><tr><td><img src=\"") + GetDataPath() + wxT("/offline.png") + wxT("\"></td><td><font size=\"3\">") + ThisBuddy.username + wxT("</font></td></tr></table>"), CurrNum);
                        }
                        else
                        {
                            Buddylist->Insert(wxT("<table><tr><td><img src=\"") + GetDataPath() + wxT("/offline_klein.png") + wxT("\"></td><td><font size=\"2\">") + ThisBuddy.username + wxT("</font></td></tr></table>"), CurrNum);
                        }
                    }
                    found = false;

                    CurrNum++;
                }
            }
            for ( int cycle2 = 0; cycle2 < (int)VBuddylist.size(); cycle2++ )
            {
                bool found = false;
                VBuddy Buddy = VBuddylist.at(cycle2);

                for ( int cycle3 = 0; cycle3 < (int)NewBuddys.GetCount(); cycle3++ )
                {
                    if ( Buddy.id == NewBuddys[cycle3] )
                    {
                        found = true;
                    }
                }

                if ( found == false ) // Buddy wurde gelöscht
                {
                    VBuddylist.erase(VBuddylist.begin() + cycle2);
                    Buddylist->Delete(cycle2);
                }
            }
        }
    }
    else if ( evt.GetInt() == PICSFINISHED )
    {
        PicsUsable = true;
        BIconItem->Enable(true);
        PicWorker = NULL;

        if ( ChatWindowOpen == true )
        {
            for ( int ChatID = 0; (size_t)ChatID < ChatWin->Chats->GetPageCount(); ChatID++ )
            {
                BuddyChatWin *Chat = (BuddyChatWin*)ChatWin->Chats->GetPage(ChatID);

                wxString TempPath = wxStandardPaths().Get().GetUserDataDir() + wxT("/Profilbilder/") + Chat->BuddyID + wxT(".jpg");
                if ( wxFileName().GetSize(TempPath) != 0 && wxFile::Exists(TempPath) == true )
                {
                    wxImage *UserPicImage = new wxImage(TempPath);
                    wxImage ScaledUserPic = UserPicImage->Scale(40, 40, wxIMAGE_QUALITY_HIGH);
                    delete UserPicImage;
                    Chat->UserPic->SetBitmap(ScaledUserPic);
                }
            }
        }
    }
    else if ( evt.GetInt() == VERSION_CHECK )
    {
        if ( evt.GetString() != VERSIONSTRING && !wxString(VERSIONSTRING).Contains("(DEV)") )
        {
            wxString Message = wxT("Es ist eine neue Version verfügbar!\n\nInstallierte Version: ");
            int retn = wxMessageBox(Message + VERSIONSTRING + wxT("\nNeueste Version: ") + evt.GetString() + wxT("\n\nWenn du auf OK drückst öffnet sich die Downloadseite."), wxT("Aktualisierung verfügbar"), wxCANCEL | wxOK | wxICON_INFORMATION | wxCENTRE, this);
            if ( retn == wxOK )
            {
                wxLaunchDefaultBrowser(wxT("http://tumessenger.ouned.de/"));
            }
        }
    }
    else if ( evt.GetInt() == MSGSEND_REPORT )
    {
        wxArrayString Msg = MsgQueue.front();

        if ( evt.GetString() == wxT("SUCCESS") )
        {
            // Nachricht erfolgreich verschickt
            wxString NewMsgS = Msg[2];
            NewMsgS.Replace(wxT(" ") + username + wxT(":"), wxT(" <b>") + username + wxT(":</b>"), false);

            if ( ChatWindowOpen == true )
            {
                for ( int ChatID = 0; (size_t)ChatID < ChatWin->Chats->GetPageCount(); ChatID++ )
                {
                    if ( ChatWin->Chats->GetPageText(ChatID) == Msg[0] )
                    {
                        BuddyChatWin *Chat = (BuddyChatWin*)ChatWin->Chats->GetPage(ChatID);
                        Chat->MarkMessage(Msg[2], NewMsgS);
                    }
                }
            }
        }
        else if ( evt.GetString() == wxT("UNKNOWNUSER") )
        {
            // Dieser Benutzer existiert nicht
            if ( ChatWindowOpen == true )
            {
                for ( int ChatID = 0; (size_t)ChatID < ChatWin->Chats->GetPageCount(); ChatID++ )
                {
                    if ( ChatWin->Chats->GetPageText(ChatID) == Msg[0] )
                    {
                        BuddyChatWin *Chat = (BuddyChatWin*)ChatWin->Chats->GetPage(ChatID);
                        Chat->AddMsg(wxT("<font color=\"#B40404\" size=\"2\">(") + wxDateTime().Now().FormatISOTime() + wxT(") <b>") + Msg[0] + wxT(" existiert nicht!</b></font>"));
                    }
                }
            }
        }
        else
        {
            // Nachricht nicht verschickt
            wxString NewMsgS = Msg[2];
            NewMsgS.Replace(wxT(" ") + username + wxT(": </font>"), wxT(" </font><b><font color=\"#FC9D30\" size=\"2\">") + username + wxT(": </b></font>"), false);

            if ( ChatWindowOpen == true )
            {
                for ( int ChatID = 0; (size_t)ChatID < ChatWin->Chats->GetPageCount(); ChatID++ )
                {
                    if ( ChatWin->Chats->GetPageText(ChatID) == Msg[0] )
                    {
                        BuddyChatWin *Chat = (BuddyChatWin*)ChatWin->Chats->GetPage(ChatID);
                        Chat->MarkMessage(Msg[2], NewMsgS);
                    }
                }
            }
        }

        MsgQueue.erase(MsgQueue.begin());
        MsgSender = NULL;

        lastMsgSent = wxDateTime().GetTicks();
        if ( MsgQueue.size() > 0 ) MsgSendTimer->Start(8000, true);
        else
        {
            #ifdef __WXMSW__
                if ( ChatWindowOpen && WinSevenTaskbarEnabled == true && wxPlatformInfo::Get().CheckOSVersion(6, 1) )
                {
                    m_taskbarInterface->SetProgressState(ChatWin->GetHandle(), TBPF_NOPROGRESS);
                }
            #endif
        }
    }
    else if ( evt.GetInt() == TREPORT_CLOST )
    {
        EnableOfflineMode(CONNLOST);
    }
    else if ( evt.GetInt() == TREPORT_CBACK )
    {
        DisableOfflineMode();
    }
    else if ( evt.GetInt() == STATUS_CHANGED )
    {
        wxString status = evt.GetString();

        if ( StatusBox->GetValue() != status )
        {
            StatusBox->ChangeValue(status);
            StatusButton->Enable(false);
        }
    }
}

void TUBuddylist::ReloadBPics(wxCommandEvent &event)
{
    PicsUsable = false; BIconItem->Enable(false);
    wxRmDir(wxStandardPaths().Get().GetUserDataDir() + wxT("/Profilbilder"));

    wxArrayString idss;
    for ( int id = 0; id < (int)VBuddylist.size(); id++ )
    {
        VBuddy TheBuddy = VBuddylist.at(id);
        idss.Add(TheBuddy.id);
    }

    PicWorker = new TUPicLoader();
    PicWorker->Create();
    PicWorker->parent = this;
    PicWorker->ids = idss;
    PicWorker->Run();
}

void TUBuddylist::UpdateClicked(wxCommandEvent&)
{
    wxString CurrVer = tu_getversionstring();
    if ( CurrVer.Contains(wxT("3")) )
    {
        if ( CurrVer != VERSIONSTRING )
        {
            wxString Message = wxT("Es ist eine neue Version verfügbar!\n\nInstallierte Version: ");
            int retn = wxMessageBox(Message + VERSIONSTRING + wxT("\nNeueste Version: ") + CurrVer + wxT("\n\nWenn du auf OK drückst öffnet sich die Downloadseite."), wxT("Aktualisierung verfügbar"), wxCANCEL | wxOK | wxICON_INFORMATION | wxCENTRE, this);
            if ( retn == wxOK )
            {
                wxLaunchDefaultBrowser(wxT("http://tumessenger.ouned.de/"));
            }
        }
        else
        {
            wxMessageBox(wxT("Du hast bereits die aktuellste\nVersion des TUMessengers. :)"), wxT("Info: Keine neue Version"), wxICON_INFORMATION | wxOK);
        }
    }
    else
    {
        wxMessageBox(wxT("Die Versionsprüfung ist fehlgeschlagen.\nBitte überprüfe deine Internetverbindung."), wxT("Error: Versionsprüfung fehlgeschlagen"), wxICON_ERROR | wxOK);
    }
}

MessageEvent::MessageEvent() { SetEventType(NEW_MESSAGE_EVT); }

void TUBuddylist::OnNewMsg(MessageEvent &event)
{
    if ( ChatWindowOpen == false )
    {
        ChatWindowOpen = true;
        ChatWin = new TUChatwindow(NULL, true);
        ChatWin->parent = this;
        ChatWin->UserName = username;
        ChatWin->Show(false);

        #ifdef __WXMSW__
            if ( WinSevenTaskbarEnabled == true && wxPlatformInfo::Get().CheckOSVersion(6, 1) && MsgQueue.size() != 0 )
            {
                m_taskbarInterface->SetProgressState(ChatWin->GetHandle(), TBPF_INDETERMINATE);
            }
        #endif
    }

    BuddyChatWin *Chat = NULL;
    int ThisMsgTab = 0;

    bool NewTabNeeded = true;
    for ( int ChatID = 0; (size_t)ChatID < ChatWin->Chats->GetPageCount(); ChatID++ )
    {
        if ( ChatWin->Chats->GetPageText(ChatID) == event.VMsg.username )
        {
            Chat = (BuddyChatWin*)ChatWin->Chats->GetPage(ChatID);
            ThisMsgTab = ChatID;
            NewTabNeeded = false;
        }
    }
    if ( NewTabNeeded == true )
    {
        Chat = new BuddyChatWin(ChatWin);
        Chat->parent = ChatWin;
        Chat->BuddyID = event.VMsg.userid;

        wxString TempPath = wxStandardPaths().Get().GetUserDataDir() + wxT("/Profilbilder/") + event.VMsg.userid + wxT(".jpg");
        if ( !PicsUsable || wxFileName().GetSize(TempPath) == 0 || wxFile::Exists(TempPath) == false )
        {
            TempPath = GetDataPath() + wxT("/KeinProfilbild.png");
        }

        bool WriterInList = false;
        VBuddy Writer;
        for ( int cycle = 0; cycle < (int)VBuddylist.size(); cycle++ )
        {
            Writer = VBuddylist.at(cycle);
            if ( Writer.id == event.VMsg.userid )
            {
                WriterInList = true;
                break;
            }
        }

        if ( WriterInList == true )
        {
            Chat->InitGui(event.VMsg.username, TempPath, wxHtmlEntitiesParser().Parse(Writer.status), Writer.connection);
        }
        else
        {
            Chat->InitGui(event.VMsg.username, TempPath, wxT("ACHTUNG: Dieser User befindet sich nicht in deiner Buddyliste! Keine Informationen verfügbar."), ONLINE);
        }
        ChatWin->Chats->AddPage(Chat, event.VMsg.username, false);

        ThisMsgTab = ChatWin->Chats->GetPageCount() - 1;
    }

    wxString MsgS = event.VMsg.message;
    while ( MsgS.Contains(wxT("<br />\r\n<br />")) == true ) { MsgS.Replace(wxT("<br />\r\n<br />"), wxT("<br />"), true); }
    if ( MsgS.rfind(wxT("<br />")) == MsgS.Length() - 8 ) MsgS = MsgS.Remove(MsgS.Length() - 8);

    // Links
    int LastSearchPos = 0;
    while ( MsgS.find(wxT("://"), LastSearchPos) != (size_t)wxNOT_FOUND )
    {
        int LinkStartPos, LinkEndPos;
        wxString TempCut;
        wxString Link;

        LinkStartPos = MsgS.find(wxT("://"), LastSearchPos);
        if ( MsgS.Mid(LinkStartPos - 4, 7) != wxT("http://") )
        {
            if ( MsgS.Mid(LinkStartPos - 3, 6) != wxT("ftp://") )
            {
                if ( MsgS.Mid(LinkStartPos - 5, 8) != wxT("https://") )
                {
                    LastSearchPos = LinkStartPos + 3;
                    continue;
                }
                else
                {
                    LinkStartPos -= 5;
                }
            }
            else
            {
                LinkStartPos -= 3;
            }
        }
        else
        {
            LinkStartPos -= 4;
        }

        if ( LinkStartPos != 0 )
        {
            wxString tmp = MsgS;
            TempCut = tmp.Remove(0, LinkStartPos);
            if ( MsgS.GetChar(LinkStartPos - 1) != wxChar('\n') && MsgS.GetChar(LinkStartPos - 1) != wxChar(' ') ) { LastSearchPos = LinkStartPos + 6; continue; }
        }
        else TempCut = MsgS;
        int LinkEndPosOne = TempCut.Find(wxT(" "));
        int LinkEndPosTwo = TempCut.Find(wxT("<br />"));

        if ( ( LinkEndPosOne < LinkEndPosTwo && LinkEndPosOne != wxNOT_FOUND ) || ( LinkEndPosTwo == wxNOT_FOUND && LinkEndPosOne != wxNOT_FOUND ) ) LinkEndPos = LinkEndPosOne;
        else if ( ( LinkEndPosTwo < LinkEndPosOne && LinkEndPosTwo != wxNOT_FOUND ) || ( LinkEndPosOne == wxNOT_FOUND && LinkEndPosTwo != wxNOT_FOUND ) ) LinkEndPos = LinkEndPosTwo;
        else LinkEndPos = MsgS.Length() - 1;

        if ( LinkEndPos != (int)MsgS.Length() - 1 ) Link = TempCut.Remove(LinkEndPos);
        else Link = TempCut;
        LastSearchPos = LinkEndPos;

        MsgS.Replace(Link, wxT("<a href=\"") + Link + wxT("\">") + Link + wxT("</a>"));
    }

    // Smilies
    wxString Path = GetDataPath() + wxT("/smilies/");
    MsgS.Replace(wxT("<img src=\"/grafiken/smilies/"), wxT("&nbsp;<img src=\"") + Path, true);

    wxString MsgHtml;
    if ( event.VMsg.subject == wxEmptyString )
    {
        MsgHtml = wxT("<font color=\"#B40404\" size=\"2\">(") + wxDateTime().Now().FormatISOTime() + wxT(") <b>") + event.VMsg.username + wxT(":</b> </font><font color=\"#000000\" size=\"2\">") + MsgS + wxT("</font>");
    }
    else
    {
        MsgHtml = wxT("<font color=\"#B40404\" size=\"2\">(") + wxDateTime().Now().FormatISOTime() + wxT(") <b>") + event.VMsg.username + wxT(":</b> </font><font color=\"#000000\" size=\"2\"><b>") + event.VMsg.subject + wxT("</b><br>") + MsgS + wxT("</font>");
    }
    Chat->AddMsg(MsgHtml);

    Chat->NewMessage = true;
    if ( SoundsEnabled == true ) wxSound(GetDataPath() + wxT("/NeueNachricht.wav")).Play();
    if ( ChatWin->Chats->GetSelection() != ThisMsgTab )
    {
        ChatWin->Chats->SetPageBitmap(ThisMsgTab, wxBitmap(GetDataPath() + wxT("/NeueNachricht.png"), wxBITMAP_TYPE_PNG));
    }
    #ifdef __WXMSW__
        if ( !ChatWin->IsActive() ) ChatWin->RequestUserAttention(wxUSER_ATTENTION_INFO);
    #else
        if ( !ChatWin->IsActive() ) ChatWin->RequestUserAttention(wxUSER_ATTENTION_ERROR);
    #endif
}

void TUBuddylist::BuddylistMouseOver(wxHtmlCellEvent &event)
{
    if ( PopupOpen )
        return;

    wxFindWindowAtPointer(OldPos);
    event.GetCell()->GetRootCell()->GetId().ToULong(&BuddyPopupNum);
    PopupTimer->Start(500, wxTIMER_ONE_SHOT);
    event.Skip(true);
}

BuddyInfoPopup::BuddyInfoPopup(TUBuddylist *parent) : wxPopupWindow(parent, wxBORDER_SIMPLE){}
void BuddyInfoPopup::InitGui()
{
    this->SetBackgroundColour(wxColor(255, 250, 205));
    wxFlexGridSizer *MainSizer = new wxFlexGridSizer(0, 2, 0, 0);
    wxImage *UserPicImage = new wxImage(pathtopic);
    wxImage ScaledUserPic = UserPicImage->Scale(75, 75, wxIMAGE_QUALITY_HIGH);
    UserPicImage->Destroy();
    wxGenericStaticBitmap *UserPic = new wxGenericStaticBitmap(this, wxID_ANY, wxBitmap(ScaledUserPic, wxBITMAP_SCREEN_DEPTH), wxDefaultPosition, wxSize(75, 75), wxBORDER_SIMPLE);
    MainSizer->Add(UserPic, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_TOP, 4);

    wxFlexGridSizer *InfoSizer = new wxFlexGridSizer(0, 1, 0, 0);
    wxStaticText *Userlabel = new wxStaticText(this, wxID_ANY, username, wxDefaultPosition, wxDefaultSize);
    wxFont UserlabelFont(13,wxDEFAULT,wxFONTSTYLE_NORMAL,wxBOLD,false,wxEmptyString,wxFONTENCODING_DEFAULT);
    Userlabel->SetFont(UserlabelFont);
    InfoSizer->Add(Userlabel, 1, wxUP|wxDOWN|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 4);

    if ( age != NOAGE )
    {
        wxFlexGridSizer *AgeSizer = new wxFlexGridSizer(0, 2, 0, 0);
        wxStaticText *Agelabel = new wxStaticText(this, wxID_ANY, wxT("Alter: "), wxDefaultPosition, wxDefaultSize);
        #ifdef __WXGTK__
        wxFont AgelabelFont(9,wxDEFAULT,wxFONTSTYLE_NORMAL,wxBOLD,false,wxEmptyString,wxFONTENCODING_DEFAULT);
        #elif defined __WXMSW__
        wxFont AgelabelFont(wxDEFAULT,wxDEFAULT,wxFONTSTYLE_NORMAL,wxBOLD,false,wxEmptyString,wxFONTENCODING_DEFAULT);
        #endif
        #ifndef __WXMAC__
        Agelabel->SetFont(AgelabelFont);
        #endif
        AgeSizer->Add(Agelabel, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 1);
        wxStaticText *Agetext = new wxStaticText(this, wxID_ANY, age, wxDefaultPosition, wxDefaultSize);
        AgeSizer->Add(Agetext, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 1);
        InfoSizer->Add(AgeSizer, 1, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 0);
    }
    if ( gender != NOGENDER )
    {
        wxFlexGridSizer *GenderSizer = new wxFlexGridSizer(0, 2, 0, 0);
        wxStaticText *Genderlabel = new wxStaticText(this, wxID_ANY, wxT("Geschlecht: "), wxDefaultPosition, wxDefaultSize);
        #ifdef __WXGTK__
        wxFont GenderlabelFont(9,wxDEFAULT,wxFONTSTYLE_NORMAL,wxBOLD,false,wxEmptyString,wxFONTENCODING_DEFAULT);
        #elif defined __WXMSW__
        wxFont GenderlabelFont(wxDEFAULT,wxDEFAULT,wxFONTSTYLE_NORMAL,wxBOLD,false,wxEmptyString,wxFONTENCODING_DEFAULT);
        #endif
        #ifndef __WXMAC__
        Genderlabel->SetFont(GenderlabelFont);
        #endif
        GenderSizer->Add(Genderlabel, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 1);
        wxStaticText *Gendertext = NULL;
        if ( gender == MALE )
        {
            Gendertext = new wxStaticText(this, wxID_ANY, wxT("männlich"), wxDefaultPosition, wxDefaultSize);
        }
        else if ( gender == FEMALE )
        {
            Gendertext = new wxStaticText(this, wxID_ANY, wxT("weiblich"), wxDefaultPosition, wxDefaultSize);
        }
        GenderSizer->Add(Gendertext, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 1);
        InfoSizer->Add(GenderSizer, 1, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 0);
    }
    if ( status != NOSTATUS )
    {
        wxFlexGridSizer *StatusSizer = new wxFlexGridSizer(0, 2, 0, 0);
        wxStaticText *Statuslabel = new wxStaticText(this, wxID_ANY, wxT("Status: "), wxDefaultPosition, wxDefaultSize);
        #ifdef __WXGTK__
        wxFont StatuslabelFont(9,wxDEFAULT,wxFONTSTYLE_NORMAL,wxBOLD,false,wxEmptyString,wxFONTENCODING_DEFAULT);
        #elif defined __WXMSW__
        wxFont StatuslabelFont(wxDEFAULT,wxDEFAULT,wxFONTSTYLE_NORMAL,wxBOLD,false,wxEmptyString,wxFONTENCODING_DEFAULT);
        #endif
        #ifndef __WXMAC__
        Statuslabel->SetFont(StatuslabelFont);
        #endif
        StatusSizer->Add(Statuslabel, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_TOP, 1);
        wxStaticText *Statustext = new wxStaticText(this, wxID_ANY, wxHtmlEntitiesParser().Parse(status), wxDefaultPosition, wxDefaultSize);
        Statustext->Wrap(200);
        StatusSizer->Add(Statustext, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 1);
        InfoSizer->Add(StatusSizer, 1, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 0);
    }

    MainSizer->Add(InfoSizer, 1, wxALIGN_LEFT|wxALIGN_TOP|wxRIGHT, 4);
    SetSizer(MainSizer);
    MainSizer->SetSizeHints(this);
}

void TUBuddylist::ShowPopup(wxTimerEvent &event)
{
    if ( PopupOpen )
        return;

    wxPoint NewPos;
    wxWindow *win = wxFindWindowAtPointer(NewPos);
    if ( NewPos.x == OldPos.x && NewPos.y == OldPos.y && win == Buddylist && BuddyPopupNum < VBuddylist.size() )
    {
        Popup = new BuddyInfoPopup(this);
        PopupOpen = true;

        VBuddy TheBuddy = VBuddylist.at(BuddyPopupNum);

        wxString TempPath = wxStandardPaths().Get().GetUserDataDir() + wxT("/Profilbilder/") + TheBuddy.id + wxT(".jpg");
        if ( PicsUsable == true && wxFileName().GetSize(TempPath) != 0 && wxFile::Exists(TempPath) == true )
        {
            Popup->picuse = true;
            Popup->pathtopic = TempPath;
        }
        else
        {
            Popup->picuse = false;
            Popup->pathtopic = GetDataPath() + wxT("/KeinProfilbild.png");
        }
        Popup->username = TheBuddy.username;
        Popup->age = TheBuddy.age;
        Popup->gender = TheBuddy.gender;
        Popup->status = TheBuddy.status;
        PopupKillTimer->Start(20);
        Popup->InitGui();

        bool WasNegative = false;
        wxRect DisplaySize = wxDisplay(wxDisplay::GetFromPoint(NewPos)).GetClientArea();
        if ( NewPos.x < 0 ) { NewPos.x += DisplaySize.width + DisplaySize.x; WasNegative = true; }
        if ( NewPos.x + Popup->GetSize().x > DisplaySize.width + DisplaySize.x ) NewPos.x = DisplaySize.width + DisplaySize.x - Popup->GetSize().x - 5;
        if ( WasNegative == true ) NewPos.x -= DisplaySize.width + DisplaySize.x;
        Popup->SetPosition(wxPoint(NewPos.x, NewPos.y + 20));
        Popup->SetCanFocus(false);
        Popup->Show(true);
    }
}

void TUBuddylist::KillPopup(wxTimerEvent &event)
{
    wxPoint NewPos;
    wxFindWindowAtPointer(NewPos);

    if ( ( ( NewPos.x - OldPos.x ) >= 5 || ( NewPos.x - OldPos.x ) <= -5 || ( NewPos.y - OldPos.y ) >= 5 || ( NewPos.y - OldPos.y ) <= -5 ) && PopupOpen == true )
    {
        PopupKillTimer->Stop();
        Popup->Destroy();
        PopupOpen = false;
    }
}

void TUBuddylist::NewChat(wxCommandEvent &event)
{
    if ( ChatWindowOpen == false )
    {
        ChatWindowOpen = true;
        ChatWin = new TUChatwindow(NULL, false);
        ChatWin->parent = this;
        ChatWin->UserName = username;
        ChatWin->Show(true);

        #ifdef __WXMSW__
            if ( WinSevenTaskbarEnabled == true && wxPlatformInfo::Get().CheckOSVersion(6, 1) && MsgQueue.size() != 0 )
            {
                m_taskbarInterface->SetProgressState(ChatWin->GetHandle(), TBPF_INDETERMINATE);
            }
        #endif
    }

    VBuddy ChatBuddy = VBuddylist.at(event.GetInt());

    bool NewPage = true;
    for ( int ChatID = 0; (size_t)ChatID < ChatWin->Chats->GetPageCount(); ChatID++ )
    {
        if ( ChatWin->Chats->GetPageText(ChatID) == ChatBuddy.username )
        {
            ChatWin->Chats->SetSelection(ChatID);
            NewPage = false;
        }
    }
    if ( NewPage == true )
    {
        BuddyChatWin *ThisChat = new BuddyChatWin(ChatWin);
        ThisChat->parent = ChatWin;
        ThisChat->BuddyID = ChatBuddy.id;

        wxString TempPath = wxStandardPaths().Get().GetUserDataDir() + wxT("/Profilbilder/") + ChatBuddy.id + wxT(".jpg");
        if ( !PicsUsable || wxFileName().GetSize(TempPath) == 0 || wxFile::Exists(TempPath) == false )
        {
            TempPath = GetDataPath() + wxT("/KeinProfilbild.png");
        }

        ThisChat->InitGui(ChatBuddy.username, TempPath, wxHtmlEntitiesParser().Parse(ChatBuddy.status), ChatBuddy.connection);
        ChatWin->Chats->AddPage(ThisChat, ChatBuddy.username, true);
    }

    if ( ChatWin->IsIconized() == true ) ChatWin->Iconize(false);
}

void TUBuddylist::StartNBChat(wxCommandEvent&)
{
    wxString name = wxGetTextFromUser(wxT("Bitte gebe hier den Nicknamen der Person ein:\nACHTUNG: Es findet keine überprüfung des eingegebenen Nicknamens statt."), wxT("Chat starten..."), wxEmptyString, this);
    if ( name != wxEmptyString )
    {
        if ( ChatWindowOpen == false )
        {
            ChatWindowOpen = true;
            ChatWin = new TUChatwindow(NULL, false);
            ChatWin->parent = this;
            ChatWin->UserName = username;
            ChatWin->Show(true);

            #ifdef __WXMSW__
                if ( WinSevenTaskbarEnabled == true && wxPlatformInfo::Get().CheckOSVersion(6, 1) && MsgQueue.size() != 0 )
                {
                    m_taskbarInterface->SetProgressState(ChatWin->GetHandle(), TBPF_INDETERMINATE);
                }
            #endif
        }

        bool NewPage = true;
        for ( int ChatID = 0; (size_t)ChatID < ChatWin->Chats->GetPageCount(); ChatID++ )
        {
            if ( ChatWin->Chats->GetPageText(ChatID) == name )
            {
                ChatWin->Chats->SetSelection(ChatID);
                NewPage = false;
            }
        }
        if ( NewPage == true )
        {
            BuddyChatWin *ThisChat = new BuddyChatWin(ChatWin);
            ThisChat->parent = ChatWin;
            ThisChat->BuddyID = wxT("N/A");
            ThisChat->InitGui(name, GetDataPath() + wxT("/KeinProfilbild.png"), wxT("ACHTUNG: Dieser User befindet sich nicht in deiner Buddyliste! Keine Informationen verfügbar."), ONLINE);
            ChatWin->Chats->AddPage(ThisChat, name, true);
        }
    }
}

void TUBuddylist::SendMsg(wxTimerEvent &event)
{
    if ( MsgQueue.size() != 0 )
    {
        MsgSender = new TUMsgSender();
        MsgSender->Create();
        MsgSender->Buddylist = this;
        MsgSender->Msg = MsgQueue.front();
        MsgSender->Run();
    }
}

// ------------------------------------------------------------------
// About Dialog

void TUBuddylist::OpenAbout(wxCommandEvent &event)
{
    AboutDialog *InfoWindow = new AboutDialog(this);
    InfoWindow->ShowModal();
}

AboutDialog::AboutDialog(TUBuddylist *par) : wxDialog(par, wxID_ANY, wxT("Über den TUMessenger"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
{
    wxFlexGridSizer *MainSizer = new wxFlexGridSizer(0, 2, 0, 0);
    wxFont HeaderFont(22, wxDEFAULT, wxFONTSTYLE_NORMAL, wxBOLD, false, wxEmptyString, wxFONTENCODING_DEFAULT);
    #if defined __WXMSW__
        wxFont BoldFont(wxDEFAULT, wxDEFAULT, wxFONTSTYLE_NORMAL, wxBOLD, false, wxEmptyString, wxFONTENCODING_DEFAULT);
    #elif defined __WXGTK__
        wxFont BoldFont(9, wxDEFAULT, wxFONTSTYLE_NORMAL, wxBOLD, false, wxEmptyString, wxFONTENCODING_DEFAULT);
    #endif

    wxGenericStaticBitmap *TULogo;
    TULogo = new wxGenericStaticBitmap(this, wxID_ANY, wxBitmap(GetDataPath() + wxT("/TU.png"), wxBITMAP_TYPE_PNG), wxDefaultPosition, wxSize(150, 150));
    MainSizer->Add(TULogo, 1, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);

    wxFlexGridSizer *InfoSizer = new wxFlexGridSizer(0, 1, 0, 0);
    wxStaticText *TUText = new wxStaticText(this, wxID_ANY, wxT("TUMessenger"));
    TUText->SetFont(HeaderFont);
    InfoSizer->Add(TUText, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 3);

    // Version
    wxFlexGridSizer *VersionSizer = new wxFlexGridSizer(0, 2, 0, 0);
    wxStaticText *VersionText = new wxStaticText(this, wxID_ANY, wxT("Version:"));
    #ifndef __WXMAC__
    VersionText->SetFont(BoldFont);
    #endif
    VersionSizer->Add(VersionText, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 3);
    wxStaticText *VersionLabel = new wxStaticText(this, wxID_ANY, VERSIONSTRING);
    VersionSizer->Add(VersionLabel, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 3);
    InfoSizer->Add(VersionSizer, 1, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 0);

    // Lizenz
    wxFlexGridSizer *LicenseSizer = new wxFlexGridSizer(0, 2, 0, 0);
    wxStaticText *LicenseText = new wxStaticText(this, wxID_ANY, wxT("Lizenz:"));
    #ifndef __WXMAC__
    LicenseText->SetFont(BoldFont);
    #endif
    LicenseSizer->Add(LicenseText, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 3);
    wxStaticText *LicenseLabel = new wxStaticText(this, wxID_ANY, wxT("GPL v3"));
    LicenseSizer->Add(LicenseLabel, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 3);
    InfoSizer->Add(LicenseSizer, 1, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 0);

    // Programmierer
    wxFlexGridSizer *ProgSizer = new wxFlexGridSizer(0, 2, 0, 0);
    wxStaticText *ProgText = new wxStaticText(this, wxID_ANY, wxT("Programmierer:"));
    #ifndef __WXMAC__
    ProgText->SetFont(BoldFont);
    #endif
    ProgSizer->Add(ProgText, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 3);
    wxHyperlinkCtrl *AuthorOneLink = new wxHyperlinkCtrl(this, wxID_ANY, wxT("download"), wxT("http://www.team-ulm.de/Profil/190590"));
    ProgSizer->Add(AuthorOneLink, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 3);
    InfoSizer->Add(ProgSizer, 1, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 0);

    // Logo
    wxFlexGridSizer *LogoAuthSizer = new wxFlexGridSizer(0, 2, 0, 0);
    wxStaticText *LogoAuthText = new wxStaticText(this, wxID_ANY, wxT("Logo:"));
    #ifndef __WXMAC__
    LogoAuthText->SetFont(BoldFont);
    #endif
    LogoAuthSizer->Add(LogoAuthText, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 3);
    wxHyperlinkCtrl *LogoLink = new wxHyperlinkCtrl(this, wxID_ANY, wxT("Nightrider94"), wxT("http://www.team-ulm.de/Profil/307145"));
    LogoAuthSizer->Add(LogoLink, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 3);
    InfoSizer->Add(LogoAuthSizer, 1, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 0);

    // wxWidgets Build Info
    wxFlexGridSizer *BuildSizer = new wxFlexGridSizer(0, 2, 0, 0);
    wxStaticText *BuildText = new wxStaticText(this, wxID_ANY, GetBuildInfo());
    BuildSizer->Add(BuildText, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 3);
    InfoSizer->Add(BuildSizer, 1, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 0);

    MainSizer->Add(InfoSizer, 1, wxALL|wxALIGN_LEFT|wxALIGN_TOP, 5);
    SetSizer(MainSizer);
    MainSizer->SetSizeHints(this);
}

// ------------------------------------------------------------------

void TUBuddylist::ClickLargeView(wxCommandEvent &event)
{
    wxFileConfig *file = new wxFileConfig(wxT(""), wxT(""), wxStandardPaths().Get().GetUserDataDir() + wxT("/einstellungen.conf"), wxT(""), wxCONFIG_USE_LOCAL_FILE);
    file->Write(wxT("Ansicht"), wxT("1"));
    delete file;

    Buddylist->Clear();
    wxArrayString Add;

    for ( int CurrBuddy = 0; (size_t)CurrBuddy < VBuddylist.size(); CurrBuddy++ )
    {
        VBuddy ThisBuddy = VBuddylist.at(CurrBuddy);

        if ( ThisBuddy.connection == ONLINE ) Add.Add(wxT("<table><tr><td><img src=\"") + GetDataPath() + wxT("/online.png") + wxT("\"></td><td><font size=\"3\">") + ThisBuddy.username + wxT("</font></td></tr></table>"));
        else Add.Add(wxT("<table><tr><td><img src=\"") + GetDataPath() + wxT("/offline.png") + wxT("\"></td><td><font size=\"3\">") + ThisBuddy.username + wxT("</font></td></tr></table>"));
    }

    Buddylist->SetMargins(-2, -3);
    Buddylist->Append(Add);
}

void TUBuddylist::ClickSmallView(wxCommandEvent &event)
{
    wxFileConfig *file = new wxFileConfig(wxT(""), wxT(""), wxStandardPaths().Get().GetUserDataDir() + wxT("/einstellungen.conf"), wxT(""), wxCONFIG_USE_LOCAL_FILE);
    file->Write(wxT("Ansicht"), wxT("2"));
    delete file;

    Buddylist->Clear();
    wxArrayString Add;

    for ( int CurrBuddy = 0; (size_t)CurrBuddy < VBuddylist.size(); CurrBuddy++ )
    {
        VBuddy ThisBuddy = VBuddylist.at(CurrBuddy);

        if ( ThisBuddy.connection == ONLINE ) Add.Add(wxT("<table><tr><td><img src=\"") + GetDataPath() + wxT("/online_klein.png") + wxT("\"></td><td><font size=\"2\">") + ThisBuddy.username + wxT("</font></td></tr></table>"));
        else Add.Add(wxT("<table><tr><td><img src=\"") + GetDataPath() + wxT("/offline_klein.png") + wxT("\"></td><td><font size=\"2\">") + ThisBuddy.username + wxT("</font></td></tr></table>"));
    }

    Buddylist->SetMargins(-3, -3);
    Buddylist->Append(Add);
}

void TrayIconClass::SoundSwitch(wxCommandEvent &event)
{
    wxFileConfig *file = new wxFileConfig(wxT(""), wxT(""), wxStandardPaths().Get().GetUserDataDir() + wxT("/einstellungen.conf"), wxT(""), wxCONFIG_USE_LOCAL_FILE);
    if ( parent->SoundsEnabled == false )
    {
        parent->SoundsEnabled = true;
        parent->SoundsItem->Check(true);
        file->Write(wxT("Klaenge"), wxT("An"));
    }
    else
    {
        parent->SoundsEnabled = false;
        parent->SoundsItem->Check(false);
        file->Write(wxT("Klaenge"), wxT("Aus"));
    }
    delete file;
}

void TUBuddylist::SoundSwitch(wxCommandEvent &event)
{
    wxFileConfig *file = new wxFileConfig(wxT(""), wxT(""), wxStandardPaths().Get().GetUserDataDir() + wxT("/einstellungen.conf"), wxT(""), wxCONFIG_USE_LOCAL_FILE);
    if ( SoundsEnabled == false )
    {
        SoundsEnabled = true;
        SoundsItem->Check(true);
        file->Write(wxT("Klaenge"), wxT("An"));
    }
    else
    {
        SoundsEnabled = false;
        SoundsItem->Check(false);
        file->Write(wxT("Klaenge"), wxT("Aus"));
    }
    delete file;
}

void TUBuddylist::WebsiteClicked(wxCommandEvent&)
{
    wxLaunchDefaultBrowser(wxT("http://tumessenger.ouned.de/"));
}

void TUBuddylist::UserChEvt(wxCommandEvent &event)
{
    if ( MsgQueue.size() != 0 )
    {
        int retn = wxMessageBox(wxT("Der TUMessenger hat noch nicht alle ausstehenden Nachrichten versendet.\nWenn du den Benutzer jetzt wechselst gehen diese Nachrichten verloren.\n\nMöchtest du dich trotzdem Ausloggen?"), wxT("Noch nicht alle Nachrichten verschickt"), wxYES_NO | wxICON_WARNING | wxCENTRE, this);
        if ( retn == wxNO )
            return;
    }

    wxFileConfig *file = new wxFileConfig(wxT(""), wxT(""), wxStandardPaths().Get().GetUserDataDir() + wxT("/einstellungen.conf"), wxT(""), wxCONFIG_USE_LOCAL_FILE);
    file->Write(wxT("BuddylistePosX"), Position.x);
    file->Write(wxT("BuddylistePosY"), Position.y);
    file->Write(wxT("BuddylisteWidth"), this->GetSize().GetWidth());
    file->Write(wxT("BuddylisteHeight"), this->GetSize().GetHeight());
    if ( ChatWindowOpen == true && !ChatWin->IsIconized() )
    {
        file->Write(wxT("ChatfensterPosX"), ChatWin->GetPosition().x);
        file->Write(wxT("ChatfensterPosY"), ChatWin->GetPosition().y);
        file->Write(wxT("ChatfensterWidth"), ChatWin->GetSize().GetWidth());
        file->Write(wxT("ChatfensterHeight"), ChatWin->GetSize().GetHeight());
    }
    delete file;

    if ( ChatWindowOpen == true ) { ChatWin->Destroy(); }

    MsgSendTimer->Stop(); PopupKillTimer->Stop(); PopupTimer->Stop();

    if ( Worker != NULL && Worker->IsRunning() )
        Worker->Delete();

    if ( PicWorker != NULL && PicWorker->IsRunning() )
        PicWorker->Delete();

    if ( MsgSender != NULL && MsgSender->IsRunning() )
        MsgSender->Delete();

    tu_getpage(tulink, wxT("https://m.team-ulm.de/Logout"));
    curl_share_cleanup(tulink);

    TUStartFrame *frame = new TUStartFrame(wxT("Login"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX);
    frame->Show(true);

    TrayIcon->Destroy();
    this->Destroy();
}

void TUBuddylist::EnableOfflineMode(bool reason)
{
    if ( CurrMode == OFFMODE )
        return;

    #ifdef __WXMSW__
        if ( WinSevenTaskbarEnabled == true && wxPlatformInfo::Get().CheckOSVersion(6, 1) )
        {
            HICON hIcon = (HICON)LoadImage(NULL, (GetDataPath() + wxT("/off_overlay.ico")).c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
            m_taskbarInterface->SetOverlayIcon(this->GetHandle(), hIcon, L"Online");
        }
    #endif

    CurrMode = OFFMODE;
    rWas = reason;

    MsgSendTimer->Stop(); PopupKillTimer->Stop(); PopupTimer->Stop();
    if ( PicWorker != NULL && PicWorker->IsRunning() )
        PicWorker->Delete();
    if ( MsgSender != NULL && MsgSender->IsRunning() )
        MsgSender->Delete();

    Buddylist->Clear();
    VBuddylist.clear();
    ChatItem->Enable(false);
    BIconItem->Enable(false);

    StatusBox->Clear();
    StatusBox->Enable(false);
    StatusButton->Enable(false);

    #ifdef __WXGTK__
        this->SetIcon(wxIcon(GetDataPath() + wxT("/TUKleinOff.png"), wxBITMAP_TYPE_PNG));
        TrayIcon->SetIcon(wxIcon(GetDataPath() + wxT("/TUKleinOff.png"), wxBITMAP_TYPE_PNG), username + wxT(" - Offline"));
    #elif defined __WXMSW__
        this->SetIcons(wxIconBundle(GetDataPath() + wxT("/TUTaskOff.ico"), wxBITMAP_TYPE_ICO));
        TrayIcon->SetIcon(wxIcon(GetDataPath() + wxT("/TUKleinOff.ico"), wxBITMAP_TYPE_ICO), username + wxT(" - Offline"));
    #elif defined __WXMAC__
        this->SetIcon(wxIcon(GetDataPath() + wxT("/TUKleinOff.png"), wxBITMAP_TYPE_PNG));
    #endif

    if ( ChatWindowOpen == true )
    {
        #if defined __WXGTK__ || defined __WXMAC__
            ChatWin->SetIcon(wxIcon(GetDataPath() + wxT("/TUKleinOff.png"), wxBITMAP_TYPE_PNG));
        #elif defined __WXMSW__
            ChatWin->SetIcons(wxIconBundle(GetDataPath() + wxT("/TUTaskOff.ico"), wxBITMAP_TYPE_ICO));
        #endif

        #ifdef __WXMSW__
            if ( WinSevenTaskbarEnabled == true && wxPlatformInfo::Get().CheckOSVersion(6, 1) )
            {
                m_taskbarInterface->SetProgressState(ChatWin->GetHandle(), TBPF_NOPROGRESS);
            }
        #endif
    }

    if ( reason == SUSPENDED )
    {
        if ( Worker != NULL && Worker->IsRunning() )
            Worker->Delete();

        tu_getpage(tulink, wxT("https://m.team-ulm.de/Logout"));
    }

    StatBar->SetStatusText(wxT("Offline"), 1);
    Buddylist->Append(wxT("<table><tr><td><img src=\"") + GetDataPath() + wxT("/OffModus.png") + wxT("\"></td><td><b>Offlinemodus</b></td></tr></table>"));
    Buddylist->Append(wxT("<p><font size=\"2\">Die Verbindung zu m.team-ulm.de konnte nicht hergestellt werden oder wurde unterbrochen.<br>Du wirst automatisch wieder eingeloggt sobald die Verbindung wieder hergestellt werden kann.</font></p>"));
}

void TUBuddylist::DisableOfflineMode()
{
    if ( CurrMode == NORMAL )
        return;

    #ifdef __WXMSW__
        if ( WinSevenTaskbarEnabled == true && wxPlatformInfo::Get().CheckOSVersion(6, 1) )
        {
            HICON hIcon = (HICON)LoadImage(NULL, (GetDataPath() + wxT("/on_overlay.ico")).c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
            m_taskbarInterface->SetOverlayIcon(this->GetHandle(), hIcon, L"Online");
        }
    #endif

    CurrMode = NORMAL;
    ChatItem->Enable(true);
    BIconItem->Enable(true);

    StatusBox->Enable(true);

    #ifdef __WXGTK__
        this->SetIcon(wxIcon(GetDataPath() + wxT("/TUKlein.png"), wxBITMAP_TYPE_PNG));
        TrayIcon->SetIcon(wxIcon(GetDataPath() + wxT("/TUKlein.png"), wxBITMAP_TYPE_PNG), username + wxT(" - Online"));
    #elif defined __WXMSW__
        this->SetIcons(wxIconBundle(GetDataPath() + wxT("/TUTask.ico"), wxBITMAP_TYPE_ICO));
        TrayIcon->SetIcon(wxIcon(GetDataPath() + wxT("/TUKlein.ico"), wxBITMAP_TYPE_ICO), username + wxT(" - Online"));
    #elif defined __WXMAC__
        this->SetIcon(wxIcon(GetDataPath() + wxT("/TUKlein.png"), wxBITMAP_TYPE_PNG));
    #endif

    if ( ChatWindowOpen == true )
    {
        #if defined __WXGTK__ || defined __WXMAC__
            ChatWin->SetIcon(wxIcon(GetDataPath() + wxT("/TUKlein.png"), wxBITMAP_TYPE_PNG));
        #elif defined __WXMSW__
            ChatWin->SetIcons(wxIconBundle(GetDataPath() + wxT("/TUTask.ico"), wxBITMAP_TYPE_ICO));
        #endif
    }

    MsgSendTimer->Start(1, true);
    Buddylist->Clear();
}

#ifdef __WXMSW__
void TUBuddylist::Suspending(wxPowerEvent &event)
{
    EnableOfflineMode(SUSPENDED);
}

void TUBuddylist::Resuming(wxPowerEvent &event)
{
    Worker = new TUWorkerThread();
    Worker->GetBuddysInstantly = false;
    Worker->username = this->username;
    Worker->FromSuspended = 3;
    wxFileConfig *file = new wxFileConfig(wxT(""), wxT(""), wxStandardPaths().Get().GetUserDataDir() + wxT("/einstellungen.conf"), wxT(""), wxCONFIG_USE_LOCAL_FILE);
    Worker->password = DecryptString(file->Read(Worker->username + wxT("Passwort")));
    delete file;
    Worker->Create();
    Worker->parent = this;
    Worker->Run();
}
#endif

#ifdef __WXMSW__
WXLRESULT TUBuddylist::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    if ( nMsg == RegisterWindowMessage(wxT("TUMessengerFocusBList") + wxGetUserId()) )
    {
        Show();

        if ( IsIconized() )
           Iconize(false);

        Raise();
    }

    return wxFrame::MSWWindowProc(nMsg, wParam, lParam);
}
#endif

void TUBuddylist::UsrStatusChanged(wxCommandEvent &event)
{
    if ( !StatusButton->IsEnabled() )
        StatusButton->Enable(true);
}

void TUBuddylist::SetStatus()
{
    if ( !StatusButton->IsEnabled() )
        return;

    if ( tu_setstatus(tulink, StatusBox->GetValue()) )
        StatusButton->Enable(false);
    else
        wxMessageBox(wxT("Status konnte nicht gesetzt werden."), wxT("Error: Status"), wxICON_ERROR | wxOK);
}
