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
#include <curl/curl.h>
#include <wx/hyperlink.h>
#include <wx/snglinst.h>

class TUApp: public wxApp
{
    virtual bool OnInit();
    virtual int OnExit();
    int FilterEvent(wxEvent &event);
    void EndSess(wxCloseEvent &event);
    wxSingleInstanceChecker *instance;
};

class TUStartFrame: public wxFrame
{
    public:
        TUStartFrame(const wxString &title, const wxPoint &pos, const wxSize &size, long style);

    private:
        wxFlexGridSizer *MainSizer;
        wxFlexGridSizer *PassSizer;
        wxBoxSizer *ButtonSizer;

        wxGenericStaticBitmap *TULogo;
        wxStaticText *Userlabel;
        wxComboBox *UserBox;
        wxStaticText *Passwordlabel;
        wxTextCtrl *PasswordBox;
        #ifdef __WXGTK__
        wxBitmapButton *ShowPassButton;
        #endif
        wxCheckBox *SavPassBox, *AutoLoginBox;
        wxButton *EndButton;
        wxButton *NextButton;
        wxHyperlinkCtrl *AuthorLink;

        // Fenster 2
        wxGenericStaticBitmap *UserPic;
        wxStaticText *Usernamelabel;
        wxStaticText *Usernametext;
        wxStaticText *Genderlabel;
        wxStaticText *Gendertext;
        wxStaticText *Agelabel;
        wxStaticText *Agetext;
        wxStaticText *Singlelabel;
        wxStaticText *Singletext;

        wxTimer *SaveTimer;

        bool fenster2;
        bool AutoLoginUser, SavePassUser;

        void AutoLoginBoxClicked(wxCommandEvent& event);
        void UserChanged(wxCommandEvent& event);
        void EndButtonClicked(wxCommandEvent &event);
        void OnClose(wxCloseEvent& event);
        void NextButtonClicked(wxCommandEvent &event);
        #ifdef __WXGTK__
        void ShowPassClicked(wxCommandEvent &event);
        #endif
        void WorkerReport(wxCommandEvent &evt);
        void TabPressEvt(wxKeyEvent &evt);
        void SavePos();
        void LoadPos();

        void Initgui();
    DECLARE_EVENT_TABLE();
};

class TUWorker: public wxThread
{
    public:
        TUStartFrame *parent;
        wxString username;
        wxString password;

        virtual void* Entry();
};

struct UserInfo
{
    wxString username;
    wxString password;
    wxString id;
    wxString gender;
    wxString lastlogin;
    wxString age;
    wxString single;
};
