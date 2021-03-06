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
#include <wx/clipbrd.h>
#include <wx/dir.h>
#include <wx/sound.h>
#include <wx/fileconf.h>
#include <wx/generic/statbmpg.h>
#include <wx/stdpaths.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/artprov.h>
#include <wx/aui/auibook.h>
#include "TUBuddylist.h"
#include "TUChatwindow.h"
#include "TUConnection.h"
#include "TUExFuncs.h"

const long TABS_CONTROL = wxNewId();
const long CHAT_ENTER = wxNewId();
const long CHAT_TEXT = wxNewId();

DECLARE_EVENT_TYPE(EVENT_FOCUS, -3)
DEFINE_EVENT_TYPE(EVENT_FOCUS)

DECLARE_EVENT_TYPE(EVENT_FUCKIT, -73)
DEFINE_EVENT_TYPE(EVENT_FUCKIT)

DECLARE_EVENT_TYPE(EVENT_THREADREPORT, -2)

BEGIN_EVENT_TABLE(TUChatwindow, wxFrame)
    EVT_AUINOTEBOOK_PAGE_CLOSE(TABS_CONTROL, TUChatwindow::OnCloseChat)
    EVT_AUINOTEBOOK_PAGE_CHANGED(TABS_CONTROL, TUChatwindow::OnChangedTab)
    EVT_COMMAND(wxID_ANY, EVENT_FOCUS, TUChatwindow::SetFocusEvent)
END_EVENT_TABLE()

TUChatwindow::TUChatwindow(wxWindow *par, bool FromNewMsg)
{
    wxFileConfig *file = new wxFileConfig(wxT(""), wxT(""), wxStandardPaths().Get().GetUserDataDir() + wxT("/einstellungen.conf"), wxT(""), wxCONFIG_USE_LOCAL_FILE);

    frNewMsg = FromNewMsg;

    if ( file->Read(wxT("ChatfensterPosX")) != wxEmptyString )
    {
        Create(par, wxID_ANY, wxT("Chatfenster"), wxPoint(wxAtoi(file->Read(wxT("ChatfensterPosX"))), wxAtoi(file->Read(wxT("ChatfensterPosY")))), wxSize(wxAtoi(file->Read(wxT("ChatfensterWidth"))), wxAtoi(file->Read(wxT("ChatfensterHeight")))));
    }
    else
    {
        Create(par, wxID_ANY, wxT("Chatfenster"), wxDefaultPosition, wxSize(550, 350));
    }
    delete file;

    this->SetMinSize(wxSize(240, 150));
    #if defined __WXGTK__ || defined __WXMAC__
        this->SetIcon(wxIcon(GetDataPath() + wxT("/TUKlein.png"), wxBITMAP_TYPE_PNG));
    #elif defined __WXMSW__
        this->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_MENUBAR));
        this->SetIcons(wxIconBundle(GetDataPath() + wxT("/TUTask.ico"), wxBITMAP_TYPE_ICO));
    #endif

    Chats = new wxAuiNotebook(this, TABS_CONTROL, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TAB_FIXED_WIDTH|wxAUI_NB_CLOSE_ON_ALL_TABS|wxAUI_NB_TAB_MOVE|wxAUI_NB_WINDOWLIST_BUTTON|wxBORDER_NONE);

    Connect(wxID_ANY, wxEVT_CLOSE_WINDOW, (wxObjectEventFunction)&TUChatwindow::OnClose);
    Connect(TABS_CONTROL, wxEVT_COMMAND_AUINOTEBOOK_TAB_MIDDLE_UP, (wxObjectEventFunction)&TUChatwindow::OnMiddleClick);
}

bool TUChatwindow::Show(bool show)
{
    ShowWindow(this->GetHWND(), SW_SHOWNA);

    return true;
}

