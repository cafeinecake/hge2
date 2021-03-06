/*
** Haaf's Game Engine 1.7
** Copyright (C) 2003-2007, Relish Games
** hge.relishgames.com
**
** hgeGUI helper class implementation
*/


#include "..\..\include\hgegui.h"


namespace hge {

HGE *hgeGUI::hge = 0;
HGE *hgeGUIObject::hge = 0;


hgeGUI::hgeGUI()
{
  hge = hgeCreate(HGE_VERSION);

  ctrls = 0;
  ctrlLock = 0;
  ctrlFocus = 0;
  ctrlOver = 0;
  navmode = HGEGUI_NONAVKEYS;
  bLPressed = bLReleased = false;
  bRPressed = bRReleased = false;
  nWheel = 0;
  nEnterLeave = 0;
  sprCursor = 0;
}

hgeGUI::~hgeGUI()
{
  hgeGUIObject *ctrl = ctrls, *nextctrl;

  while (ctrl) {
    nextctrl = ctrl->next;
    delete ctrl;
    ctrl = nextctrl;
  }

  hge->Release();
}

void hgeGUI::AddCtrl(hgeGUIObject *ctrl)
{
  hgeGUIObject *last = ctrls;

  ctrl->gui = this;

  if (!ctrls) {
    ctrls = ctrl;
    ctrl->prev = 0;
    ctrl->next = 0;
  } else {
    while (last->next) {
      last = last->next;
    }

    last->next = ctrl;
    ctrl->prev = last;
    ctrl->next = 0;
  }
}

void hgeGUI::DelCtrl(int id)
{
  hgeGUIObject *ctrl = ctrls;

  while (ctrl) {
    if (ctrl->id == id) {
      if (ctrl->prev) {
        ctrl->prev->next = ctrl->next;
      } else {
        ctrls = ctrl->next;
      }

      if (ctrl->next) {
        ctrl->next->prev = ctrl->prev;
      }

      delete ctrl;
      return;
    }

    ctrl = ctrl->next;
  }
}

hgeGUIObject *hgeGUI::GetCtrl(int id) const
{
  hgeGUIObject *ctrl = ctrls;

  while (ctrl) {
    if (ctrl->id == id) {
      return ctrl;
    }

    ctrl = ctrl->next;
  }

  return NULL;
}

void hgeGUI::MoveCtrl(int id, float x, float y)
{
  hgeGUIObject *ctrl = GetCtrl(id);
  ctrl->rect.move_to(x, y);
  //ctrl->rect.x2 = x + (ctrl->rect.x2 - ctrl->rect.x1);
  //ctrl->rect.y2 = y + (ctrl->rect.y2 - ctrl->rect.y1);
  //ctrl->rect.x1 = x;
  //ctrl->rect.y1 = y;
}

void hgeGUI::ShowCtrl(int id, bool bVisible)
{
  GetCtrl(id)->bVisible = bVisible;
}

void hgeGUI::EnableCtrl(int id, bool bEnabled)
{
  GetCtrl(id)->bEnabled = bEnabled;
}

void hgeGUI::SetNavMode(int mode)
{
  navmode = mode;
}

void hgeGUI::SetCursor(hgeSprite *spr)
{
  sprCursor = spr;
}


void hgeGUI::SetColor(uint32_t color)
{
  hgeGUIObject *ctrl = ctrls;

  while (ctrl) {
    ctrl->SetColor(color);
    ctrl = ctrl->next;
  }
}


void hgeGUI::Reset()
{
  hgeGUIObject *ctrl = ctrls;

  while (ctrl) {
    ctrl->Reset();
    ctrl = ctrl->next;
  }

  ctrlLock = 0;
  ctrlOver = 0;
  ctrlFocus = 0;
}


void hgeGUI::Move(float dx, float dy)
{
  hgeGUIObject *ctrl = ctrls;

  while (ctrl) {
    /*ctrl->rect.x1 += dx;
    ctrl->rect.y1 += dy;
    ctrl->rect.x2 += dx;
    ctrl->rect.y2 += dy;*/
    ctrl->rect.move_by(dx, dy);

    ctrl = ctrl->next;
  }
}


void hgeGUI::SetFocus(int id)
{
  hgeGUIObject *ctrlNewFocus = GetCtrl(id);

  if (ctrlNewFocus == ctrlFocus) {
    return;
  }

  if (!ctrlNewFocus) {
    if (ctrlFocus) {
      ctrlFocus->Focus(false);
    }

    ctrlFocus = 0;
  } else if (!ctrlNewFocus->bStatic && ctrlNewFocus->bVisible && ctrlNewFocus->bEnabled) {
    if (ctrlFocus) {
      ctrlFocus->Focus(false);
    }

    if (ctrlNewFocus) {
      ctrlNewFocus->Focus(true);
    }

    ctrlFocus = ctrlNewFocus;
  }
}

int hgeGUI::GetFocus() const
{
  if (ctrlFocus) {
    return ctrlFocus->id;
  } else {
    return 0;
  }
}

void hgeGUI::Enter()
{
  hgeGUIObject *ctrl = ctrls;

  while (ctrl) {
    ctrl->Enter();
    ctrl = ctrl->next;
  }

  nEnterLeave = 2;
}

void hgeGUI::Leave()
{
  hgeGUIObject *ctrl = ctrls;

  while (ctrl) {
    ctrl->Leave();
    ctrl = ctrl->next;
  }

  ctrlFocus = 0;
  ctrlOver = 0;
  ctrlLock = 0;
  nEnterLeave = 1;
}

void hgeGUI::Render()
{
  hgeGUIObject *ctrl = ctrls;

  while (ctrl) {
    if (ctrl->bVisible) {
      ctrl->Render();
    }

    ctrl = ctrl->next;
  }

  if (hge->Input_IsMouseOver() && sprCursor) {
    sprCursor->Render(m_mousepos.x, m_mousepos.y);
  }
}

int hgeGUI::Update(float dt)
{
  bool bDone;
  hgeGUIObject *ctrl;

// Update the mouse variables

  m_mousepos = hge->Input_GetMousePos();
  bLPressed  = hge->Input_KeyDown(Key::LBUTTON);
  bLReleased = hge->Input_KeyUp(Key::LBUTTON);
  bRPressed  = hge->Input_KeyDown(Key::RBUTTON);
  bRReleased = hge->Input_KeyUp(Key::RBUTTON);
  nWheel = hge->Input_GetMouseWheel();

// Update all controls

  ctrl = ctrls;

  while (ctrl) {
    ctrl->Update(dt);
    ctrl = ctrl->next;
  }

// Handle Enter/Leave

  if (nEnterLeave) {
    ctrl = ctrls;
    bDone = true;

    while (ctrl) {
      if (!ctrl->IsDone()) {
        bDone = false;
        break;
      }

      ctrl = ctrl->next;
    }

    if (!bDone) {
      return 0;
    } else {
      if (nEnterLeave == 1) {
        return -1;
      } else {
        nEnterLeave = 0;
      }
    }
  }

// Handle keys

  auto key = hge->Input_GetKey();

  if (((navmode & HGEGUI_LEFTRIGHT) && key == Key::LEFT) ||
      ((navmode & HGEGUI_UPDOWN) && key == Key::UP)) {
    ctrl = ctrlFocus;

    if (!ctrl) {
      ctrl = ctrls;

      if (!ctrl) {
        return 0;
      }
    }

    do {
      ctrl = ctrl->prev;

      if (!ctrl && ((navmode & HGEGUI_CYCLED) || !ctrlFocus)) {
        ctrl = ctrls;

        while (ctrl->next) {
          ctrl = ctrl->next;
        }
      }

      if (!ctrl || ctrl == ctrlFocus) {
        break;
      }
    } while (ctrl->bStatic == true || ctrl->bVisible == false || ctrl->bEnabled == false);

    if (ctrl && ctrl != ctrlFocus) {
      if (ctrlFocus) {
        ctrlFocus->Focus(false);
      }

      if (ctrl) {
        ctrl->Focus(true);
      }

      ctrlFocus = ctrl;
    }
  } else if (((navmode & HGEGUI_LEFTRIGHT) && key == Key::RIGHT) ||
             ((navmode & HGEGUI_UPDOWN) && key == Key::DOWN)) {
    ctrl = ctrlFocus;

    if (!ctrl) {
      ctrl = ctrls;

      if (!ctrl) {
        return 0;
      }

      while (ctrl->next) {
        ctrl = ctrl->next;
      }
    }

    do {
      ctrl = ctrl->next;

      if (!ctrl && ((navmode & HGEGUI_CYCLED) || !ctrlFocus)) {
        ctrl = ctrls;
      }

      if (!ctrl || ctrl == ctrlFocus) {
        break;
      }
    } while (ctrl->bStatic == true || ctrl->bVisible == false || ctrl->bEnabled == false);

    if (ctrl && ctrl != ctrlFocus) {
      if (ctrlFocus) {
        ctrlFocus->Focus(false);
      }

      if (ctrl) {
        ctrl->Focus(true);
      }

      ctrlFocus = ctrl;
    }
  } else if (ctrlFocus && key != Key::NO_KEY && key != Key::LBUTTON && key != Key::RBUTTON) {
    if (ctrlFocus->KeyClick(key, hge->Input_GetChar())) {
      return ctrlFocus->id;
    }
  }

// Handle mouse

  bool bLDown = hge->Input_GetKeyState(Key::LBUTTON);
  bool bRDown = hge->Input_GetKeyState(Key::RBUTTON);

  if (ctrlLock) {
    ctrl = ctrlLock;

    if (!bLDown && !bRDown) {
      ctrlLock = 0;
    }

    if (ProcessCtrl(ctrl)) {
      return ctrl->id;
    }
  } else {
    // Find last (topmost) control

    ctrl = ctrls;

    if (ctrl)
      while (ctrl->next) {
        ctrl = ctrl->next;
      }

    while (ctrl) {
      if (ctrl->rect.TestPoint(m_mousepos.x, m_mousepos.y) && ctrl->bEnabled) {
        if (ctrlOver != ctrl) {
          if (ctrlOver) {
            ctrlOver->MouseOver(false);
          }

          ctrl->MouseOver(true);
          ctrlOver = ctrl;
        }

        if (ProcessCtrl(ctrl)) {
          return ctrl->id;
        } else {
          return 0;
        }
      }

      ctrl = ctrl->prev;
    }

    if (ctrlOver) {
      ctrlOver->MouseOver(false);
      ctrlOver = 0;
    }

  }

  return 0;
}

bool hgeGUI::ProcessCtrl(hgeGUIObject *ctrl)
{
  bool bResult = false;

  if (bLPressed) {
    ctrlLock = ctrl;
    SetFocus(ctrl->id);
    bResult = bResult || ctrl->MouseLButton(true);
  }

  if (bRPressed) {
    ctrlLock = ctrl;
    SetFocus(ctrl->id);
    bResult = bResult || ctrl->MouseRButton(true);
  }

  if (bLReleased)  {
    bResult = bResult || ctrl->MouseLButton(false);
  }

  if (bRReleased)  {
    bResult = bResult || ctrl->MouseRButton(false);
  }

  if (nWheel)    {
    bResult = bResult || ctrl->MouseWheel(nWheel);
  }

  bResult = bResult || ctrl->MouseMove(m_mousepos.x - ctrl->rect.tl.x, 
                                       m_mousepos.y - ctrl->rect.tl.y);

  return bResult;
}

} // ns hge
