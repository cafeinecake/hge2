/*
** Haaf's Game Engine 1.7
** Copyright (C) 2003-2007, Relish Games
** hge.relishgames.com
**
** hgeGUI default controls implementation
*/


#include "..\..\include\hgeguictrls.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

namespace hge {

/*
** hgeGUIText
*/

hgeGUIText::hgeGUIText(int _id, float x, float y, float w, float h, hgeFont *fnt)
{
  id = _id;
  bStatic = true;
  bVisible = true;
  bEnabled = true;
  rect.Set(x, y, x + w, y + h);

  font = fnt;
  tx = x;
  ty = y + (h - fnt->GetHeight()) / 2.0f;

  text[0] = 0;
}

void hgeGUIText::SetMode(int _align)
{
  align = _align;

  if (align == HGETEXT_RIGHT) {
    tx = rect.br.x;
  } else if (align == HGETEXT_CENTER) {
    tx = (rect.tl.x + rect.tl.x) / 2.0f;
  } else {
    tx = rect.tl.x;
  }
}

void hgeGUIText::SetText(const char *_text)
{
  strcpy(text, _text);
}

void hgeGUIText::printf(const char *format, ...)
{
  vsprintf(text, format, (char *)&format + sizeof(format));
}

void hgeGUIText::Render()
{
  font->SetColor(color);
  font->Render(tx, ty, align, text);
}

/*
** hgeGUIButton
*/

hgeGUIButton::hgeGUIButton(int _id, float x, float y, float w, float h, HTEXTURE tex, float tx,
                           float ty)
{
  id = _id;
  bStatic = false;
  bVisible = true;
  bEnabled = true;
  rect.Set(x, y, x + w, y + h);

  bPressed = false;
  bTrigger = false;

  sprUp = new hgeSprite(tex, tx, ty, w, h);
  sprDown = new hgeSprite(tex, tx + w, ty, w, h);
}

hgeGUIButton::~hgeGUIButton()
{
  if (sprUp) {
    delete sprUp;
  }

  if (sprDown) {
    delete sprDown;
  }
}

void hgeGUIButton::Render()
{
  if (bPressed) {
    sprDown->Render(rect.tl);
  } else {
    sprUp->Render(rect.tl);
  }
}

bool hgeGUIButton::MouseLButton(bool bDown)
{
  if (bDown) {
    bOldState = bPressed;
    bPressed = true;
    return false;
  } else {
    if (bTrigger) {
      bPressed = !bOldState;
    } else {
      bPressed = false;
    }

    return true;
  }
}

/*
** hgeGUISlider
*/

hgeGUISlider::hgeGUISlider(int _id, float x, float y, float w, float h, HTEXTURE tex, float tx,
                           float ty, float sw, float sh, bool vertical)
{
  id = _id;
  bStatic = false;
  bVisible = true;
  bEnabled = true;
  bPressed = false;
  bVertical = vertical;
  rect.Set(x, y, x + w, y + h);

  mode = HGESLIDER_BAR;
  fMin = 0;
  fMax = 100;
  fVal = 50;
  sl_w = sw;
  sl_h = sh;

  sprSlider = new hgeSprite(tex, tx, ty, sw, sh);
}

hgeGUISlider::~hgeGUISlider()
{
  if (sprSlider) {
    delete sprSlider;
  }
}

void hgeGUISlider::SetValue(float _fVal)
{
  if (_fVal < fMin) {
    fVal = fMin;
  } else if (_fVal > fMax) {
    fVal = fMax;
  } else {
    fVal = _fVal;
  }
}

void hgeGUISlider::Render()
{
  float xx, yy;
  float x1, y1, x2, y2;

  xx = rect.tl.x + rect.width() * (fVal - fMin) / (fMax - fMin);
  yy = rect.tl.y + rect.height() * (fVal - fMin) / (fMax - fMin);

  if (bVertical)
    switch (mode) {
    case HGESLIDER_BAR:
      x1 = rect.tl.x;
      y1 = rect.tl.y;
      x2 = rect.br.x;
      y2 = yy;
      break;

    case HGESLIDER_BARRELATIVE:
      x1 = rect.tl.x;
      y1 = (rect.tl.y + rect.br.y) / 2;
      x2 = rect.br.x;
      y2 = yy;
      break;

    case HGESLIDER_SLIDER:
      x1 = (rect.tl.x + rect.br.x - sl_w) / 2;
      y1 = yy - sl_h / 2;
      x2 = (rect.tl.x + rect.br.x + sl_w) / 2;
      y2 = yy + sl_h / 2;
      break;
    }
  else
    switch (mode) {
    case HGESLIDER_BAR:
      x1 = rect.tl.x;
      y1 = rect.tl.y;
      x2 = xx;
      y2 = rect.br.y;
      break;

    case HGESLIDER_BARRELATIVE:
      x1 = (rect.tl.x + rect.br.x) / 2;
      y1 = rect.tl.y;
      x2 = xx;
      y2 = rect.br.y;
      break;

    case HGESLIDER_SLIDER:
      x1 = xx - sl_w / 2;
      y1 = (rect.tl.y + rect.br.y - sl_h) / 2;
      x2 = xx + sl_w / 2;
      y2 = (rect.tl.y + rect.br.y + sl_h) / 2;
      break;
    }

  sprSlider->RenderStretch(x1, y1, x2, y2);
}

bool hgeGUISlider::MouseLButton(bool bDown)
{
  bPressed = bDown;
  return false;
}

bool hgeGUISlider::MouseMove(float x, float y)
{
  if (bPressed) {
    if (bVertical) {
      if (y > rect.height()) {
        y = rect.height();
      }

      if (y < 0) {
        y = 0;
      }

      fVal = fMin + (fMax - fMin) * y / rect.height();
    } else {
      if (x > rect.width()) {
        x = rect.width();
      }

      if (x < 0) {
        x = 0;
      }

      fVal = fMin + (fMax - fMin) * x / rect.width();
    }

    return true;
  }

  return false;
}


/*
** hgeGUIListbox
*/

hgeGUIListbox::hgeGUIListbox(int _id, float x, float y, float w, float h, hgeFont *fnt,
                             uint32_t tColor, uint32_t thColor, uint32_t hColor)
{
  id = _id;
  bStatic = false;
  bVisible = true;
  bEnabled = true;
  rect.Set(x, y, x + w, y + h);
  font = fnt;
  sprHighlight = new hgeSprite(0, 0, 0, w, fnt->GetHeight());
  sprHighlight->SetColor(hColor);
  textColor = tColor;
  texthilColor = thColor;
  pItems = 0;
  nItems = 0;

  nSelectedItem = 0;
  nTopItem = 0;
  mx = 0;
  my = 0;
}

hgeGUIListbox::~hgeGUIListbox()
{
  Clear();

  if (sprHighlight) {
    delete sprHighlight;
  }
}


int hgeGUIListbox::AddItem(char *item)
{
  hgeGUIListboxItem *pItem = pItems, *pPrev = 0, *pNew;

  pNew = new hgeGUIListboxItem;
  memcpy(pNew->text, item, min(sizeof(pNew->text), strlen(item) + 1));
  pNew->text[sizeof(pNew->text) - 1] = '\0';
  pNew->next = 0;

  while (pItem) {
    pPrev = pItem;
    pItem = pItem->next;
  }

  if (pPrev) {
    pPrev->next = pNew;
  } else {
    pItems = pNew;
  }

  nItems++;

  return nItems - 1;
}

void hgeGUIListbox::DeleteItem(int n)
{
  int i;
  hgeGUIListboxItem *pItem = pItems, *pPrev = 0;

  if (n < 0 || n >= GetNumItems()) {
    return;
  }

  for (i = 0; i < n; i++) {
    pPrev = pItem;
    pItem = pItem->next;
  }

  if (pPrev) {
    pPrev->next = pItem->next;
  } else {
    pItems = pItem->next;
  }

  delete pItem;
  nItems--;
}

char *hgeGUIListbox::GetItemText(int n)
{
  int i;
  hgeGUIListboxItem *pItem = pItems;

  if (n < 0 || n >= GetNumItems()) {
    return 0;
  }

  for (i = 0; i < n; i++) {
    pItem = pItem->next;
  }

  return pItem->text;
}

void hgeGUIListbox::Clear()
{
  hgeGUIListboxItem *pItem = pItems, *pNext;

  while (pItem) {
    pNext = pItem->next;
    delete pItem;
    pItem = pNext;
  }

  pItems = 0;
  nItems = 0;
}

void hgeGUIListbox::Render()
{
  int i;
  hgeGUIListboxItem *pItem = pItems;

  for (i = 0; i < nTopItem; i++) {
    pItem = pItem->next;
  }

  for (i = 0; i < GetNumRows(); i++) {
    if (i >= nItems) {
      return;
    }

    if (nTopItem + i == nSelectedItem) {
      sprHighlight->Render(rect.tl.x, rect.tl.y + i * font->GetHeight());
      font->SetColor(texthilColor);
    } else {
      font->SetColor(textColor);
    }

    font->Render(rect.tl.x + 3, rect.tl.y + i * font->GetHeight(), HGETEXT_LEFT, pItem->text);
    pItem = pItem->next;
  }
}

bool hgeGUIListbox::MouseLButton(bool bDown)
{
  int nItem;

  if (bDown) {
    nItem = nTopItem + int(my) / int(font->GetHeight());

    if (nItem < nItems) {
      nSelectedItem = nItem;
      return true;
    }
  }

  return false;
}


bool hgeGUIListbox::MouseWheel(int nNotches)
{
  nTopItem -= nNotches;

  if (nTopItem < 0) {
    nTopItem = 0;
  }

  if (nTopItem > GetNumItems() - GetNumRows()) {
    nTopItem = GetNumItems() - GetNumRows();
  }

  return true;
}

bool hgeGUIListbox::KeyClick(Key key, int chr)
{
  switch (key) {
  case Key::DOWN:
    if (nSelectedItem < nItems - 1) {
      nSelectedItem++;

      if (nSelectedItem > nTopItem + GetNumRows() - 1) {
        nTopItem = nSelectedItem - GetNumRows() + 1;
      }

      return true;
    }

    break;

  case Key::UP:
    if (nSelectedItem > 0) {
      nSelectedItem--;

      if (nSelectedItem < nTopItem) {
        nTopItem = nSelectedItem;
      }

      return true;
    }

    break;
  }

  return false;
}

} // ns hge