void TUChatwindow::EndChatWindow()
{
    wxFileConfig *file = new wxFileConfig(wxT(""), wxT(""), wxStandardPaths().Get().GetUserDataDir() + wxT("/einstellungen.conf"), wxT(""), wxCONFIG_USE_LOCAL_FILE);
    file->Write(wxT("ChatfensterPosX"), this->GetPosition().x);
    file->Write(wxT("ChatfensterPosY"), this->GetPosition().y);
    file->Write(wxT("ChatfensterWidth"), this->GetSize().GetWidth());
    file->Write(wxT("ChatfensterHeight"), this->GetSize().GetHeight());
    delete file;

    parent->ChatWindowOpen = false;
    this->Destroy();
}

void TUChatwindow::OnMiddleClick(wxAuiNotebookEvent &event)
{
    if ( Chats->GetPageCount() == 1 )
    {
        EndChatWindow();
    }
    else
    {
        Chats->DeletePage(event.GetInt());
    }
}

void TUChatwindow::OnCloseChat(wxAuiNotebookEvent &event)
{
    if ( Chats->GetPageCount() == 1 )
    {
        EndChatWindow();
    }
}

void TUChatwindow::OnChangedTab(wxAuiNotebookEvent &event)
{
    this->SetTitle(Chats->GetPageText(Chats->GetSelection()));

    Chats->SetPageBitmap(Chats->GetSelection(), wxNullBitmap);
    BuddyChatWin *ThisChatWin = (BuddyChatWin*)Chats->GetPage(Chats->GetSelection());
    if ( ThisChatWin->NewMessage == true ) { ThisChatWin->NewMessage = false; }

    wxCommandEvent evt(EVENT_FOCUS, wxID_ANY);
    this->AddPendingEvent(evt);
}

void TUChatwindow::SetFocusEvent(wxCommandEvent &event)
{
    BuddyChatWin *ThisChatWin = (BuddyChatWin*)Chats->GetPage(Chats->GetSelection());
    ThisChatWin->EnterText->SetFocus();
}

void TUChatwindow::OnClose(wxCloseEvent& event)
{
    EndChatWindow();
}

/***************************************************************************/
// TabWindows
/***************************************************************************/

BEGIN_EVENT_TABLE(BuddyChatWin, wxWindow)
    EVT_TEXT_ENTER(CHAT_ENTER, BuddyChatWin::SendMessage)
    EVT_COMMAND(wxID_ANY, EVENT_FUCKIT, BuddyChatWin::AntiCrashKeyPress)
    EVT_HTML_LINK_CLICKED(CHAT_TEXT, BuddyChatWin::OnLinkClicked)
END_EVENT_TABLE()

BuddyChatWin::BuddyChatWin(wxWindow *par) { Create(par, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE); }

