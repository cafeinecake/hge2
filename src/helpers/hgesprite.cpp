/*
** Haaf's Game Engine 1.7
** Copyright (C) 2003-2007, Relish Games
** hge.relishgames.com
**
** hgeSprite helper class implementation
*/


#include "..\..\include\hgesprite.h"
#include <math.h>

namespace hge {

HGE *hgeSprite::hge = 0;


hgeSprite::hgeSprite(HTEXTURE texture, float texx, float texy, float w, float h)
{
  float texx1, texy1, texx2, texy2;

  hge = hgeCreate(HGE_VERSION);

  tx = texx;
  ty = texy;
  width = w;
  height = h;

  if (texture) {
    tex_width = (float)hge->Texture_GetWidth(texture);
    tex_height = (float)hge->Texture_GetHeight(texture);
  } else {
    tex_width = 1.0f;
    tex_height = 1.0f;
  }

  hotX = 0;
  hotY = 0;
  bXFlip = false;
  bYFlip = false;
  bHSFlip = false;
  quad.tex = texture;

  texx1 = texx / tex_width;
  texy1 = texy / tex_height;
  texx2 = (texx + w) / tex_width;
  texy2 = (texy + h) / tex_height;

  quad.v[0].tex.x = texx1;
  quad.v[0].tex.y = texy1;
  quad.v[1].tex.x = texx2;
  quad.v[1].tex.y = texy1;
  quad.v[2].tex.x = texx2;
  quad.v[2].tex.y = texy2;
  quad.v[3].tex.x = texx1;
  quad.v[3].tex.y = texy2;

  quad.v[0].z =
    quad.v[1].z =
      quad.v[2].z =
        quad.v[3].z = 0.5f;

  quad.v[0].col =
    quad.v[1].col =
      quad.v[2].col =
        quad.v[3].col = 0xffffffff;

  quad.blend = BLEND_DEFAULT;
}

hgeSprite::hgeSprite(const hgeSprite &spr)
{
  memcpy(this, &spr, sizeof(hgeSprite));
  hge = hgeCreate(HGE_VERSION);
}

void hgeSprite::Render(float x, float y)
{
  float tempx1, tempy1, tempx2, tempy2;

  tempx1 = x - hotX;
  tempy1 = y - hotY;
  tempx2 = x + width - hotX;
  tempy2 = y + height - hotY;

  quad.v[0].pos.x = tempx1;
  quad.v[0].pos.y = tempy1;
  quad.v[1].pos.x = tempx2;
  quad.v[1].pos.y = tempy1;
  quad.v[2].pos.x = tempx2;
  quad.v[2].pos.y = tempy2;
  quad.v[3].pos.x = tempx1;
  quad.v[3].pos.y = tempy2;

  hge->Gfx_RenderQuad(&quad);
}


void hgeSprite::RenderEx(float x, float y, float rot, float hscale, float vscale)
{
  float tx1, ty1, tx2, ty2;
  float sint, cost;

  if (vscale == 0) {
    vscale = hscale;
  }

  tx1 = -hotX * hscale;
  ty1 = -hotY * vscale;
  tx2 = (width - hotX) * hscale;
  ty2 = (height - hotY) * vscale;

  if (rot != 0.0f) {
    cost = cosf(rot);
    sint = sinf(rot);

    quad.v[0].pos.x  = tx1 * cost - ty1 * sint + x;
    quad.v[0].pos.y  = tx1 * sint + ty1 * cost + y;

    quad.v[1].pos.x  = tx2 * cost - ty1 * sint + x;
    quad.v[1].pos.y  = tx2 * sint + ty1 * cost + y;

    quad.v[2].pos.x  = tx2 * cost - ty2 * sint + x;
    quad.v[2].pos.y  = tx2 * sint + ty2 * cost + y;

    quad.v[3].pos.x  = tx1 * cost - ty2 * sint + x;
    quad.v[3].pos.y  = tx1 * sint + ty2 * cost + y;
  } else {
    quad.v[0].pos.x = tx1 + x;
    quad.v[0].pos.y = ty1 + y;
    quad.v[1].pos.x = tx2 + x;
    quad.v[1].pos.y = ty1 + y;
    quad.v[2].pos.x = tx2 + x;
    quad.v[2].pos.y = ty2 + y;
    quad.v[3].pos.x = tx1 + x;
    quad.v[3].pos.y = ty2 + y;
  }

  hge->Gfx_RenderQuad(&quad);
}


void hgeSprite::RenderStretch(float x1, float y1, float x2, float y2)
{
  quad.v[0].pos.x = x1;
  quad.v[0].pos.y = y1;
  quad.v[1].pos.x = x2;
  quad.v[1].pos.y = y1;
  quad.v[2].pos.x = x2;
  quad.v[2].pos.y = y2;
  quad.v[3].pos.x = x1;
  quad.v[3].pos.y = y2;

  hge->Gfx_RenderQuad(&quad);
}


void hgeSprite::Render4V(float x0, float y0, float x1, float y1, float x2, float y2, float x3,
                         float y3)
{
  quad.v[0].pos.x = x0;
  quad.v[0].pos.y = y0;
  quad.v[1].pos.x = x1;
  quad.v[1].pos.y = y1;
  quad.v[2].pos.x = x2;
  quad.v[2].pos.y = y2;
  quad.v[3].pos.x = x3;
  quad.v[3].pos.y = y3;

  hge->Gfx_RenderQuad(&quad);
}


hgeRect *hgeSprite::GetBoundingBoxEx(float x, float y, float rot, float hscale, float vscale,
                                     hgeRect *rect) const
{
  float tx1, ty1, tx2, ty2;
  float sint, cost;

  rect->Clear();

  tx1 = -hotX * hscale;
  ty1 = -hotY * vscale;
  tx2 = (width - hotX) * hscale;
  ty2 = (height - hotY) * vscale;

  if (rot != 0.0f) {
    cost = cosf(rot);
    sint = sinf(rot);

    rect->Encapsulate(tx1 * cost - ty1 * sint + x, tx1 * sint + ty1 * cost + y);
    rect->Encapsulate(tx2 * cost - ty1 * sint + x, tx2 * sint + ty1 * cost + y);
    rect->Encapsulate(tx2 * cost - ty2 * sint + x, tx2 * sint + ty2 * cost + y);
    rect->Encapsulate(tx1 * cost - ty2 * sint + x, tx1 * sint + ty2 * cost + y);
  } else {
    rect->Encapsulate(tx1 + x, ty1 + y);
    rect->Encapsulate(tx2 + x, ty1 + y);
    rect->Encapsulate(tx2 + x, ty2 + y);
    rect->Encapsulate(tx1 + x, ty2 + y);
  }

  return rect;
}

void hgeSprite::SetFlip(bool bX, bool bY, bool bHotSpot)
{
  float tx, ty;

  if (bHSFlip && bXFlip) {
    hotX = width - hotX;
  }

  if (bHSFlip && bYFlip) {
    hotY = height - hotY;
  }

  bHSFlip = bHotSpot;

  if (bHSFlip && bXFlip) {
    hotX = width - hotX;
  }

  if (bHSFlip && bYFlip) {
    hotY = height - hotY;
  }

  if (bX != bXFlip) {
    tx = quad.v[0].tex.x;
    quad.v[0].tex.x = quad.v[1].tex.x;
    quad.v[1].tex.x = tx;
    ty = quad.v[0].tex.y;
    quad.v[0].tex.y = quad.v[1].tex.y;
    quad.v[1].tex.y = ty;
    tx = quad.v[3].tex.x;
    quad.v[3].tex.x = quad.v[2].tex.x;
    quad.v[2].tex.x = tx;
    ty = quad.v[3].tex.y;
    quad.v[3].tex.y = quad.v[2].tex.y;
    quad.v[2].tex.y = ty;

    bXFlip = !bXFlip;
  }

  if (bY != bYFlip) {
    tx = quad.v[0].tex.x;
    quad.v[0].tex.x = quad.v[3].tex.x;
    quad.v[3].tex.x = tx;
    ty = quad.v[0].tex.y;
    quad.v[0].tex.y = quad.v[3].tex.y;
    quad.v[3].tex.y = ty;
    tx = quad.v[1].tex.x;
    quad.v[1].tex.x = quad.v[2].tex.x;
    quad.v[2].tex.x = tx;
    ty = quad.v[1].tex.y;
    quad.v[1].tex.y = quad.v[2].tex.y;
    quad.v[2].tex.y = ty;

    bYFlip = !bYFlip;
  }
}


void hgeSprite::SetTexture(HTEXTURE tex)
{
  float tx1, ty1, tx2, ty2;
  float tw, th;

  quad.tex = tex;

  if (tex) {
    tw = (float)hge->Texture_GetWidth(tex);
    th = (float)hge->Texture_GetHeight(tex);
  } else {
    tw = 1.0f;
    th = 1.0f;
  }

  if (tw != tex_width || th != tex_height) {
    tx1 = quad.v[0].tex.x * tex_width;
    ty1 = quad.v[0].tex.y * tex_height;
    tx2 = quad.v[2].tex.x * tex_width;
    ty2 = quad.v[2].tex.y * tex_height;

    tex_width = tw;
    tex_height = th;

    tx1 /= tw;
    ty1 /= th;
    tx2 /= tw;
    ty2 /= th;

    quad.v[0].tex.x = tx1;
    quad.v[0].tex.y = ty1;
    quad.v[1].tex.x = tx2;
    quad.v[1].tex.y = ty1;
    quad.v[2].tex.x = tx2;
    quad.v[2].tex.y = ty2;
    quad.v[3].tex.x = tx1;
    quad.v[3].tex.y = ty2;
  }
}


void hgeSprite::SetTextureRect(float x, float y, float w, float h, bool adjSize)
{
  float tx1, ty1, tx2, ty2;
  bool bX, bY, bHS;

  tx = x;
  ty = y;

  if (adjSize) {
    width = w;
    height = h;
  }

  tx1 = tx / tex_width;
  ty1 = ty / tex_height;
  tx2 = (tx + w) / tex_width;
  ty2 = (ty + h) / tex_height;

  quad.v[0].tex.x = tx1;
  quad.v[0].tex.y = ty1;
  quad.v[1].tex.x = tx2;
  quad.v[1].tex.y = ty1;
  quad.v[2].tex.x = tx2;
  quad.v[2].tex.y = ty2;
  quad.v[3].tex.x = tx1;
  quad.v[3].tex.y = ty2;

  bX = bXFlip;
  bY = bYFlip;
  bHS = bHSFlip;
  bXFlip = false;
  bYFlip = false;
  SetFlip(bX, bY, bHS);
}


void hgeSprite::SetColor(uint32_t col, int i)
{
  if (i != -1) {
    quad.v[i].col = col;
  } else {
    quad.v[0].col = quad.v[1].col = quad.v[2].col = quad.v[3].col = col;
  }
}

void hgeSprite::SetZ(float z, int i)
{
  if (i != -1) {
    quad.v[i].z = z;
  } else {
    quad.v[0].z = quad.v[1].z = quad.v[2].z = quad.v[3].z = z;
  }
}

} // ns hge
