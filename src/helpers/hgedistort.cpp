/*
** Haaf's Game Engine 1.7
** Copyright (C) 2003-2007, Relish Games
** hge.relishgames.com
**
** hgeDistortionMesh helper class implementation
*/

#include "..\..\include\hgedistort.h"

namespace hge {

HGE *hgeDistortionMesh::hge = 0;


hgeDistortionMesh::hgeDistortionMesh(int cols, int rows)
{
  int i;

  hge = hgeCreate(HGE_VERSION);

  nRows = rows;
  nCols = cols;
  cellw = cellh = 0;
  quad.tex = 0;
  quad.blend = BLEND_COLORMUL | BLEND_ALPHABLEND | BLEND_ZWRITE;
  disp_array = new Vertex[rows * cols];

  for (i = 0; i < rows * cols; i++) {
    disp_array[i].pos.x = 0.0f;
    disp_array[i].pos.y = 0.0f;
    disp_array[i].tex.x = 0.0f;
    disp_array[i].tex.y = 0.0f;

    disp_array[i].z = 0.5f;
    disp_array[i].col = 0xFFFFFFFF;
  }
}

hgeDistortionMesh::hgeDistortionMesh(const hgeDistortionMesh &dm)
{
  hge = hgeCreate(HGE_VERSION);

  nRows = dm.nRows;
  nCols = dm.nCols;
  cellw = dm.cellw;
  cellh = dm.cellh;
  tx = dm.tx;
  ty = dm.ty;
  width = dm.width;
  height = dm.height;
  quad = dm.quad;

  disp_array = new Vertex[nRows * nCols];
  memcpy(disp_array, dm.disp_array, sizeof(Vertex)*nRows * nCols);
}

hgeDistortionMesh::~hgeDistortionMesh()
{
  delete[] disp_array;
  hge->Release();
}

hgeDistortionMesh &hgeDistortionMesh::operator= (const hgeDistortionMesh &dm)
{
  if (this != &dm) {
    nRows = dm.nRows;
    nCols = dm.nCols;
    cellw = dm.cellw;
    cellh = dm.cellh;
    tx = dm.tx;
    ty = dm.ty;
    width = dm.width;
    height = dm.height;
    quad = dm.quad;

    delete[] disp_array;
    disp_array = new Vertex[nRows * nCols];
    memcpy(disp_array, dm.disp_array, sizeof(Vertex)*nRows * nCols);
  }

  return *this;

}

void hgeDistortionMesh::SetTexture(HTEXTURE tex)
{
  quad.tex = tex;
}

void hgeDistortionMesh::SetTextureRect(float x, float y, float w, float h)
{
  int i, j;
  float tw, th;

  tx = x;
  ty = y;
  width = w;
  height = h;

  if (quad.tex) {
    tw = (float)hge->Texture_GetWidth(quad.tex);
    th = (float)hge->Texture_GetHeight(quad.tex);
  } else {
    tw = w;
    th = h;
  }

  cellw = w / (nCols - 1);
  cellh = h / (nRows - 1);

  for (j = 0; j < nRows; j++)
    for (i = 0; i < nCols; i++) {
      disp_array[j * nCols + i].tex.x = (x + i * cellw) / tw;
      disp_array[j * nCols + i].tex.y = (y + j * cellh) / th;

      disp_array[j * nCols + i].pos.x = i * cellw;
      disp_array[j * nCols + i].pos.y = j * cellh;
    }
}

void hgeDistortionMesh::SetBlendMode(int blend)
{
  quad.blend = blend;
}

void hgeDistortionMesh::Clear(uint32_t col, float z)
{
  int i, j;

  for (j = 0; j < nRows; j++)
    for (i = 0; i < nCols; i++) {
      disp_array[j * nCols + i].pos.x = i * cellw;
      disp_array[j * nCols + i].pos.y = j * cellh;
      disp_array[j * nCols + i].col = col;
      disp_array[j * nCols + i].z = z;
    }
}

void hgeDistortionMesh::Render(float x, float y)
{
  int i, j, idx;

  for (j = 0; j < nRows - 1; j++)
    for (i = 0; i < nCols - 1; i++) {
      idx = j * nCols + i;

      // TODO: memcpy here?
      quad.v[0].tex.x = disp_array[idx].tex.x;
      quad.v[0].tex.y = disp_array[idx].tex.y;
      quad.v[0].pos.x = x + disp_array[idx].pos.x;
      quad.v[0].pos.y = y + disp_array[idx].pos.y;
      quad.v[0].z = disp_array[idx].z;
      quad.v[0].col = disp_array[idx].col;

      quad.v[1].tex.x = disp_array[idx + 1].tex.x;
      quad.v[1].tex.y = disp_array[idx + 1].tex.y;
      quad.v[1].pos.x = x + disp_array[idx + 1].pos.x;
      quad.v[1].pos.y = y + disp_array[idx + 1].pos.y;
      quad.v[1].z = disp_array[idx + 1].z;
      quad.v[1].col = disp_array[idx + 1].col;

      quad.v[2].tex.x = disp_array[idx + nCols + 1].tex.x;
      quad.v[2].tex.y = disp_array[idx + nCols + 1].tex.y;
      quad.v[2].pos.x = x + disp_array[idx + nCols + 1].pos.x;
      quad.v[2].pos.y = y + disp_array[idx + nCols + 1].pos.y;
      quad.v[2].z = disp_array[idx + nCols + 1].z;
      quad.v[2].col = disp_array[idx + nCols + 1].col;

      quad.v[3].tex.x = disp_array[idx + nCols].tex.x;
      quad.v[3].tex.y = disp_array[idx + nCols].tex.y;
      quad.v[3].pos.x = x + disp_array[idx + nCols].pos.x;
      quad.v[3].pos.y = y + disp_array[idx + nCols].pos.y;
      quad.v[3].z = disp_array[idx + nCols].z;
      quad.v[3].col = disp_array[idx + nCols].col;

      hge->Gfx_RenderQuad(&quad);
    }
}

void hgeDistortionMesh::SetZ(int col, int row, float z)
{
  if (row < nRows && col < nCols) {
    disp_array[row * nCols + col].z = z;
  }
}

void hgeDistortionMesh::SetColor(int col, int row, uint32_t color)
{
  if (row < nRows && col < nCols) {
    disp_array[row * nCols + col].col = color;
  }
}

void hgeDistortionMesh::SetDisplacement(int col, int row, float dx, float dy, int ref)
{
  if (row < nRows && col < nCols) {
    switch (ref) {
    case HGEDISP_NODE:
      dx += col * cellw;
      dy += row * cellh;
      break;

    case HGEDISP_CENTER:
      dx += cellw * (nCols - 1) / 2;
      dy += cellh * (nRows - 1) / 2;
      break;

    case HGEDISP_TOPLEFT:
      break;
    }

    disp_array[row * nCols + col].pos.x = dx;
    disp_array[row * nCols + col].pos.y = dy;
  }
}

float hgeDistortionMesh::GetZ(int col, int row) const
{
  if (row < nRows && col < nCols) {
    return disp_array[row * nCols + col].z;
  } else {
    return 0.0f;
  }
}

uint32_t hgeDistortionMesh::GetColor(int col, int row) const
{
  if (row < nRows && col < nCols) {
    return disp_array[row * nCols + col].col;
  } else {
    return 0;
  }
}

void hgeDistortionMesh::GetDisplacement(int col, int row, float *dx, float *dy, int ref) const
{
  if (row < nRows && col < nCols) {
    switch (ref) {
    case HGEDISP_NODE:
      *dx = disp_array[row * nCols + col].pos.x - col * cellw;
      *dy = disp_array[row * nCols + col].pos.y - row * cellh;
      break;

    case HGEDISP_CENTER:
      *dx = disp_array[row * nCols + col].pos.x - cellw * (nCols - 1) / 2;
      *dy = disp_array[row * nCols + col].pos.x - cellh * (nRows - 1) / 2;
      break;

    case HGEDISP_TOPLEFT:
      *dx = disp_array[row * nCols + col].pos.x;
      *dy = disp_array[row * nCols + col].pos.y;
      break;
    }
  }
}

} // ns hge