void BuddyChatWin::InitGui(wxString username, wxString UserPicPath, wxString status, bool connection)
{
    MsgID = 0;

    wxFlexGridSizer *MainSizer = new wxFlexGridSizer(0, 1, 0, 0);
    wxFlexGridSizer *BuddyInfSizer = new wxFlexGridSizer(0, 3, 0, 0);
    wxImage *UserPicImage = new wxImage(UserPicPath);
    wxImage ScaledUserPic = UserPicImage->Scale(40, 40, wxIMAGE_QUALITY_HIGH);
    UserPicImage->Destroy();
    UserPic = new wxGenericStaticBitmap(this, wxID_ANY, wxBitmap(ScaledUserPic, wxBITMAP_SCREEN_DEPTH), wxDefaultPosition, wxSize(40, 40), wxBORDER_SIMPLE);
    BuddyInfSizer->Add(UserPic, 1, wxLEFT|wxTOP|wxRIGHT|wxALIGN_LEFT|wxALIGN_TOP, 2);
    wxFlexGridSizer *UserStatusSizer = new wxFlexGridSizer(0, 1, 0, 0);
    wxStaticText *Userlabel = new wxStaticText(this, wxID_ANY, username, wxDefaultPosition, wxDefaultSize);
    wxFont UserlabelFont(13,wxDEFAULT,wxFONTSTYLE_NORMAL,wxBOLD,false,wxEmptyString,wxFONTENCODING_DEFAULT);
    Userlabel->SetFont(UserlabelFont);
    UserStatusSizer->Add(Userlabel, 1, wxTOP|wxLEFT|wxRIGHT|wxALIGN_LEFT|wxALIGN_TOP|wxEXPAND, 5);
    Userstatus = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
    Userstatus->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    UserStatusSizer->Add(Userstatus, 1, wxEXPAND|wxLEFT|wxRIGHT|wxALIGN_LEFT|wxALIGN_TOP, 5);
    Userstatus->SetLabel(status);
    BuddyInfSizer->Add(UserStatusSizer, 1, wxLEFT|wxEXPAND|wxALIGN_LEFT|wxALIGN_TOP, 2);

    if ( status == wxT("ACHTUNG: Dieser User befindet sich nicht in deiner Buddyliste! Keine Informationen verfügbar.") ){}
    else if ( connection == ONLINE )
    {
        StatusImage = new wxGenericStaticBitmap(this, wxID_ANY, wxBitmap(GetDataPath() + wxT("/online.png"), wxBITMAP_TYPE_PNG), wxDefaultPosition, wxSize(20,20));
        BuddyInfSizer->Add(StatusImage, 1, wxLEFT|wxTOP|wxRIGHT|wxALIGN_RIGHT|wxALIGN_CENTRE_VERTICAL, 5);
    }
    else
    {
        StatusImage = new wxGenericStaticBitmap(this, wxID_ANY, wxBitmap(GetDataPath() + wxT("/offline.png"), wxBITMAP_TYPE_PNG), wxDefaultPosition, wxSize(20,20));
        BuddyInfSizer->Add(StatusImage, 1, wxLEFT|wxTOP|wxRIGHT|wxALIGN_RIGHT|wxALIGN_CENTRE_VERTICAL, 5);
    }

    MainSizer->Add(BuddyInfSizer, 1, wxALL|wxEXPAND|wxALIGN_LEFT|wxALIGN_TOP, 2);
    #ifdef __WXMSW__
        if ( parent->parent->backend == BROWSERBACKEND )
        {
            if ( !wxDir::Exists(wxStandardPaths().Get().GetUserDataDir() + wxT("/tmp")) )
            {
                wxFileName::Mkdir(wxStandardPaths().Get().GetUserDataDir() + wxT("/tmp"));
            }
            InnerHTM = HTMLTEMPLATE;
            wxFile *file = new wxFile(wxStandardPaths().Get().GetUserDataDir() + wxT("/tmp/") + BuddyID + wxT(".htm"), wxFile::write);
            file->Write(InnerHTM);
            file->Close();
            ChatTextNew = wxWebView::GetNew(this, CHAT_TEXT, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxWEB_VIEW_BACKEND_DEFAULT, wxBORDER_THEME);
            ChatTextNew->LoadUrl(wxStandardPaths().Get().GetUserDataDir() + wxT("/tmp/") + BuddyID + wxT(".htm"));
            MainSizer->Add(ChatTextNew, 1, wxALL|wxEXPAND|wxALIGN_LEFT|wxALIGN_TOP, 1);
            ChatTextNew->Disable();
        }
        else
        {
            ChatText = new wxHtmlWindow(this, CHAT_TEXT, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO | wxBORDER_THEME);
            ChatText->Connect(ChatText->GetId(), wxEVT_KEY_DOWN, (wxObjectEventFunction)&BuddyChatWin::OnPressKeyChatText);
            ChatText->SetBorders(2);
            ChatText->SetCaret(NULL);
            MainSizer->Add(ChatText, 1, wxALL|wxEXPAND|wxALIGN_LEFT|wxALIGN_TOP, 1);
        }
    #else
        ChatText = new wxHtmlWindow(this, CHAT_TEXT, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO | wxBORDER_THEME);
        ChatText->Connect(ChatText->GetId(), wxEVT_KEY_DOWN, (wxObjectEventFunction)&BuddyChatWin::OnPressKeyChatText);
        ChatText->SetBorders(2);
        ChatText->SetCaret(NULL);
        parent->parent->backend = WXHTMLBACKEND;
        MainSizer->Add(ChatText, 1, wxALL|wxEXPAND|wxALIGN_LEFT|wxALIGN_TOP, 1);
    #endif
    ChatTextEmpty = true;
    NewMessage = false;
    EnterText = new wxTextCtrl(this, CHAT_ENTER, wxEmptyString, wxDefaultPosition, wxSize(-1, 50), wxTE_MULTILINE | wxTE_RICH | wxTE_PROCESS_ENTER | wxBORDER_THEME);
    EnterText->Connect(CHAT_ENTER, wxEVT_KEY_DOWN, (wxObjectEventFunction)&BuddyChatWin::OnKeyDown);
    MainSizer->Add(EnterText, 1, wxALL|wxEXPAND|wxALIGN_LEFT|wxALIGN_BOTTOM, 1);
    MainSizer->AddGrowableCol(0, wxHORIZONTAL);
    MainSizer->AddGrowableRow(1, wxBOTH);
    BuddyInfSizer->AddGrowableCol(1, wxHORIZONTAL);
    UserStatusSizer->AddGrowableCol(0, wxHORIZONTAL);

    SetSizer(MainSizer);
    MainSizer->SetSizeHints(this);
}

