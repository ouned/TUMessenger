/////////////////////////////////////////////////////////////////////////////
// Name:        wxiepanel.cpp
// Purpose:     wxMSW wxIEPanel class implementation for web view component
// Author:      Marianne Gagnon
// Id:          $Id$
// Copyright:   (c) 2010 Marianne Gagnon
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wxiepanel.h"

#if wxHAVE_WEB_BACKEND_IE

#include "wx/wx.h"
#include <olectl.h>
#include <oleidl.h>
#include <exdispid.h>
#include <exdisp.h>
#include <Mshtml.h>

// FIXME: according to Microsoft's docs, this is supposed to be in exdispid.h...
#ifdef __GNUC__
#define DISPID_COMMANDSTATECHANGE   105
typedef enum CommandStateChangeConstants {
    CSC_UPDATECOMMANDS = (int) 0xFFFFFFFF,
    CSC_NAVIGATEFORWARD = 0x1,
    CSC_NAVIGATEBACK = 0x2
} CommandStateChangeConstants;


// FIXME: I can't imagine that Microsoft doesn't provide those in some
//        header so users don't need to hardcode them...
#define DISPID_NAVIGATECOMPLETE2    252
#define DISPID_NAVIGATEERROR        271
#define OLECMDID_OPTICAL_ZOOM  63
#define INET_E_ERROR_FIRST 0x800C0002L
#define INET_E_INVALID_URL 0x800C0002L
#define INET_E_NO_SESSION 0x800C0003L
#define INET_E_CANNOT_CONNECT 0x800C0004L
#define INET_E_RESOURCE_NOT_FOUND 0x800C0005L
#define INET_E_OBJECT_NOT_FOUND 0x800C0006L
#define INET_E_DATA_NOT_AVAILABLE 0x800C0007L
#define INET_E_DOWNLOAD_FAILURE 0x800C0008L
#define INET_E_AUTHENTICATION_REQUIRED 0x800C0009L
#define INET_E_NO_VALID_MEDIA 0x800C000AL
#define INET_E_CONNECTION_TIMEOUT 0x800C000BL
#define INET_E_INVALID_REQUEST 0x800C000CL
#define INET_E_UNKNOWN_PROTOCOL 0x800C000DL
#define INET_E_SECURITY_PROBLEM 0x800C000EL
#define INET_E_CANNOT_LOAD_DATA 0x800C000FL
#define INET_E_CANNOT_INSTANTIATE_OBJECT 0x800C0010L
#define INET_E_QUERYOPTION_UNKNOWN 0x800C0013L
#define INET_E_REDIRECT_FAILED 0x800C0014L
#define INET_E_REDIRECT_TO_DIR 0x800C0015L
#define INET_E_CANNOT_LOCK_REQUEST 0x800C0016L
#define INET_E_USE_EXTEND_BINDING 0x800C0017L
#define INET_E_TERMINATED_BIND 0x800C0018L
#define INET_E_INVALID_CERTIFICATE 0x800C0019L
#define INET_E_CODE_DOWNLOAD_DECLINED 0x800C0100L
#define INET_E_RESULT_DISPATCHED 0x800C0200L
#define INET_E_CANNOT_REPLACE_SFP_FILE 0x800C0300L
#define INET_E_CODE_INSTALL_BLOCKED_BY_HASH_POLICY 0x800C0500L
#define INET_E_CODE_INSTALL_SUPPRESSED 0x800C0400L
#endif
#define REFRESH_COMPLETELY 3

BEGIN_EVENT_TABLE(wxIEPanel, wxControl)
EVT_ACTIVEX(wxID_ANY, wxIEPanel::onActiveXEvent)
EVT_ERASE_BACKGROUND(wxIEPanel::onEraseBg)
END_EVENT_TABLE()

