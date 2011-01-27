// WX related stuff

#ifndef __WXS_H__
#define __WXS_H__

#include <wx/wx.h>

BEGIN_DECLARE_EVENT_TYPES()
  DECLARE_LOCAL_EVENT_TYPE( wxEVT_TEXTCTRL_DATA, 1 )
END_DECLARE_EVENT_TYPES()

#define EVT_TEXTCTRL_DATA( id, fn ) \
  DECLARE_EVENT_TABLE_ENTRY( \
      wxEVT_TEXTCTRL_DATA, id, wxID_ANY, \
      ( wxObjectEventFunction )( wxEventFunction ) wxStaticCastEvent( wxCommandEventFunction, &fn ), \
      ( wxObject * ) NULL \
  ),

#endif