void BuddyChatWin::SendMessage(wxCommandEvent &event)
{
    if ( wxKeyboardState().AltDown() == true ) return;

    wxString Message = EnterText->GetValue();
    if ( Message == wxT("\n") || Message == wxEmptyString ) { EnterText->Clear(); return; }

    if ( parent->parent->MsgQueue.size() >= 10 && parent->parent->ChuckNorrisMode == false )
    {
        wxMessageBox(wxT("Die Nachrichtenwarteschleife ist voll!\nBitte ein paar Sekunden warten..."), wxT("Warnung"), wxICON_WARNING | wxOK);
        return;
    }

    #ifdef __WXMSW__
        Message = Message.Remove(EnterText->GetInsertionPoint() - 1, 1);
    #endif

    wxString MessageInChatWin = Message;
    EnterText->Clear();

    if ( MessageInChatWin == wxT("/ChuckNorrisModus An") ) { parent->parent->ChuckNorrisMode = true; return; }
    if ( MessageInChatWin == wxT("/ChuckNorrisModus Aus") ) { parent->parent->ChuckNorrisMode = false; return; }

    MessageInChatWin.Replace(wxT("<"), wxT("&lt;"), true); MessageInChatWin.Replace(wxT(">"), wxT("&gt;"), true); // Anti HTML

    // Links
    int LastSearchPos = 0;
    while ( MessageInChatWin.find(wxT("://"), LastSearchPos) != (size_t)wxNOT_FOUND )
    {
        int LinkStartPos, LinkEndPos;
        wxString TempCut;
        wxString Link;

        LinkStartPos = MessageInChatWin.find(wxT("://"), LastSearchPos);
        if ( MessageInChatWin.Mid(LinkStartPos - 4, 7) != wxT("http://") )
        {
            if ( MessageInChatWin.Mid(LinkStartPos - 3, 6) != wxT("ftp://") )
            {
                if ( MessageInChatWin.Mid(LinkStartPos - 5, 8) != wxT("https://") )
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
            wxString tmp = MessageInChatWin;
            TempCut = tmp.Remove(0, LinkStartPos);
            if ( MessageInChatWin.GetChar(LinkStartPos - 1) != wxChar('\n') && MessageInChatWin.GetChar(LinkStartPos - 1) != wxChar(' ') ) { LastSearchPos = LinkStartPos + 6; continue; }
        }
        else TempCut = MessageInChatWin;
        int LinkEndPosOne = TempCut.Find(wxT(" "));
        int LinkEndPosTwo = TempCut.Find(wxT("\n"));

        if ( ( LinkEndPosOne < LinkEndPosTwo && LinkEndPosOne != wxNOT_FOUND ) || ( LinkEndPosTwo == wxNOT_FOUND && LinkEndPosOne != wxNOT_FOUND ) ) LinkEndPos = LinkEndPosOne;
        else if ( ( LinkEndPosTwo < LinkEndPosOne && LinkEndPosTwo != wxNOT_FOUND ) || ( LinkEndPosOne == wxNOT_FOUND && LinkEndPosTwo != wxNOT_FOUND ) ) LinkEndPos = LinkEndPosTwo;
        else LinkEndPos = MessageInChatWin.Length() - 1;

        if ( LinkEndPos != (int)MessageInChatWin.Length() - 1 ) Link = TempCut.Remove(LinkEndPos);
        else Link = TempCut;
        LastSearchPos = LinkEndPos;

        MessageInChatWin.Replace(Link, wxT("<a href=\"") + Link + wxT("\">") + Link + wxT("</a>"));
    }

    MessageInChatWin.Replace(wxT("\n"), wxT("<br>"), true);

    // Smilies
    wxString Path = GetDataPath() + wxT("/smilies/");
    for ( int SmilieNum = 0; (size_t)SmilieNum < parent->parent->Smilies.GetCount(); SmilieNum++ )
    {
        if ( parent->parent->Smilies[SmilieNum] == wxT("grins") )
        {
            MessageInChatWin.Replace(wxT(":-)"), wxT("<img src=\"") + Path + parent->parent->Smilies[SmilieNum] + wxT(".gif\">"), true);
            continue;
        }
        if ( parent->parent->Smilies[SmilieNum] == wxT("lol") )
        {
            MessageInChatWin.Replace(wxT(":-D"), wxT("<img src=\"") + Path + parent->parent->Smilies[SmilieNum] + wxT(".gif\">"), true);
            continue;
        }
        if ( parent->parent->Smilies[SmilieNum] == wxT("zwinker") )
        {
            MessageInChatWin.Replace(wxT(";-)"), wxT("<img src=\"") + Path + parent->parent->Smilies[SmilieNum] + wxT(".gif\">"), true);
            continue;
        }
        if ( parent->parent->Smilies[SmilieNum] == wxT("heuli") )
        {
            MessageInChatWin.Replace(wxT(":-("), wxT("<img src=\"") + Path + parent->parent->Smilies[SmilieNum] + wxT(".gif\">"), true);
            continue;
        }
        if ( parent->parent->Smilies[SmilieNum] == wxT("zungeraus") )
        {
            MessageInChatWin.Replace(wxT(":-P"), wxT("<img src=\"") + Path + parent->parent->Smilies[SmilieNum] + wxT(".gif\">"), true);
            continue;
        }
        if ( parent->parent->Smilies[SmilieNum] == wxT("brille") )
        {
            MessageInChatWin.Replace(wxT("8-)"), wxT("<img src=\"") + Path + parent->parent->Smilies[SmilieNum] + wxT(".gif\">"), true);
            continue;
        }

        MessageInChatWin.Replace(wxT(":") + parent->parent->Smilies[SmilieNum] + wxT(":"), wxT("<img src=\"") + Path + parent->parent->Smilies[SmilieNum] + wxT(".gif\">"), true);
    }

    MessageInChatWin = wxT("<font color=\"#08088A\" id=") + wxString::Format(wxT("%i"), MsgID) + wxT(" size=\"2\">(") + wxDateTime().Now().FormatISOTime() + wxT(") ") + parent->UserName + wxT(": </font><font color=\"#000000\" size=\"2\">") + MessageInChatWin + wxT("</font>");
    AddMsg(MessageInChatWin);

    wxArrayString MsgForQueue;
    MsgForQueue.Add(parent->Chats->GetPageText(parent->Chats->GetSelection())); MsgForQueue.Add(Message); MsgForQueue.Add(MessageInChatWin);
    parent->parent->MsgQueue.push_back(MsgForQueue);
    if ( parent->parent->SoundsEnabled == true ) wxSound(GetDataPath() + wxT("/NachrichtGesendet.wav")).Play();

    if ( !parent->parent->MsgSendTimer->IsRunning() && !parent->parent->MsgSender && parent->parent->CurrMode == NORMAL )
    {
        if ( parent->parent->lastMsgSent > 0 )
        {
            int timeDiff = wxDateTime().GetTicks() - parent->parent->lastMsgSent;
            parent->parent->MsgSendTimer->Start((8 - timeDiff) * 1000, true);
        }
        else
        {
            parent->parent->MsgSendTimer->Start(1, true);
        }
    }

    MsgID++;

    #ifdef __WXMSW__
    if ( parent->parent->CurrMode == NORMAL )
    {
        if ( parent->parent->WinSevenTaskbarEnabled == true && wxPlatformInfo::Get().CheckOSVersion(6, 1) )
        {
            parent->parent->m_taskbarInterface->SetProgressState(parent->GetHandle(), TBPF_INDETERMINATE);
        }
    }
    #endif
}

void *TUMsgSender::Entry()
{
    for ( int i = 0; i < 5; i++ ) // 5 Versuche
    {
        wxString result = tu_sendmsg(Buddylist->tulink, Msg[0], Msg[1]);

        if ( result.Contains("<p>Die Nachricht wurde erfolgreich gesendet.</p>") )
        {
            wxCommandEvent *evt = new wxCommandEvent(EVENT_THREADREPORT, wxID_ANY);
            evt->SetInt(MSGSEND_REPORT);
            evt->SetString(wxT("SUCCESS"));
            wxQueueEvent(Buddylist, evt);

            return 0;
        }
        else if ( result.Contains("nger wurde nicht gefunden.</div>") )
        {
            wxCommandEvent *evt = new wxCommandEvent(EVENT_THREADREPORT, wxID_ANY);
            evt->SetInt(MSGSEND_REPORT);
            evt->SetString(wxT("UNKNOWNUSER"));
            wxQueueEvent(Buddylist, evt);

            return 0;
        }

        if ( SleepF(2) == 1 ) return 0;
    }

    wxCommandEvent *evt = new wxCommandEvent(EVENT_THREADREPORT, wxID_ANY);
    evt->SetInt(MSGSEND_REPORT);
    evt->SetString(wxT("MSGFAILED"));
    wxQueueEvent(Buddylist, evt);

    return 0;
}

int TUMsgSender::SleepF(int secs)
{
    for ( int tts = secs * 1000 / 50; tts > 0; tts-- )
    {
        if ( TestDestroy() ) return 1;
        wxMilliSleep(50);
    }

    return 0;
}

void BuddyChatWin::OnKeyDown(wxKeyEvent &event)
{
    if ( event.GetKeyCode() == WXK_RETURN && ( event.AltDown() == true || event.ShiftDown() == true || event.ControlDown() == true ) )
    {
        wxCommandEvent evt(EVENT_FUCKIT, wxID_ANY);
        evt.SetInt(1);
        this->AddPendingEvent(evt);
    }
    else if ( event.ControlDown() == true && event.GetKeyCode() == WXK_TAB )
    {
        wxCommandEvent evt(EVENT_FUCKIT, wxID_ANY);
        evt.SetInt(4);
        this->AddPendingEvent(evt);
        return;
    }
    else { event.Skip(); }
}

void BuddyChatWin::OnPressKeyChatText(wxKeyEvent &event)
{
    if ( event.ControlDown() == true && event.GetUnicodeKey() == wxChar('C') )
    {
        wxCommandEvent evt(EVENT_FUCKIT, wxID_ANY);
        evt.SetInt(3);
        this->AddPendingEvent(evt);
        return;
    }
    else if ( event.ControlDown() == true && event.GetKeyCode() == WXK_TAB )
    {
        wxCommandEvent evt(EVENT_FUCKIT, wxID_ANY);
        evt.SetInt(4);
        this->AddPendingEvent(evt);
        return;
    }
    else { event.Skip(); }
}

void BuddyChatWin::AntiCrashKeyPress(wxCommandEvent &event)
{
    if ( event.GetInt() == 1 )
    {
        EnterText->WriteText(wxT("\n"));
    }
    else if ( event.GetInt() == 3 )
    {
        if ( wxTheClipboard->Open() )
        {
            wxTheClipboard->SetData(new wxTextDataObject(ChatText->SelectionToText()));
            wxTheClipboard->Close();
        }
    }
    else if ( event.GetInt() == 4 )
    {
        int PageToSelect = 0;
        if ( parent->Chats->GetPageCount() == 1 ) return;

        if ( (size_t)parent->Chats->GetPageIndex(this) == parent->Chats->GetPageCount() - 1 ) PageToSelect = 0;
        else PageToSelect = parent->Chats->GetPageIndex(this) + 1;

        parent->Chats->SetSelection(PageToSelect);
    }
}

void BuddyChatWin::OnLinkClicked(wxHtmlLinkEvent &event)
{
    wxLaunchDefaultBrowser(event.GetLinkInfo().GetHref());
}

void BuddyChatWin::AddMsg(wxString str)
{
    #ifdef __WXMSW__
        if ( parent->parent->backend == BROWSERBACKEND )
        {
            int endPos = InnerHTM.rfind(wxT("</div>"));
            if ( ChatTextEmpty == true )
            {
                ChatTextEmpty = false;
                InnerHTM = InnerHTM.insert(endPos, str);
            }
            else
                InnerHTM = InnerHTM.insert(endPos, wxT("<br />") + str);

            wxFile *file = new wxFile(wxStandardPaths().Get().GetUserDataDir() + wxT("/tmp/") + BuddyID + wxT(".htm"), wxFile::write);
            file->Write(InnerHTM);
            file->Close();
            ChatTextNew->Reload();
        }
        else
        {
            if ( ChatTextEmpty == true )
            {
                InnerHTM += wxT("<p>") + str;
                ChatText->SetPage(InnerHTM);
                ChatTextEmpty = false;
            }
            else
            {
                InnerHTM += wxT("<br>") + str;
                ChatText->SetPage(InnerHTM);
            }
        }
    #else
        if ( ChatTextEmpty == true )
        {
            InnerHTM += wxT("<p>") + str;
            ChatText->SetPage(InnerHTM);
            ChatTextEmpty = false;
        }
        else
        {
            InnerHTM += wxT("<br>") + str;
            ChatText->SetPage(InnerHTM);
        }
    #endif
}

void BuddyChatWin::MarkMessage(wxString oldstr, wxString newstr)
{
    InnerHTM.Replace(oldstr, newstr, false);

    #ifdef __WXMSW__
        if ( parent->parent->backend == BROWSERBACKEND )
        {
            wxFile *file = new wxFile(wxStandardPaths().Get().GetUserDataDir() + wxT("/tmp/") + BuddyID + wxT(".htm"), wxFile::write);
            file->Write(InnerHTM);
            file->Close();
            ChatTextNew->Reload();
        }
        else
        {
            ChatText->SetPage(InnerHTM);
        }
    #else
        ChatText->SetPage(InnerHTM);
    #endif
}