bool wxIEPanel::Create(wxWindow* parent,
           wxWindowID id,
           const wxString& url,
           const wxPoint& pos,
           const wxSize& size,
           long style,
           const wxString& name)
{
	if (!wxControl::Create(parent, id, pos, size, style, wxDefaultValidator, name))
	{
		return false;
	}

    m_web_browser = NULL;
    m_can_navigate_back = false;
    m_can_navigate_forward = false;
    m_is_busy = false;

    if (::CoCreateInstance(CLSID_WebBrowser, NULL,
                           CLSCTX_INPROC_SERVER, // CLSCTX_INPROC,
                           IID_IWebBrowser2 , (void**)&m_web_browser) != 0)
    {
        wxLogError("Failed to initialize IEPanel, CoCreateInstance returned an error");
        return false;
    }

    m_ie.SetDispatchPtr(m_web_browser); // wxAutomationObject will release itself

    m_web_browser->put_RegisterAsBrowser(VARIANT_TRUE);
    m_web_browser->put_RegisterAsDropTarget(VARIANT_TRUE);
    //m_web_browser->put_Silent(VARIANT_F);

    m_container = new wxActiveXContainer(this, IID_IWebBrowser2, m_web_browser);

	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetDoubleBuffered(true);
	return true;
}


void wxIEPanel::LoadUrl(wxString url)
{
    wxVariant out = m_ie.CallMethod("Navigate", (BSTR) url.wc_str(), NULL, NULL, NULL, NULL);
    if (out.GetType() != "null")
    {
        wxMessageBox("Non-null return message : " + out.GetType());
    }

    // FIXME: why is out value null??
    //(HRESULT)(out.GetLong()) == S_OK;
}

void wxIEPanel::SetPage(wxString html, wxString baseUrl)
{
    LoadUrl("about:blank");

    // Let the wx events generated for navigation events be processed, so that the underlying
    // IE component completes its Document object.
    // FIXME: calling wxYield is not elegant nor very reliable probably
    wxYield();

    wxVariant documentVariant = m_ie.GetProperty("Document");
    void* documentPtr = documentVariant.GetVoidPtr();

    wxASSERT (documentPtr != NULL);

	// TODO: consider the "baseUrl" parameter if possible
    // TODO: consider encoding
    BSTR bstr = SysAllocString(html.wc_str());

    // Creates a new one-dimensional array
    SAFEARRAY *psaStrings = SafeArrayCreateVector(VT_VARIANT, 0, 1);
    if (psaStrings != NULL)
    {
        VARIANT *param;
        HRESULT hr = SafeArrayAccessData(psaStrings, (LPVOID*)&param);
        param->vt = VT_BSTR;
        param->bstrVal = bstr;

        hr = SafeArrayUnaccessData(psaStrings);

        IHTMLDocument2* document = (IHTMLDocument2*)documentPtr;
        document->write(psaStrings);

        // SafeArrayDestroy calls SysFreeString for each BSTR
        SafeArrayDestroy(psaStrings);
    }
    else
    {
        wxLogError("wxIEPanel::SetPage() : psaStrings is NULL");
    }

}

wxString wxIEPanel::GetPageSource()
{
    wxVariant documentVariant = m_ie.GetProperty("Document");
    void* documentPtr = documentVariant.GetVoidPtr();

    if (documentPtr == NULL)
    {
        return wxEmptyString;
    }

    IHTMLDocument2* document = (IHTMLDocument2*)documentPtr;

    IHTMLElement *bodyTag = NULL;
    IHTMLElement *htmlTag = NULL;
    document->get_body(&bodyTag);
    wxASSERT(bodyTag != NULL);

    document->Release();
    bodyTag->get_parentElement(&htmlTag);
    wxASSERT(htmlTag != NULL);

    BSTR    bstr;
    htmlTag->get_outerHTML(&bstr);

    bodyTag->Release();
    htmlTag->Release();

    //wxMessageBox(wxString(bstr));

    // TODO: check encoding
    return wxString(bstr);
}

