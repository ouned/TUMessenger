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
#include <wx/stdpaths.h>
#include <wx/artprov.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/fileconf.h>
#include <curl/curl.h>
#include <wx/tokenzr.h>
#include "TUStartFrame.h"
#include "TUBuddylist.h"
#include "TUConnection.h"
#include "TUExFuncs.h"
#include "TUChatWindow.h"

IMPLEMENT_APP(TUApp)

#ifdef __WXGTK__
const long BUTTON_SHOWPASS = wxNewId();
#endif
const long BUTTON_END = wxNewId();
const long BUTTON_NEXT = wxNewId();
const long USERBOX = wxNewId();
const long PASSWORDBOX = wxNewId();
const long SAVEPASSBOX = wxNewId();
const long AUTOLOGINBOX = wxNewId();

DECLARE_EVENT_TYPE(EVENT_WORKERREPORT, -1)
DEFINE_EVENT_TYPE(EVENT_WORKERREPORT)

UserInfo *usrinfo;
CURLSH *tuconnection = curl_share_init();
TUBuddylist *BuddyFrame;

bool TUApp::OnInit()
{
    wxInitAllImageHandlers();
    wxDisableAsserts();
    Connect(wxID_ANY, wxEVT_END_SESSION, (wxObjectEventFunction)&TUApp::EndSess);

    wxLogNull noLogging;
    const wxString InstanceName = wxString::Format("TUMessenger-%s", wxGetUserId());
    instance = new wxSingleInstanceChecker(InstanceName);
    if ( instance->IsAnotherRunning() )
    {
        #ifdef __WXMSW__
            SendMessageW(HWND_BROADCAST, RegisterWindowMessageW(wxT("TUMessengerFocusBList") + wxGetUserId()), NULL, NULL);
        #else
            wxMessageBox(wxT("Der TUMessenger läuft bereits."), wxT("TUMessenger läuft bereits"), wxICON_ERROR);
        #endif
        return false;
    }
    noLogging.~wxLogNull();

    wxString FilesDir = GetDataPath();
    if ( !wxDir::Exists(FilesDir) )
    {
        #ifdef __WXMSW__
            wxMessageBox(wxT("Zum Ausführen des TUMessengers wird der Ordner \"Daten\" mit dem gesamten Inhalt benötigt.\nDieser sollte sich im Verzeichnis der ausführbaren Datei befinden."), wxT("Error: Voraussetzungen nicht erfüllt"), wxICON_ERROR | wxOK);
        #elif defined __WXGTK__
            wxMessageBox(wxT("Der Ordner\n") + FilesDir + wxT("\nkonnte nicht gefunden werden.\n\nBitte versuche den TUMessenger neu zu installieren."), wxT("Error: Voraussetzungen nicht erfüllt"), wxICON_ERROR | wxOK);
        #endif
        return false;
    }

    wxFileConfig *file = new wxFileConfig(wxT(""), wxT(""), wxStandardPaths().Get().GetUserDataDir() + wxT("/einstellungen.conf"), wxT(""), wxCONFIG_USE_LOCAL_FILE);
    if ( file->Read(wxT("Autologin")) == wxEmptyString )
    {
        delete file;
        TUStartFrame *frame = new TUStartFrame(wxT("Login"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX);
        frame->Show(true);
        SetTopWindow(frame);
    }
    else
    {
        file->Write(wxT("AlsLetztes"), file->Read(wxT("Autologin")));
        delete file;
        BuddyFrame = new TUBuddylist(wxT("Buddyliste"), wxDefaultPosition, wxSize(290, 380), wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxRESIZE_BORDER);
        BuddyFrame->FromStartWindow = false;
        BuddyFrame->startHidden = false;
        if ( this->argc >= 2 )
        {
            if ( argv[1] == wxT("--hidden") )
            {
                BuddyFrame->startHidden = true;
            }
        }
        BuddyFrame->OnStart();
    }

    return true;
}

int TUApp::FilterEvent(wxEvent &event)
{
    if ( event.GetEventType() == wxEVT_CHAR && BuddyFrame && BuddyFrame->ChatWindowOpen == true && BuddyFrame->ChatWin->IsActive() == true )
    {
        BuddyChatWin *Chat = (BuddyChatWin*)BuddyFrame->ChatWin->Chats->GetPage(BuddyFrame->ChatWin->Chats->GetSelection());

        if ( Chat->EnterText->HasFocus() == false )
        {
            wxString chart;
            chart.Append(((wxKeyEvent&)event).GetUnicodeKey());
            Chat->EnterText->WriteText(chart);
            Chat->EnterText->SetFocus();

            return true;
        }
    }

    return -1;
}

void TUApp::EndSess(wxCloseEvent &event)
{
    if ( BuddyFrame && !BuddyFrame->IsIconized() && ( BuddyFrame->Position.x < 10000 && BuddyFrame->Position.x > -10000 ) )
    {
        wxFileConfig *file = new wxFileConfig(wxT(""), wxT(""), wxStandardPaths().Get().GetUserDataDir() + wxT("/einstellungen.conf"), wxT(""), wxCONFIG_USE_LOCAL_FILE);
        file->Write(wxT("BuddylistePosX"), BuddyFrame->Position.x);
        file->Write(wxT("BuddylistePosY"), BuddyFrame->Position.y);
        file->Write(wxT("BuddylisteWidth"), BuddyFrame->GetSize().GetWidth());
        file->Write(wxT("BuddylisteHeight"), BuddyFrame->GetSize().GetHeight());
        if ( BuddyFrame->ChatWindowOpen == true && !BuddyFrame->ChatWin->IsIconized() )
        {
            file->Write(wxT("ChatfensterPosX"), BuddyFrame->ChatWin->GetPosition().x);
            file->Write(wxT("ChatfensterPosY"), BuddyFrame->ChatWin->GetPosition().y);
            file->Write(wxT("ChatfensterWidth"), BuddyFrame->ChatWin->GetSize().GetWidth());
            file->Write(wxT("ChatfensterHeight"), BuddyFrame->ChatWin->GetSize().GetHeight());
        }
        delete file;
    }

    BuddyFrame->MsgSendTimer->Stop(); BuddyFrame->PopupKillTimer->Stop(); BuddyFrame->PopupTimer->Stop();

    if ( BuddyFrame->Worker != NULL && BuddyFrame->Worker->IsRunning() )
        BuddyFrame->Worker->Delete();

    if ( BuddyFrame->PicWorker != NULL && BuddyFrame->PicWorker->IsRunning() )
        BuddyFrame->PicWorker->Delete();

    if ( BuddyFrame->MsgSender != NULL && BuddyFrame->MsgSender->IsRunning() )
        BuddyFrame->MsgSender->Delete();

    tu_getpage(BuddyFrame->tulink, wxT("https://m.team-ulm.de/Logout"));
    curl_share_cleanup(BuddyFrame->tulink);

    if ( BuddyFrame->ChatWindowOpen == true ) { BuddyFrame->ChatWin->Destroy(); }
    BuddyFrame->TrayIcon->Destroy(); BuddyFrame->Destroy();
}

int TUApp::OnExit()
{
    delete instance;
    return 0;
}

BEGIN_EVENT_TABLE(TUStartFrame, wxFrame)
    EVT_BUTTON(BUTTON_END, TUStartFrame::EndButtonClicked)
    EVT_BUTTON(BUTTON_NEXT, TUStartFrame::NextButtonClicked)
    #ifdef __WXGTK__
        EVT_BUTTON(BUTTON_SHOWPASS, TUStartFrame::ShowPassClicked)
    #endif
    EVT_COMMAND(wxID_ANY, EVENT_WORKERREPORT, TUStartFrame::WorkerReport)
    EVT_TEXT_ENTER(PASSWORDBOX, TUStartFrame::NextButtonClicked)
    EVT_CHECKBOX(AUTOLOGINBOX, TUStartFrame::AutoLoginBoxClicked)
    EVT_TEXT(USERBOX, TUStartFrame::UserChanged)
END_EVENT_TABLE()

TUStartFrame::TUStartFrame(const wxString &title, const wxPoint &pos, const wxSize &size, long style) : wxFrame(NULL, wxID_ANY, title, pos, size, style) { fenster2 = false; Initgui(); }

void TUStartFrame::Initgui()
{
    LoadPos();

    usrinfo = new UserInfo();
    MainSizer = new wxFlexGridSizer(0, 1, 0, 0);
    AuthorLink = new wxHyperlinkCtrl(this, wxID_ANY, wxT("download"), wxT("https://www.team-ulm.de/Profil/190590"), wxDefaultPosition, wxDefaultSize);
    MainSizer->Add(AuthorLink, 1, wxALIGN_RIGHT|wxALIGN_BOTTOM|wxALL, 2);
    TULogo = new wxGenericStaticBitmap(this, wxID_ANY, wxBitmap(GetDataPath() + wxT("/TU.png"), wxBITMAP_TYPE_PNG), wxDefaultPosition, wxSize(150, 150));
    MainSizer->Add(TULogo, 1, wxALL|wxALIGN_BOTTOM|wxALIGN_CENTER_HORIZONTAL, 5);
    Userlabel = new wxStaticText(this, wxID_ANY, wxT("Benutzername:"));
    MainSizer->Add(Userlabel, 1, wxALL|wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    UserBox = new wxComboBox(this, USERBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxTE_PROCESS_ENTER | wxCB_DROPDOWN);
    MainSizer->Add(UserBox, 1, wxALL|wxEXPAND|wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    Passwordlabel = new wxStaticText(this, wxID_ANY, wxT("Passwort:"));
    MainSizer->Add(Passwordlabel, 1, wxALL|wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    PassSizer = new wxFlexGridSizer(2);
    PasswordBox = new wxTextCtrl(this, PASSWORDBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD | wxTE_PROCESS_ENTER);
    PassSizer->Add(PasswordBox, 1, wxALL|wxEXPAND|wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    PassSizer->AddGrowableCol(0, wxHORIZONTAL);
    #ifdef __WXGTK__
        ShowPassButton = new wxBitmapButton(this, BUTTON_SHOWPASS, wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_FIND"))));
        PassSizer->Add(ShowPassButton, 1, wxALL|wxALIGN_RIGHT|wxALIGN_BOTTOM, 5);
    #endif
    MainSizer->Add(PassSizer, 1, wxEXPAND|wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    SavPassBox = new wxCheckBox(this, SAVEPASSBOX, wxT("Passwort speichern"));
    MainSizer->Add(SavPassBox, 1, wxALIGN_LEFT|wxALIGN_BOTTOM|wxALL, 5);
    AutoLoginBox = new wxCheckBox(this, AUTOLOGINBOX, wxT("Automatisch einloggen"));
    MainSizer->Add(AutoLoginBox, 1, wxALIGN_LEFT|wxALIGN_BOTTOM|wxALL, 5);
    ButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    EndButton = new wxButton(this, BUTTON_END, wxT("Beenden"), wxDefaultPosition, wxSize(100, -1));
    ButtonSizer->Add(EndButton, 1, wxALL|wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    ButtonSizer->Add(20, 1);
    NextButton = new wxButton(this, BUTTON_NEXT, wxT("Weiter"), wxDefaultPosition, wxSize(100, -1));
    ButtonSizer->Add(NextButton, 1, wxALL|wxALIGN_RIGHT|wxALIGN_BOTTOM, 5);
    MainSizer->Add(ButtonSizer, 1, wxEXPAND|wxALIGN_LEFT|wxALIGN_BOTTOM, 8);
    SetSizer(MainSizer);
    MainSizer->SetSizeHints(this);
    #if defined __WXGTK__ || defined __WXMAC__
        this->SetIcon(wxIcon(GetDataPath() + wxT("/TUKlein.png"), wxBITMAP_TYPE_PNG));
    #elif defined __WXMSW__
        this->SetIcons(wxIconBundle(GetDataPath() + wxT("/TUTask.ico"), wxBITMAP_TYPE_ICO));
        this->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_MENUBAR));
    #endif
    #ifdef __WXGTK__
        ShowPassButton->SetToolTip(wxT("Passwort im Klartext anzeigen"));
    #endif

    wxFileConfig *file = new wxFileConfig(wxT(""), wxT(""), wxStandardPaths().Get().GetUserDataDir() + wxT("/einstellungen.conf"), wxT(""), wxCONFIG_USE_LOCAL_FILE);
    wxString KnownNames = file->Read(wxT("BekannteBenutzernamen"));
    UserBox->SetValue(file->Read(wxT("AlsLetztes")));
    delete file;
    wxStringTokenizer tokenizer(KnownNames, "|");
    while ( tokenizer.HasMoreTokens() )
    {
        wxString token = tokenizer.GetNextToken();
        UserBox->Append(token);
    }

    if ( fenster2 == true ) { fenster2 = false; }

    Connect(wxID_ANY, wxEVT_CLOSE_WINDOW, (wxObjectEventFunction)&TUStartFrame::OnClose);
    UserBox->Connect(UserBox->GetId(), wxEVT_KEY_DOWN, (wxObjectEventFunction)&TUStartFrame::TabPressEvt);

    // ToolTipps
    UserBox->SetToolTip(wxT("Trage hier deinen Team-Ulm Benutzernamen ein"));
    PasswordBox->SetToolTip(wxT("Trage hier dein Team-Ulm Passwort ein"));
    EndButton->SetToolTip(wxT("Vorgang abbrechen und den TU Messenger beenden"));
    SavPassBox->SetToolTip(wxT("Passwort auf dem Computer abspeichern"));
    AutoLoginBox->SetToolTip(wxT("Diesen Account beim Start automatisch einloggen"));

    // Fokus auf das Benutzerfeld setzen
    UserBox->SetFocus();
}

void TUStartFrame::TabPressEvt(wxKeyEvent &evt)
{
    if ( evt.GetKeyCode() == WXK_TAB ) this->Navigate();
    else evt.Skip();
}

void *TUWorker::Entry()
{
    int returnmsg = tu_login(tuconnection, username, password);
    if ( returnmsg == WRONGLOGIN || returnmsg == NOCONNECTION )
    {
        wxCommandEvent *event = new wxCommandEvent(EVENT_WORKERREPORT, wxID_ANY);
        event->SetInt(returnmsg);
        wxQueueEvent(parent, event);
        return 0;
    }

    // Alles OK, Bild herunterladen!
    if ( !wxDir::Exists(wxStandardPaths().Get().GetUserDataDir()) )
    {
        wxFileName::Mkdir(wxStandardPaths().Get().GetUserDataDir());
    }

    wxString ProfilHtml = tu_getpage(tuconnection, wxT("https://m.team-ulm.de/Profil"));
    usrinfo->username = username;
    usrinfo->id = FilterString(ProfilHtml, wxT("<a href=\"/Profil/"), wxT("\""));
    usrinfo->gender = wxHtmlEntitiesParser().Parse(FilterString(ProfilHtml, wxT("\"trDark\"><td>Geschlecht:</td><td>"), wxT("</td></tr>")));
    usrinfo->lastlogin = FilterString(ProfilHtml, wxT(">Letzter Login:</td><td>"), wxT("</td></tr>"));
    usrinfo->age = FilterString(ProfilHtml, wxT("Alter:</td><td>"), wxT("</td></"));
    usrinfo->single = FilterString(ProfilHtml, wxT("Single:</td><td>"), wxT("</td></"));

    if ( ProfilHtml.Contains(wxT(">Der User hat kein Profilbild.</div>")) )
    {
        tu_filedownload(wxT("http://www.team-ulm.de/grafiken/profil/noImageMedium.png"), wxStandardPaths().Get().GetUserDataDir() + wxT("/profilbild.png"));
    }
    else
    {
        tu_filedownload(wxT("http://www.team-ulm.de/fotos/profil/medium/") + usrinfo->id + wxT(".jpg"), wxStandardPaths().Get().GetUserDataDir() + wxT("/profilbild.jpg"));
    }

    wxCommandEvent *event = new wxCommandEvent(EVENT_WORKERREPORT, wxID_ANY);
    event->SetInt(-4);
    wxQueueEvent(parent, event);

    return 0;
}

#ifdef __WXGTK__
void TUStartFrame::ShowPassClicked(wxCommandEvent &event)
{
    PasswordBox->SetWindowStyle(wxTE_PROCESS_ENTER);
}
#endif

void TUStartFrame::SavePos()
{
    wxString PosX = wxString::Format(wxT("%i"), this->GetPosition().x);
    wxString PosY = wxString::Format(wxT("%i"), this->GetPosition().y);

    if ( !wxDir::Exists(wxStandardPaths().Get().GetUserDataDir()) )
    {
        wxFileName::Mkdir(wxStandardPaths().Get().GetUserDataDir());
    }

    if ( this->GetPosition().x > -10000 && this->GetPosition().x < 10000 && this->GetPosition().y > -10000 && this->GetPosition().y < 10000 )
    {
        wxFileConfig *file = new wxFileConfig(wxT(""), wxT(""), wxStandardPaths().Get().GetUserDataDir() + wxT("/einstellungen.conf"), wxT(""), wxCONFIG_USE_LOCAL_FILE);
        file->Write(wxT("StartFensterPosX"), PosX);
        file->Write(wxT("StartFensterPosY"), PosY);
        delete file;
    }
}

void TUStartFrame::LoadPos()
{
    wxFileConfig *file = new wxFileConfig(wxT(""), wxT(""), wxStandardPaths().Get().GetUserDataDir() + wxT("/einstellungen.conf"), wxT(""), wxCONFIG_USE_LOCAL_FILE);

    if ( file->Read(wxT("StartFensterPosX"), wxEmptyString) == wxEmptyString )
    {
        Center();
    }
    else
    {
        this->SetPosition(wxPoint(wxAtoi(file->Read(wxT("StartFensterPosX"))), wxAtoi(file->Read(wxT("StartFensterPosY")))));
    }

    delete file;
}

void TUStartFrame::OnClose(wxCloseEvent& event)
{
    if ( wxFileExists(wxStandardPaths().Get().GetUserDataDir() + wxT("/profilbild.png")) )
    {
        wxRemoveFile(wxStandardPaths().Get().GetUserDataDir() + wxT("/profilbild.png"));
    }
    if ( wxFileExists(wxStandardPaths().Get().GetUserDataDir() + wxT("/profilbild.jpg")) )
    {
        wxRemoveFile(wxStandardPaths().Get().GetUserDataDir() + wxT("/profilbild.jpg"));
    }

    SavePos();

    delete this;
}

void TUStartFrame::EndButtonClicked(wxCommandEvent &event)
{
    if ( fenster2 == false )
    {
        Close();
    }
    else
    {
        delete UserPic; delete Usernamelabel; delete Usernametext; delete Genderlabel; delete Gendertext; delete Agelabel; delete Agetext; delete Singlelabel; delete Singletext; delete EndButton; delete NextButton;
        curl_share_cleanup(tuconnection);
        tuconnection = curl_share_init();

        if ( wxFileExists(wxStandardPaths().Get().GetUserDataDir() + wxT("/profilbild.png")) )
        {
            wxRemoveFile(wxStandardPaths().Get().GetUserDataDir() + wxT("/profilbild.png"));
        }
        if ( wxFileExists(wxStandardPaths().Get().GetUserDataDir() + wxT("/profilbild.jpg")) )
        {
            wxRemoveFile(wxStandardPaths().Get().GetUserDataDir() + wxT("/profilbild.jpg"));
        }

        Initgui();
    }
}

void TUStartFrame::AutoLoginBoxClicked(wxCommandEvent& event)
{
    if ( AutoLoginBox->IsChecked() == true )
    {
        SavPassBox->SetValue(true); SavPassBox->Disable();
    }
    else
    {
        SavPassBox->Enable();
    }
}

void TUStartFrame::UserChanged(wxCommandEvent& event)
{
    wxFileConfig *file = new wxFileConfig(wxT(""), wxT(""), wxStandardPaths().Get().GetUserDataDir() + wxT("/einstellungen.conf"), wxT(""), wxCONFIG_USE_LOCAL_FILE);
    wxString KnownNames = file->Read(wxT("BekannteBenutzernamen"));
    if ( KnownNames.Contains(wxT("|") + UserBox->GetValue()) == true || KnownNames.Contains(UserBox->GetValue() + wxT("|")) == true || KnownNames == UserBox->GetValue() )
    {
        NextButton->SetLabel(wxT("Login"));
        wxString Pass = file->Read(UserBox->GetValue() + wxT("Passwort"));
        if ( Pass != wxEmptyString )
        {
            PasswordBox->SetValue(DecryptString(Pass));
            SavPassBox->SetValue(true);
        }
        else
        {
            SavPassBox->SetValue(false);
            PasswordBox->SetValue(wxEmptyString);
        }
        if ( file->Read(wxT("Autologin")) == UserBox->GetValue() )
        {
            AutoLoginBox->SetValue(true);
            SavPassBox->Disable();
            SavPassBox->SetValue(true);
        }
        else
        {
            SavPassBox->Enable();
            AutoLoginBox->SetValue(false);
        }
    }
    else
    {
        SavPassBox->SetValue(false);
        SavPassBox->Enable();
        AutoLoginBox->SetValue(false);
        PasswordBox->SetValue(wxEmptyString);
        NextButton->SetLabel(wxT("Weiter"));
    }
}

void TUStartFrame::NextButtonClicked(wxCommandEvent &event)
{
    if ( NextButton->GetLabel() == wxT("Login") )
    {
        if ( SavPassBox->IsChecked() == true ) SavePassUser = true;
        else SavePassUser = false;
        if ( AutoLoginBox->IsChecked() == true ) AutoLoginUser = true;
        else AutoLoginUser = false;

        NextButton->Disable();
        EndButton->Disable();
        this->SetCursor(wxCURSOR_ARROWWAIT);

        tuconnection = curl_share_init();

        TUWorker *Worker = new TUWorker();
        Worker->Create();
        Worker->parent = this;
        Worker->username = UserBox->GetValue();
        Worker->password = PasswordBox->GetValue();
        usrinfo->password = PasswordBox->GetValue();
        Worker->Run();

        return;
    }

    if ( fenster2 == false )
    {
        if ( SavPassBox->IsChecked() == true ) SavePassUser = true;
        else SavePassUser = false;
        if ( AutoLoginBox->IsChecked() == true ) AutoLoginUser = true;
        else AutoLoginUser = false;

        NextButton->Disable();
        EndButton->Disable();
        this->SetCursor(wxCURSOR_ARROWWAIT);

        TUWorker *Worker = new TUWorker();
        Worker->Create();
        Worker->parent = this;
        Worker->username = UserBox->GetValue();
        Worker->password = PasswordBox->GetValue();
        usrinfo->password = PasswordBox->GetValue();
        Worker->Run();
    }
    else
    {
        // Abspeichern
        wxFileConfig *file = new wxFileConfig(wxT(""), wxT(""), wxStandardPaths().Get().GetUserDataDir() + wxT("/einstellungen.conf"), wxT(""), wxCONFIG_USE_LOCAL_FILE);
        wxString cryptedpass = EncryptString(usrinfo->password);
        if ( SavePassUser == true || AutoLoginUser == true ) file->Write(usrinfo->username + wxT("Passwort"), cryptedpass);
        if ( AutoLoginUser == true ) file->Write(wxT("Autologin"), usrinfo->username);

        wxString KnownNames = file->Read(wxT("BekannteBenutzernamen"));
        if ( KnownNames == wxEmptyString ) KnownNames = usrinfo->username;
        else KnownNames += (wxT("|") + usrinfo->username);
        file->Write(wxT("BekannteBenutzernamen"), KnownNames);
        file->Write(wxT("AlsLetztes"), usrinfo->username);
        delete file;

        BuddyFrame = new TUBuddylist(wxT("Buddyliste"), wxDefaultPosition, wxSize(270, 570), wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxRESIZE_BORDER);
        BuddyFrame->startHidden = false;
        BuddyFrame->FromStartWindow = true;
        BuddyFrame->username = usrinfo->username;
        BuddyFrame->tulink = tuconnection;
        BuddyFrame->OnStart();

        delete usrinfo;
        Close();
    }
}

void TUStartFrame::WorkerReport(wxCommandEvent &evt)
{
    this->SetCursor(wxCURSOR_ARROW);

    if ( evt.GetInt() == WRONGLOGIN )
    {
        wxMessageBox(wxT("Du hast falsche Logindaten eingegeben.\nBitte versuche es erneut."), wxT("Error: Falsche Logindaten"), wxICON_ERROR | wxOK);
        NextButton->Enable();
        EndButton->Enable();
        return;
    }
    else if ( evt.GetInt() == NOCONNECTION )
    {
        wxMessageBox(wxT("Der TU Messenger kann keine Verbindung zu team-ulm.de aufbauen.\nBitte überprüfe deine Internetverbindung."), wxT("Error: Keine Verbindung möglich"), wxICON_ERROR | wxOK);
        NextButton->Enable();
        EndButton->Enable();
        return;
    }

    if ( NextButton->GetLabel() == wxT("Login") )
    {
        // Abspeichern
        wxFileConfig *file = new wxFileConfig(wxT(""), wxT(""), wxStandardPaths().Get().GetUserDataDir() + wxT("/einstellungen.conf"), wxT(""), wxCONFIG_USE_LOCAL_FILE);
        if ( SavePassUser == false && AutoLoginUser == false ) file->Write(usrinfo->username + wxT("Passwort"), wxT(""));
        else file->Write(usrinfo->username + wxT("Passwort"), EncryptString(usrinfo->password));
        if ( file->Read(wxT("Autologin")) == usrinfo->username && AutoLoginUser == false ) file->Write(wxT("Autologin"), wxT(""));
        if ( AutoLoginUser == true ) file->Write(wxT("Autologin"), usrinfo->username);
        file->Write(wxT("AlsLetztes"), usrinfo->username);
        delete file;

        BuddyFrame = new TUBuddylist(wxT("Buddyliste"), wxDefaultPosition, wxSize(270, 570), wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxRESIZE_BORDER);
        BuddyFrame->FromStartWindow = true;
        BuddyFrame->startHidden = false;
        BuddyFrame->username = usrinfo->username;
        BuddyFrame->tulink = tuconnection;
        BuddyFrame->OnStart();

        delete usrinfo;
        Close();
        return;
    }

    delete TULogo; delete Userlabel; delete UserBox; delete Passwordlabel; delete PasswordBox; delete AuthorLink; delete SavPassBox; delete AutoLoginBox; MainSizer->Detach(ButtonSizer); MainSizer->Remove(0);
    #ifdef __WXGTK__
        delete ShowPassButton;
    #endif

    fenster2 = true;

    EndButton->SetLabel(wxT("Zurück"));
    EndButton->SetToolTip(wxT("Zurück zum Eingabefenster"));
    NextButton->SetLabel(wxT("Fertigstellen"));
    NextButton->Enable();
    EndButton->Enable();
    NextButton->SetFocus();

    wxFlexGridSizer *MainSizer2 = new wxFlexGridSizer(3, 1, 0, 0);
    if ( wxFileExists(wxStandardPaths().Get().GetUserDataDir() + wxT("/profilbild.png")) )
    {
        UserPic = new wxGenericStaticBitmap(this, wxID_ANY, wxBitmap(wxStandardPaths().Get().GetUserDataDir() + wxT("/profilbild.png"), wxBITMAP_TYPE_PNG), wxPoint(68, 10), wxSize(150, 150));
    }
    else
    {
        UserPic = new wxGenericStaticBitmap(this, wxID_ANY, wxBitmap(wxStandardPaths().Get().GetUserDataDir() + wxT("/profilbild.jpg"), wxBITMAP_TYPE_JPEG), wxPoint(68, 10), wxSize(150, 150));
    }
    MainSizer2->Add(UserPic, 1, wxALL|wxALIGN_BOTTOM|wxALIGN_CENTER_HORIZONTAL, 5);

    wxGridSizer *InfoSizer = new wxGridSizer(4, 2, 0, 0);
    Usernamelabel = new wxStaticText(this, wxID_ANY, wxT("Benutzername:"));
    InfoSizer->Add(Usernamelabel, 1, wxALL|wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    Usernametext = new wxStaticText(this, wxID_ANY, usrinfo->username);
    InfoSizer->Add(Usernametext, 1, wxALL|wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    Genderlabel = new wxStaticText(this, wxID_ANY, wxT("Geschlecht:"));
    InfoSizer->Add(Genderlabel, 1, wxALL|wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    Gendertext = new wxStaticText(this, wxID_ANY, usrinfo->gender);
    InfoSizer->Add(Gendertext, 1, wxALL|wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    Agelabel = new wxStaticText(this, wxID_ANY, wxT("Alter:"));
    InfoSizer->Add(Agelabel, 1, wxALL|wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    Agetext = new wxStaticText(this, wxID_ANY, usrinfo->age);
    InfoSizer->Add(Agetext, 1, wxALL|wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    Singlelabel = new wxStaticText(this, wxID_ANY, wxT("Single:"));
    InfoSizer->Add(Singlelabel, 1, wxALL|wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    Singletext = new wxStaticText(this, wxID_ANY, usrinfo->single);
    InfoSizer->Add(Singletext, 1, wxALL|wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    MainSizer2->Add(InfoSizer, 1, wxALL|wxEXPAND|wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    MainSizer2->Add(ButtonSizer, 1, wxEXPAND|wxALIGN_LEFT|wxALIGN_BOTTOM, 8);
    SetSizer(MainSizer2);
    MainSizer2->SetSizeHints(this);

    NextButton->SetFocus();
}
