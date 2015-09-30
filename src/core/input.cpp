/*
** Haaf's Game Engine 1.8
** Copyright (C) 2003-2007, Relish Games
** hge.relishgames.com
**
** Core functions implementation: input
*/


#include "hge_impl.h"

namespace hge {

char *KeyNames[] = {
  "?",
  "Left Mouse Button", "Right Mouse Button", "?", "Middle Mouse Button",
  "?", "?", "?", "Backspace", "Tab", "?", "?", "?", "Enter", "?", "?",
  "Shift", "Ctrl", "Alt", "Pause", "Caps Lock", "?", "?", "?", "?", "?", "?",
  "Escape", "?", "?", "?", "?",
  "Space", "Page Up", "Page Down", "End", "Home",
  "Left Arrow", "Up Arrow", "Right Arrow", "Down Arrow",
  "?", "?", "?", "?", "Insert", "Delete", "?",
  "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
  "?", "?", "?", "?", "?", "?", "?",
  "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
  "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
  "Left Win", "Right Win", "Application", "?", "?",
  "NumPad 0", "NumPad 1", "NumPad 2", "NumPad 3", "NumPad 4",
  "NumPad 5", "NumPad 6", "NumPad 7", "NumPad 8", "NumPad 9",
  "Multiply", "Add", "?", "Subtract", "Decimal", "Divide",
  "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
  "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
  "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
  "Num Lock", "Scroll Lock",
  "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
  "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
  "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
  "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
  "Semicolon", "Equals", "Comma", "Minus", "Period", "Slash", "Grave",
  "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
  "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
  "?", "?", "?", "?", "?", "?",
  "Left bracket", "Backslash", "Right bracket", "Apostrophe",
  "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
  "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
  "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
  "?", "?", "?"
};


bool HGE_CALL HGE_Impl::Input_GetEvent(InputEvent *event)
{
  CInputEventList *eptr;

  if (queue) {
    eptr = queue;
    memcpy(event, &eptr->event, sizeof(InputEvent));
    queue = eptr->next;
    delete eptr;
    return true;
  }

  return false;
}

Pointi32 HGE_CALL HGE_Impl::Input_GetMousePos()
{
  return m_mousepos;
}


void HGE_CALL HGE_Impl::Input_SetMousePos(const Pointi32 &pos)
{
  POINT pt;
  pt.x = (long)pos.x;
  pt.y = (long)pos.y;
  ClientToScreen(hwnd, &pt);
  SetCursorPos(pt.x, pt.y);
}

int HGE_CALL HGE_Impl::Input_GetMouseWheel()
{
  return Zpos;
}

bool HGE_CALL HGE_Impl::Input_IsMouseOver()
{
  return bMouseOver;
}

bool HGE_CALL HGE_Impl::Input_GetKeyState(Key key)
{
  return ((GetKeyState((int)key) & 0x8000) != 0);
}

bool HGE_CALL HGE_Impl::Input_KeyDown(Key key)
{
  return (keyz[(int)key] & 1) != 0;
}

bool HGE_CALL HGE_Impl::Input_KeyUp(Key key)
{
  return (keyz[(int)key] & 2) != 0;
}

char *HGE_CALL HGE_Impl::Input_GetKeyName(Key key)
{
  return KeyNames[(int)key];
}

Key HGE_CALL HGE_Impl::Input_GetKey()
{
  return VKey;
}

int HGE_CALL HGE_Impl::Input_GetChar()
{
  return Char;
}


//////// Implementation ////////


void HGE_Impl::_InputInit()
{
  /*POINT pt;
  GetCursorPos(&pt);
  ScreenToClient(hwnd, &pt);
  m_mousepos.set((float)pt.x, (float)pt.y);
  */
  int x, y;
  SDL_GetMouseState(&x, &y);
  m_mousepos.set(x, y);

  memset(&keyz, 0, sizeof(keyz));
}

void HGE_Impl::_UpdateMouse()
{
  /*POINT pt;
  RECT  rc;

  GetCursorPos(&pt);
  GetClientRect(hwnd, &rc);
  MapWindowPoints(hwnd, NULL, (LPPOINT)&rc, 2);

  if (bCaptured || (PtInRect(&rc, pt) && WindowFromPoint(pt) == hwnd)) {
    bMouseOver = true;
  } else {
    bMouseOver = false;
  }
  */
  int x, y;
  SDL_GetMouseState(&x, &y);
  m_mousepos.set(x, y);
  // TODO: bMouseOver if in window
}

void HGE_Impl::_BuildEvent(int type, Key key, int scan, int flags, int x, int y)
{
  CInputEventList *last, *eptr = new CInputEventList;
  unsigned char kbstate[256];
  POINT pt;

  eptr->event.type = type;
  eptr->event.chr = 0;
  pt.x = x;
  pt.y = y;

  GetKeyboardState(kbstate);

  if (type == INPUT_KEYDOWN) {
    if ((flags & HGEINP_REPEAT) == 0) {
      keyz[(int)key] |= 1;
    }

    ToAscii((int)key, scan, kbstate, (unsigned short *)&eptr->event.chr, 0);
  }

  if (type == INPUT_KEYUP) {
    keyz[(int)key] |= 2;
    ToAscii((int)key, scan, kbstate, (unsigned short *)&eptr->event.chr, 0);
  }

  if (type == INPUT_MOUSEWHEEL) {
    eptr->event.key = Key::NO_KEY;
    eptr->event.wheel = key;
    ScreenToClient(hwnd, &pt);
  } else {
    eptr->event.key = key;
    eptr->event.wheel = Key::NO_KEY;
  }

  if (type == INPUT_MBUTTONDOWN) {
    keyz[(int)key] |= 1;
    SetCapture(hwnd);
    bCaptured = true;
  }

  if (type == INPUT_MBUTTONUP) {
    keyz[(int)key] |= 2;
    ReleaseCapture();
    Input_SetMousePos(m_mousepos);
    pt.x = (int)m_mousepos.x;
    pt.y = (int)m_mousepos.y;
    bCaptured = false;
  }

  if (kbstate[VK_SHIFT] & 0x80) {
    flags |= HGEINP_SHIFT;
  }

  if (kbstate[VK_CONTROL] & 0x80) {
    flags |= HGEINP_CTRL;
  }

  if (kbstate[VK_MENU] & 0x80) {
    flags |= HGEINP_ALT;
  }

  if (kbstate[VK_CAPITAL] & 0x1) {
    flags |= HGEINP_CAPSLOCK;
  }

  if (kbstate[VK_SCROLL] & 0x1) {
    flags |= HGEINP_SCROLLLOCK;
  }

  if (kbstate[VK_NUMLOCK] & 0x1) {
    flags |= HGEINP_NUMLOCK;
  }

  eptr->event.flags = flags;

  if (pt.x == -1) {
    eptr->event.x = m_mousepos.x;
    eptr->event.y = m_mousepos.y;
  } else {
    if (pt.x < 0) {
      pt.x = 0;
    }

    if (pt.y < 0) {
      pt.y = 0;
    }

    if (pt.x >= nScreenWidth) {
      pt.x = nScreenWidth - 1;
    }

    if (pt.y >= nScreenHeight) {
      pt.y = nScreenHeight - 1;
    }

    eptr->event.x = (float)pt.x;
    eptr->event.y = (float)pt.y;
  }

  eptr->next = 0;

  if (!queue) {
    queue = eptr;
  } else {
    last = queue;

    while (last->next) {
      last = last->next;
    }

    last->next = eptr;
  }

  if (eptr->event.type == INPUT_KEYDOWN || eptr->event.type == INPUT_MBUTTONDOWN) {
    VKey = eptr->event.key;
    Char = eptr->event.chr;
  } else if (eptr->event.type == INPUT_MOUSEMOVE) {
    m_mousepos.set(eptr->event.x, eptr->event.y);
  } else if (eptr->event.type == INPUT_MOUSEWHEEL) {
    Zpos += (int)eptr->event.wheel;
  }
}

void HGE_Impl::_ClearQueue()
{
  CInputEventList *nexteptr, *eptr = queue;

  memset(&keyz, 0, sizeof(keyz));

  while (eptr) {
    nexteptr = eptr->next;
    delete eptr;
    eptr = nexteptr;
  }

  queue = 0;
  VKey = Key::NO_KEY;
  Char = 0;
  Zpos = 0;
}


} // ns hge