// FIXME? retrieve OLECMDID_GETZOOMRANGE instead of hardcoding range 0-4
wxWebViewZoom wxIEPanel::GetZoom()
{
    const int zoom = GetIETextZoom();

    switch (zoom)
    {
        case 0:
            return wxWEB_VIEW_ZOOM_TINY;
            break;
        case 1:
            return wxWEB_VIEW_ZOOM_SMALL;
            break;
        case 2:
            return wxWEB_VIEW_ZOOM_MEDIUM;
            break;
        case 3:
            return wxWEB_VIEW_ZOOM_LARGE;
            break;
        case 4:
            return wxWEB_VIEW_ZOOM_LARGEST;
            break;
        default:
            wxASSERT(false);
            return wxWEB_VIEW_ZOOM_MEDIUM;
    }
}
void wxIEPanel::SetZoom(wxWebViewZoom zoom)
{
    // I know I could cast from enum to int since wxWebViewZoom happens to match
    // with IE's zoom levels, but I don't like doing that, what if enum values change...
    switch (zoom)
    {
        case wxWEB_VIEW_ZOOM_TINY:
            SetIETextZoom(0);
            break;
        case wxWEB_VIEW_ZOOM_SMALL:
            SetIETextZoom(1);
            break;
        case wxWEB_VIEW_ZOOM_MEDIUM:
            SetIETextZoom(2);
            break;
        case wxWEB_VIEW_ZOOM_LARGE:
            SetIETextZoom(3);
            break;
        case wxWEB_VIEW_ZOOM_LARGEST:
            SetIETextZoom(4);
            break;
        default:
            wxASSERT(false);
    }
}

void wxIEPanel::SetIETextZoom(int level)
{
    VARIANT zoomVariant;
    VariantInit (&zoomVariant);
    V_VT(&zoomVariant) = VT_I4;
    V_I4(&zoomVariant) = level;

    HRESULT result = m_web_browser->ExecWB(OLECMDID_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER,
                                           &zoomVariant, NULL);
    wxASSERT (result == S_OK);

    VariantClear (&zoomVariant);
}

int wxIEPanel::GetIETextZoom()
{
    VARIANT zoomVariant;
    VariantInit (&zoomVariant);
    V_VT(&zoomVariant) = VT_I4;
    V_I4(&zoomVariant) = 4;

    HRESULT result = m_web_browser->ExecWB(OLECMDID_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER,
                                           NULL, &zoomVariant);
    wxASSERT (result == S_OK);

    int zoom = V_I4(&zoomVariant);
   // wxMessageBox(wxString::Format("Zoom : %i", zoom));
    VariantClear (&zoomVariant);

    return zoom;
}

void wxIEPanel::SetIEOpticalZoom(float zoom)
{
	// TODO: add support for optical zoom (IE7+ only)

	// TODO: get range from OLECMDID_OPTICAL_GETZOOMRANGE instead of hardcoding?
	wxASSERT(zoom >= 10.0f);
	wxASSERT(zoom <= 1000.0f);

	VARIANT zoomVariant;
    VariantInit (&zoomVariant);
    V_VT(&zoomVariant) = VT_I4;
    V_I4(&zoomVariant) = (zoom * 100.0f);

	HRESULT result = m_web_browser->ExecWB((OLECMDID)OLECMDID_OPTICAL_ZOOM, OLECMDEXECOPT_DODEFAULT, &zoomVariant, NULL);
	wxASSERT (result == S_OK);
}

float wxIEPanel::GetIEOpticalZoom()
{
	// TODO: add support for optical zoom (IE7+ only)

	VARIANT zoomVariant;
    VariantInit (&zoomVariant);
    V_VT(&zoomVariant) = VT_I4;
    V_I4(&zoomVariant) = -1;

	HRESULT result = m_web_browser->ExecWB((OLECMDID)OLECMDID_OPTICAL_ZOOM, OLECMDEXECOPT_DODEFAULT, NULL, &zoomVariant);
	wxASSERT (result == S_OK);

    const int zoom = V_I4(&zoomVariant);
    VariantClear (&zoomVariant);

	return zoom / 100.0f;
}

void wxIEPanel::SetZoomType(wxWebViewZoomType)
{
	// TODO: add support for optical zoom (IE7+ only)
	wxASSERT(false);
}

wxWebViewZoomType wxIEPanel::GetZoomType() const
{
	// TODO: add support for optical zoom (IE7+ only)
	return wxWEB_VIEW_ZOOM_TYPE_TEXT;
}

bool wxIEPanel::CanSetZoomType(wxWebViewZoomType) const
{
	// both are supported
	// TODO: IE6 only supports text zoom, check if it's IE6 first
	return true;
}

