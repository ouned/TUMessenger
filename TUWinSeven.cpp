#include <wx/wx.h>
#include <wx/dir.h>
#include <wx/stdpaths.h>
#include <windows.h>
#include <shlwapi.h>
#ifndef  __GNUC__
    #include <Shobjidl.h>
#endif

std::wstring getPathLinkIsPointingTo(std::wstring lpszLinkFile)
{
  IShellLinkW* psl;
  std::wstring path;

  if (CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
    IID_IShellLinkW, (LPVOID*)&psl)==S_OK)
  {
    IPersistFile* ppf;
    if (psl->QueryInterface(IID_IPersistFile, (void**)&ppf)==S_OK)
    {
      if (ppf->Load(lpszLinkFile.c_str(), STGM_READ)==S_OK)
      {
        if(psl->Resolve(NULL, 0)==S_OK)
        {
          WCHAR szGotPath[MAX_PATH];
          if (psl->GetPath(szGotPath, MAX_PATH, NULL, SLGP_UNCPRIORITY)==S_OK)
          {
            path = szGotPath;
          }
        }
      }
      ppf->Release();
    }
    psl->Release();
  }

  return path;
}

bool IsProgramPinned()
{
    wxString tumExecutablePath = wxStandardPaths().GetExecutablePath();
    wxString winScPath = wxStandardPaths().GetUserConfigDir() + wxT("\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar");

    wxArrayString shortcuts;
    wxDir().GetAllFiles(winScPath, &shortcuts, wxEmptyString);

    for ( int fileNum = 0; (size_t)fileNum < shortcuts.GetCount(); fileNum++ )
    {
        std::wstring tp = getPathLinkIsPointingTo(shortcuts[fileNum].wc_str());
        wxString linkOfSc(tp);

        if ( linkOfSc == tumExecutablePath )
            return true;
    }

    return false;
}