void wxIEPanel::Print()
{
    m_web_browser->ExecWB(OLECMDID_PRINTPREVIEW, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
}

void wxIEPanel::GoBack()
{
    wxVariant out = m_ie.CallMethod("GoBack");
    if (out.GetType() != "null")
    {
        wxMessageBox("Non-null return message : " + out.GetType());
    }

    // FIXME: why is out value null??
    //return (HRESULT)(out.GetLong()) == S_OK;
}

void wxIEPanel::GoForward()
{
    wxVariant out = m_ie.CallMethod("GoForward");
    if (out.GetType() != "null")
    {
        wxMessageBox("Non-null return message : " + out.GetType());
    }

    // FIXME: why is out value null??
    //return (HRESULT)(out.GetLong()) == S_OK;
}

void wxIEPanel::Stop()
{
    wxVariant out = m_ie.CallMethod("Stop");
    if (out.GetType() != "null")
    {
        wxMessageBox("Non-null return message : " + out.GetType());
    }

    // FIXME: why is out value null??
    //return (HRESULT)(out.GetLong()) == S_OK;
}


void wxIEPanel::Reload(bool bypassCache)
{
    wxVariant out;

    if (bypassCache)
    {
        VARIANTARG level;
        level.vt = VT_I2;
        level.iVal = 3;
        out = m_ie.CallMethod("Refresh2", &level);
    }
    else
    {
        out = m_ie.CallMethod("Refresh");
    }

    if (out.GetType() != "null")
    {
        wxMessageBox("Non-null return message : " + out.GetType());
    }

    //m_container->m_oleInPlaceObject->UIDeactivate();
}

bool wxIEPanel::IsOfflineMode()
{
    wxVariant out = m_ie.GetProperty("Offline");

    wxASSERT(out.GetType() == "bool");

    return out.GetBool();
}

void wxIEPanel::SetOfflineMode(bool offline)
{
    // FIXME: the wxWidgets docs do not really document what the return parameter of PutProperty is
    const bool success = m_ie.PutProperty("Offline", (offline ? VARIANT_TRUE : VARIANT_FALSE));
    wxASSERT(success);
}

bool wxIEPanel::IsBusy()
{
	if (m_is_busy) return true;

    wxVariant out = m_ie.GetProperty("Busy");

    wxASSERT(out.GetType() == "bool");

    return out.GetBool();
}

wxString wxIEPanel::GetCurrentURL()
{
    wxVariant out = m_ie.GetProperty("LocationURL");

    wxASSERT(out.GetType() == "string");
    return out.GetString();
}

wxString wxIEPanel::GetCurrentTitle()
{
    wxVariant out = m_ie.GetProperty("LocationName");

    wxASSERT(out.GetType() == "string");
    return out.GetString();
}

void wxIEPanel::onActiveXEvent(wxActiveXEvent& evt)
{
    if (m_web_browser == NULL) return;

    switch (evt.GetDispatchId())
    {
        case DISPID_BEFORENAVIGATE2:
        {
			m_is_busy = true;

            wxString url = evt[1].GetString();
            wxString target = evt[3].GetString();

            wxWebNavigationEvent event(wxEVT_COMMAND_WEB_VIEW_NAVIGATING, GetId(), url, target, true);
            event.SetEventObject(this);
            HandleWindowEvent(event);

            if (event.IsVetoed())
            {
                wxActiveXEventNativeMSW* nativeParams = evt.GetNativeParameters();
                *V_BOOLREF(&nativeParams->pDispParams->rgvarg[0]) = VARIANT_TRUE;
            }

			// at this point, either the navigation event has been cancelled and we're not busy, either
			// it was accepted and IWebBrowser2's Bus property will be true; so we don't need our override
			// flag anymore.
			m_is_busy = false;

            break;
        }

        case DISPID_NAVIGATECOMPLETE2:
        {
            wxString url = evt[1].GetString();
            // TODO: target parameter
            wxString target = wxEmptyString;
            wxWebNavigationEvent event(wxEVT_COMMAND_WEB_VIEW_NAVIGATED, GetId(), url, target, false);
            event.SetEventObject(this);
            HandleWindowEvent(event);
            break;
        }

        case DISPID_PROGRESSCHANGE:
        {
            // download progress
            break;
        }

        case DISPID_DOCUMENTCOMPLETE:
        {
            wxString url = evt[1].GetString();
            // TODO: target parameter
            wxString target = wxEmptyString;
            wxWebNavigationEvent event(wxEVT_COMMAND_WEB_VIEW_LOADED, GetId(), url, target, false);
            event.SetEventObject(this);
            HandleWindowEvent(event);
            break;
        }

        case DISPID_STATUSTEXTCHANGE:
        {
            break;
        }

        case DISPID_TITLECHANGE:
        {
            break;
        }

        case DISPID_NAVIGATEERROR:
        {
            wxWebNavigationError errorType = wxWEB_NAV_ERR_OTHER;
            wxString errorCode = "?";
            switch (evt[3].GetLong())
            {
            case INET_E_INVALID_URL: // (0x800C0002L or -2146697214)
                errorCode = "INET_E_INVALID_URL";
                errorType = wxWEB_NAV_ERR_REQUEST;
                break;
            case INET_E_NO_SESSION: // (0x800C0003L or -2146697213)
                errorCode = "INET_E_NO_SESSION";
                errorType = wxWEB_NAV_ERR_CONNECTION;
                break;
            case INET_E_CANNOT_CONNECT: // (0x800C0004L or -2146697212)
                errorCode = "INET_E_CANNOT_CONNECT";
                errorType = wxWEB_NAV_ERR_CONNECTION;
                break;
            case INET_E_RESOURCE_NOT_FOUND: // (0x800C0005L or -2146697211)
                errorCode = "INET_E_RESOURCE_NOT_FOUND";
                errorType = wxWEB_NAV_ERR_NOT_FOUND;
                break;
            case INET_E_OBJECT_NOT_FOUND: // (0x800C0006L or -2146697210)
                errorCode = "INET_E_OBJECT_NOT_FOUND";
                errorType = wxWEB_NAV_ERR_NOT_FOUND;
                break;
            case INET_E_DATA_NOT_AVAILABLE: // (0x800C0007L or -2146697209)
                errorCode = "INET_E_DATA_NOT_AVAILABLE";
                errorType = wxWEB_NAV_ERR_NOT_FOUND;
                break;
            case INET_E_DOWNLOAD_FAILURE: // (0x800C0008L or -2146697208)
                errorCode = "INET_E_DOWNLOAD_FAILURE";
                errorType = wxWEB_NAV_ERR_CONNECTION;
                break;
            case INET_E_AUTHENTICATION_REQUIRED: // (0x800C0009L or -2146697207)
                errorCode = "INET_E_AUTHENTICATION_REQUIRED";
                errorType = wxWEB_NAV_ERR_AUTH;
                break;
            case INET_E_NO_VALID_MEDIA: // (0x800C000AL or -2146697206)
                errorCode = "INET_E_NO_VALID_MEDIA";
                errorType = wxWEB_NAV_ERR_REQUEST;
                break;
            case INET_E_CONNECTION_TIMEOUT: // (0x800C000BL or -2146697205)
                errorCode = "INET_E_CONNECTION_TIMEOUT";
                errorType = wxWEB_NAV_ERR_CONNECTION;
                break;
            case INET_E_INVALID_REQUEST: // (0x800C000CL or -2146697204)
                errorCode = "INET_E_INVALID_REQUEST";
                errorType = wxWEB_NAV_ERR_REQUEST;
                break;
            case INET_E_UNKNOWN_PROTOCOL: // (0x800C000DL or -2146697203)
                errorCode = "INET_E_UNKNOWN_PROTOCOL";
                errorType = wxWEB_NAV_ERR_REQUEST;
                break;
            case INET_E_SECURITY_PROBLEM: // (0x800C000EL or -2146697202)
                errorCode = "INET_E_SECURITY_PROBLEM";
                errorType = wxWEB_NAV_ERR_SECURITY;
                break;
            case INET_E_CANNOT_LOAD_DATA: // (0x800C000FL or -2146697201)
                errorCode = "INET_E_CANNOT_LOAD_DATA";
                errorType = wxWEB_NAV_ERR_OTHER;
                break;
            case INET_E_CANNOT_INSTANTIATE_OBJECT: // (0x800C0010L or -2146697200)
                // CoCreateInstance will return an error code if this happens, we'll handle this above.
                return;
                break;
            case INET_E_REDIRECT_FAILED: // (0x800C0014L or -2146697196)
                errorCode = "INET_E_REDIRECT_FAILED";
                errorType = wxWEB_NAV_ERR_OTHER;
                break;
            case INET_E_REDIRECT_TO_DIR: // (0x800C0015L or -2146697195)
                errorCode = "INET_E_REDIRECT_TO_DIR";
                errorType = wxWEB_NAV_ERR_REQUEST;
                break;
            case INET_E_CANNOT_LOCK_REQUEST: // (0x800C0016L or -2146697194)
                errorCode = "INET_E_CANNOT_LOCK_REQUEST";
                errorType = wxWEB_NAV_ERR_OTHER;
                break;
            case INET_E_USE_EXTEND_BINDING: // (0x800C0017L or -2146697193)
                errorCode = "INET_E_USE_EXTEND_BINDING";
                errorType = wxWEB_NAV_ERR_OTHER;
                break;
            case INET_E_TERMINATED_BIND: // (0x800C0018L or -2146697192)
                errorCode = "INET_E_TERMINATED_BIND";
                errorType = wxWEB_NAV_ERR_OTHER;
                break;
            case INET_E_INVALID_CERTIFICATE: // (0x800C0019L or -2146697191)
                errorCode = "INET_E_INVALID_CERTIFICATE";
                errorType = wxWEB_NAV_ERR_CERTIFICATE;
                break;
            case INET_E_CODE_DOWNLOAD_DECLINED: // (0x800C0100L or -2146696960)
                errorCode = "INET_E_CODE_DOWNLOAD_DECLINED";
                errorType = wxWEB_NAV_ERR_USER_CANCELLED;
                break;
            case INET_E_RESULT_DISPATCHED: // (0x800C0200L or -2146696704)
                // cancel request cancelled...
                errorCode = "INET_E_RESULT_DISPATCHED";
                errorType = wxWEB_NAV_ERR_OTHER;
                break;
            case INET_E_CANNOT_REPLACE_SFP_FILE: // (0x800C0300L or -2146696448)
                errorCode = "INET_E_CANNOT_REPLACE_SFP_FILE";
                errorType = wxWEB_NAV_ERR_SECURITY;
                break;
            case INET_E_CODE_INSTALL_BLOCKED_BY_HASH_POLICY: // (0x800C0500L or -2146695936)
                errorCode = "INET_E_CODE_INSTALL_BLOCKED_BY_HASH_POLICY";
                errorType = wxWEB_NAV_ERR_SECURITY;
                break;
            case INET_E_CODE_INSTALL_SUPPRESSED: // (0x800C0400L or -2146696192)
                errorCode = "INET_E_CODE_INSTALL_SUPPRESSED";
                errorType = wxWEB_NAV_ERR_SECURITY;
                break;
            }

            wxString url = evt[1].GetString();
            wxString target = evt[2].GetString();
            wxWebNavigationEvent event(wxEVT_COMMAND_WEB_VIEW_ERROR, GetId(), url, target, false);
            event.SetEventObject(this);
            event.SetInt(errorType);
            event.SetString(errorCode);
            HandleWindowEvent(event);
            break;
        }

        case DISPID_COMMANDSTATECHANGE:
        {
            long commandId = evt[0].GetLong();
            bool enable = evt[1].GetBool();
            if (commandId == CSC_NAVIGATEBACK)
            {
                m_can_navigate_back = enable;
            }
            else if (commandId == CSC_NAVIGATEFORWARD)
            {
                m_can_navigate_forward = enable;
            }
        }

		/*
		case DISPID_NEWWINDOW2:
		//case DISPID_NEWWINDOW3:
		{
			wxLogMessage("DISPID_NEWWINDOW2\n");
			wxActiveXEventNativeMSW* nativeParams = evt.GetNativeParameters();
			// Cancel the attempt to open a new window
			*V_BOOLREF(&nativeParams->pDispParams->rgvarg[0]) = VARIANT_TRUE;
		}*/
    }

    evt.Skip();
}

#endif
