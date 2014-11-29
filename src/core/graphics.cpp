/*
** Haaf's Game Engine 1.8
** Copyright (C) 2003-2007, Relish Games
** hge.relishgames.com
**
** Core functions implementation: graphics
*/

// !!! FIXME: the texture data when locking/unlocking textures is in GL_BGRA format, not GL_RGBA.
// !!! FIXME:  ...but this mistake wasn't noticed for several games, since most didn't lock outside
// !!! FIXME:  of a piece of code that was #ifdef'd for Unix anyhow.
// !!! FIXME: But if you lock textures and the colors are wrong, that's what happened. We need to
// !!! FIXME:  sort out all the places where we're passing things around in RGBA to fix this.
// !!! FIXME:  In the mean time, it's usually easier to just change your application to expect
// !!! FIXME:  locked textures to be RGBA instead of BGRA.

// !!! FIXME: ...apparently we're locking textures upside down, too?

#include "hge_impl.h"

#include <algorithm>

//#define SUPPORT_CXIMAGE 1
#if SUPPORT_CXIMAGE
// conflict with Mac OS X 10.3.9 SDK...
#ifdef _T
#undef _T
#endif
#include <cximage/ximage.h>
#else
/* Use DevIL instead of CXImage */
#include <IL/il.h>
#include <IL/ilu.h>
#endif

// avoiding glext.h here ...
#ifndef GL_TEXTURE_RECTANGLE_ARB
#define GL_TEXTURE_RECTANGLE_ARB 0x84F5
#endif
#ifndef GL_FRAMEBUFFER_EXT
#define GL_FRAMEBUFFER_EXT 0x8D40
#endif
#ifndef GL_RENDERBUFFER_EXT
#define GL_RENDERBUFFER_EXT 0x8D41
#endif
#ifndef GL_COLOR_ATTACHMENT0_EXT
#define GL_COLOR_ATTACHMENT0_EXT 0x8CE0
#endif
#ifndef GL_DEPTH_ATTACHMENT_EXT
#define GL_DEPTH_ATTACHMENT_EXT 0x8D00
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE_EXT
#define GL_FRAMEBUFFER_COMPLETE_EXT 0x8CD5
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif
#ifndef GL_YCBCR_422_APPLE
#define GL_YCBCR_422_APPLE 0x85B9
#endif
#ifndef GL_UNSIGNED_SHORT_8_8_APPLE
#define GL_UNSIGNED_SHORT_8_8_APPLE 0x85BA
#endif
#ifndef GL_UNSIGNED_SHORT_8_8_REV_APPLE
//#define GL_UNSIGNED_SHORT_8_8_REV_APPLE 0x85BB
#endif

struct gltexture {
  GLuint name;
  GLuint width;
  GLuint height;
  GLuint potw;  // Power-of-two width.
  GLuint poth;  // Power-of-two height.
  const char *filename;  // if backed by a file, not a managed buffer.
  uint32_t *pixels;  // original rgba data.
  uint32_t *lock_pixels;  // for locked texture
  bool is_render_target;
  bool lost;
  bool lock_readonly;
  GLint lock_x;
  GLint lock_y;
  GLint lock_width;
  GLint lock_height;
};

static uint32_t *_DecodeImage(uint8_t *data, const char *fname, uint32_t size, int &width,
                              int &height)
{
  width = height = 0;

  uint32_t *pixels = nullptr;
  const size_t fnamelen = fname ? strlen(fname) : 0;

  if ((fnamelen > 5) && (strcasecmp((fname + fnamelen) - 5, ".rgba") == 0)) {
    uint32_t *ptr = reinterpret_cast<uint32_t *>(data);
    uint32_t w = ptr[0];
    uint32_t h = ptr[1];
    BYTESWAP(w);
    BYTESWAP(h);

    if (((w * h * 4) + 8) == size) {   // not truncated?
      width = static_cast<int>(w);
      height = static_cast<int>(h);
      pixels = new uint32_t[width * height];
      memcpy(pixels, ptr + 2, w * h * 4);  // !!! FIXME: ignores pitch.
    }

    return pixels;
  }

#if SUPPORT_CXIMAGE
  CxImage img;
  img.Decode(data, size, CXIMAGE_FORMAT_UNKNOWN);

  if (img.IsValid()) {
    width = static_cast<int>(img.GetWidth());
    height = static_cast<int>(img.GetHeight());
    pixels = new uint32_t[width * height];
    uint8_t *wptr = reinterpret_cast<uint8_t *>(pixels);
    const bool hasalpha = img.AlphaIsValid();

    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        const RGBQUAD rgb = img.GetPixelColor(x, y, true);
        *(wptr++) = rgb.rgbRed;
        *(wptr++) = rgb.rgbGreen;
        *(wptr++) = rgb.rgbBlue;
        *(wptr++) = hasalpha ? rgb.rgbReserved : 0xFF;  // alpha.
      }
    }
  }

#else
  ilInit();
  iluInit();

  ILuint id;
  ilGenImages(1, &id);

  if (ilLoadImage(fname)) {
    printf("success: %s\n", fname);
    ILinfo info;
    iluGetImageInfo(&info);
    width = info.Width;
    height = info.Height;
    size = info.SizeOfData;
    pixels = new uint32_t[width * height];
    ilCopyPixels(0, 0, 0, width, height, 0, IL_RGBA, IL_UNSIGNED_INT, pixels);
    ilShutDown();
  }

#endif

  return pixels;
}


void HGE_Impl::_BindTexture(gltexture *t)
{
  // The Direct3D renderer is using managed textures, so they aren't every
  //  actually "lost" ... we may have to rebuild them here, though.
  if ((t != nullptr) && (t->lost)) {
    _ConfigureTexture(t,
                      static_cast<int>(t->width),
                      static_cast<int>(t->height),
                      t->pixels);
  }

  if ((reinterpret_cast<HTEXTURE>(t)) != CurTexture) {
    pOpenGLDevice->glBindTexture(pOpenGLDevice->TextureTarget, t ? t->name : 0);
    CurTexture = reinterpret_cast<HTEXTURE>(t);
  }
}

void CALL HGE_Impl::Gfx_Clear(uint32_t color)
{
  GLbitfield flags = GL_COLOR_BUFFER_BIT;

  if (((pCurTarget) && (pCurTarget->depth)) || bZBuffer) {
    flags |= GL_DEPTH_BUFFER_BIT;
  }

  const GLfloat a = (static_cast<GLfloat>((color >> 24) & 0xFF)) / 255.0f;
  const GLfloat r = (static_cast<GLfloat>((color >> 16) & 0xFF)) / 255.0f;
  const GLfloat g = (static_cast<GLfloat>((color >>  8) & 0xFF)) / 255.0f;
  const GLfloat b = (static_cast<GLfloat>((color >>  0) & 0xFF)) / 255.0f;
  pOpenGLDevice->glClearColor(r, g, b, a);
  pOpenGLDevice->glClear(flags);
}

void CALL HGE_Impl::Gfx_SetClipping(int x, int y, int w, int h)
{
  int scr_width, scr_height;
  struct {
    int X;
    int Y;
    int Width;
    int Height;
    float MinZ;
    float MaxZ;
  } vp;

  if (!pCurTarget) {
    scr_width = pHGE->System_GetStateInt(HGE_SCREENWIDTH);
    scr_height = pHGE->System_GetStateInt(HGE_SCREENHEIGHT);
  } else {
    scr_width = Texture_GetWidth(pCurTarget->tex);
    scr_height = Texture_GetHeight(pCurTarget->tex);
  }

  if (!w) {
    vp.X = 0;
    vp.Y = 0;
    vp.Width = scr_width;
    vp.Height = scr_height;
  } else {
    if (x < 0) {
      w += x;
      x = 0;
    }

    if (y < 0) {
      h += y;
      y = 0;
    }

    if (x + w > scr_width) {
      w = scr_width - x;
    }

    if (y + h > scr_height) {
      h = scr_height - y;
    }

    vp.X = x;
    vp.Y = y;
    vp.Width = w;
    vp.Height = h;
  }

  if ((clipX == vp.X) && (clipY == vp.Y) && (clipW == vp.Width) && (clipH == vp.Height)) {
    return;  // nothing to do here, don't call into the GL.
  }

  vp.MinZ = 0.0f;
  vp.MaxZ = 1.0f;

  _render_batch();

  clipX = vp.X;
  clipY = vp.Y;
  clipW = vp.Width;
  clipH = vp.Height;
  pOpenGLDevice->glScissor(vp.X, (scr_height - vp.Y) - vp.Height, vp.Width, vp.Height);
}

void CALL HGE_Impl::Gfx_SetTransform(float x, float y, float dx, float dy, float rot, float hscale,
                                     float vscale)
{
  if (!bTransforming) {
    if ((x == 0.0f) && (y == 0.0f) && (dx == 0.0f) && (dy == 0.0f) && (rot == 0.0f) && (hscale == 1.0f)
        && (vscale == 1.0f)) {
      return;  // nothing to do here, don't call into the GL.
    }
  }

  _render_batch();

  bTransforming = true;

  // !!! FIXME: this math is probably wrong. Re-sync with the Direct3D code.

  pOpenGLDevice->glMatrixMode(GL_MODELVIEW);

  if (vscale == 0.0f) {
    pOpenGLDevice->glLoadIdentity();
  } else {
    pOpenGLDevice->glTranslatef(-x, -y, 0.0f);
    //pOpenGLDevice->glScalef(1.0f, -1.0f, 1.0f);
    pOpenGLDevice->glRotatef(-rot, 0.0f, 0.0f, -1.0f);
    pOpenGLDevice->glTranslatef(x + dx, y + dy, 0.0f);
  }
}

bool CALL HGE_Impl::Gfx_BeginScene(HTARGET targ)
{
  CRenderTargetList *target = reinterpret_cast<CRenderTargetList *>(targ);

  if (VertArray) {
    _PostError("Gfx_BeginScene: Scene is already being rendered");
    return false;
  }

  if (target != pCurTarget) {
    if (pOpenGLDevice->have_GL_EXT_framebuffer_object) {
      pOpenGLDevice->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, (target) ? target->frame : 0);
    }

    if (((target) && (target->depth)) || (bZBuffer)) {
      pOpenGLDevice->glEnable(GL_DEPTH_TEST);
    } else {
      pOpenGLDevice->glDisable(GL_DEPTH_TEST);
    }

    // d3d's SetRenderTarget() forces the viewport to surface size...
    if (target) {
      pOpenGLDevice->glScissor(0, 0, target->width, target->height);
      pOpenGLDevice->glViewport(0, 0, target->width, target->height);
      _SetProjectionMatrix(target->width, target->height);
    } else {
      pOpenGLDevice->glScissor(0, 0, nScreenWidth, nScreenHeight);
      pOpenGLDevice->glViewport(0, 0, nScreenWidth, nScreenHeight);
      _SetProjectionMatrix(nScreenWidth, nScreenHeight);
    }

    pOpenGLDevice->glMatrixMode(GL_MODELVIEW);
    pOpenGLDevice->glLoadIdentity();

    pCurTarget = target;
  }

  VertArray = pVB;
  return true;
}

void CALL HGE_Impl::Gfx_EndScene()
{
  _render_batch(true);

  // no "real" render targets? Push the framebuffer to a texture.
  // This is not going to work in lots of legitimate scenarios, but it will
  //  most of the time, so it's better than nothing when you lack FBOs.
  if ((pCurTarget) && (!pOpenGLDevice->have_GL_EXT_framebuffer_object)) {
    gltexture *pTex = reinterpret_cast<gltexture *>(pCurTarget->tex);

    if ((pTex != nullptr) && (pTex->lost)) {
      _ConfigureTexture(pTex,
                        static_cast<int>(pTex->width),
                        static_cast<int>(pTex->height),
                        pTex->pixels);
    }

    const int width = pCurTarget->width;
    const int height = pCurTarget->height;
    pOpenGLDevice->glFinish();
    uint32_t *pixels = new uint32_t[width * height];
    pOpenGLDevice->glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    pOpenGLDevice->glBindTexture(pOpenGLDevice->TextureTarget, pTex->name);
    pOpenGLDevice->glTexSubImage2D(pOpenGLDevice->TextureTarget, 0, 0, 0, width, height,
                                   GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    pOpenGLDevice->glBindTexture(pOpenGLDevice->TextureTarget,
                                 CurTexture ?
                                 ((reinterpret_cast<gltexture *>(CurTexture))->name) : 0);
    delete[] pixels;
  }

  if (!pCurTarget) {
    SDL_GL_SwapBuffers();
  }

  //const GLenum err = pOpenGLDevice->glGetError();
  //if (err != GL_NO_ERROR) printf("GL error! 0x%X\n", (int) err);
  //Gfx_Clear(0xFF | (0xFF<<24) | (random() & 0xFF << 16) | (random() & 0xFF << 8));
  //Gfx_Clear(0xFF000000);
}

void HGE_Impl::_SetTextureFilter()
{
  const GLenum filter = (bTextureFilter) ? GL_LINEAR : GL_NEAREST;
  pOpenGLDevice->glTexParameteri(pOpenGLDevice->TextureTarget,
                                 GL_TEXTURE_MIN_FILTER,
                                 static_cast<GLint>(filter));
  pOpenGLDevice->glTexParameteri(pOpenGLDevice->TextureTarget,
                                 GL_TEXTURE_MAG_FILTER,
                                 static_cast<GLint>(filter));
}


bool HGE_Impl::_PrimsOutsideClipping(const hgeVertex *v, const int verts)
{
  if (bTransforming) {
    return false;  // screw it, let the GL do the clipping.
  }

  const int maxX = clipX + clipW;
  const int maxY = clipY + clipH;

  for (int i = 0; i < verts; i++, v++) {
    const int x = static_cast<int>(v->x);
    const int y = static_cast<int>(v->y);

    if ((x > clipX) && (x < maxX) && (y > clipY) && (y < maxY)) {
      return false;
    }
  }

  return true;
}


void CALL HGE_Impl::Gfx_RenderLine(float x1, float y1, float x2, float y2, uint32_t color, float z)
{
  if (VertArray) {
    if (CurPrimType != HGEPRIM_LINES || nPrim >= VERTEX_BUFFER_SIZE / HGEPRIM_LINES || CurTexture
        || CurBlendMode != BLEND_DEFAULT) {
      _render_batch();

      CurPrimType = HGEPRIM_LINES;

      if (CurBlendMode != BLEND_DEFAULT) {
        _SetBlendMode(BLEND_DEFAULT);
      }

      _BindTexture(nullptr);
    }

    int i = nPrim * HGEPRIM_LINES;
    VertArray[i].x = x1;
    VertArray[i + 1].x = x2;
    VertArray[i].y = y1;
    VertArray[i + 1].y = y2;
    VertArray[i].z     = VertArray[i + 1].z = z;
    VertArray[i].col   = VertArray[i + 1].col = color;
    VertArray[i].tx    = VertArray[i + 1].tx =
                           VertArray[i].ty    = VertArray[i + 1].ty = 0.0f;

    if (!_PrimsOutsideClipping(&VertArray[i], HGEPRIM_LINES)) {
      nPrim++;
    }
  }
}

//template <class T> static inline const T Min(const T a, const T b)
//{
//  return a < b ? a : b;
//}
//template <class T> static inline const T Max(const T a, const T b)
//{
//  return a > b ? a : b;
//}

void CALL HGE_Impl::Gfx_RenderTriple(const hgeTriple *triple)
{
  if (VertArray) {
    const hgeVertex *v = triple->v;

    if (_PrimsOutsideClipping(v, HGEPRIM_TRIPLES)) {
      // check for overlap, despite triangle points being outside clipping...
      const int maxX = clipX + clipW;
      const int maxY = clipY + clipH;
      const int leftmost = static_cast<int>(std::min(std::min(v[0].x, v[1].x),
                                            v[2].x));
      const int rightmost = static_cast<int>(std::max(std::max(v[0].x, v[1].x),
                                             v[2].x));
      const int topmost = static_cast<int>(std::min(std::min(v[0].y, v[1].y),
                                           v[2].y));
      const int bottommost = static_cast<int>(std::max(std::max(v[0].y, v[1].y),
                                              v[2].y));

      if (((clipX < leftmost) || (clipX > rightmost)) &&
          ((maxX < leftmost) || (maxX > rightmost)) &&
          ((clipY < topmost) || (clipY > bottommost)) &&
          ((maxY < topmost) || (maxY > bottommost))) {
        return;  // no, this is really totally clipped.
      }
    }

    if (CurPrimType != HGEPRIM_TRIPLES || nPrim >= VERTEX_BUFFER_SIZE / HGEPRIM_TRIPLES
        || CurTexture != triple->tex || CurBlendMode != triple->blend) {
      _render_batch();

      CurPrimType = HGEPRIM_TRIPLES;

      if (CurBlendMode != triple->blend) {
        _SetBlendMode(triple->blend);
      }

      _BindTexture(reinterpret_cast<gltexture *>(triple->tex));
    }

    memcpy(&VertArray[nPrim * HGEPRIM_TRIPLES], triple->v, sizeof(hgeVertex)*HGEPRIM_TRIPLES);
    nPrim++;
  }
}

void CALL HGE_Impl::Gfx_RenderQuad(const hgeQuad *quad)
{
  if (VertArray) {
    const hgeVertex *v = quad->v;

    if (_PrimsOutsideClipping(v, HGEPRIM_QUADS)) {
      // check for overlap, despite quad points being outside clipping...
      const int maxX = clipX + clipW;
      const int maxY = clipY + clipH;
      const int leftmost = static_cast<int32_t>(
                             std::min(std::min(std::min(v[0].x, v[1].x), v[2].x), v[3].x));
      const int rightmost = static_cast<int32_t>(
                              std::max(std::max(std::max(v[0].x, v[1].x), v[2].x), v[3].x));
      const int topmost = static_cast<int32_t>(
                            std::min(std::min(std::min(v[0].y, v[1].y), v[2].y), v[3].y));
      const int bottommost = static_cast<int32_t>(
                               std::max(std::max(std::max(v[0].y, v[1].y), v[2].y), v[3].y));

      if (((clipX < leftmost) || (clipX > rightmost)) &&
          ((maxX < leftmost) || (maxX > rightmost)) &&
          ((clipY < topmost) || (clipY > bottommost)) &&
          ((maxY < topmost) || (maxY > bottommost))) {
        return;  // no, this is really totally clipped.
      }
    }

    if (CurPrimType != HGEPRIM_QUADS || nPrim >= VERTEX_BUFFER_SIZE / HGEPRIM_QUADS
        || CurTexture != quad->tex
        || CurBlendMode != quad->blend) {
      _render_batch();

      CurPrimType = HGEPRIM_QUADS;

      if (CurBlendMode != quad->blend) {
        _SetBlendMode(quad->blend);
      }

      _BindTexture(reinterpret_cast<gltexture *>(quad->tex));
    }

    memcpy(&VertArray[nPrim * HGEPRIM_QUADS], quad->v, sizeof(hgeVertex)*HGEPRIM_QUADS);
    nPrim++;
  }
}

hgeVertex *CALL HGE_Impl::Gfx_StartBatch(int prim_type, HTEXTURE tex, int blend, int *max_prim)
{
  if (VertArray) {
    _render_batch();

    CurPrimType = prim_type;

    if (CurBlendMode != blend) {
      _SetBlendMode(blend);
    }

    _BindTexture(reinterpret_cast<gltexture *>(tex));
    *max_prim = VERTEX_BUFFER_SIZE / prim_type;
    return VertArray;
  } else {
    return 0;
  }
}

void CALL HGE_Impl::Gfx_FinishBatch(int nprim)
{
  nPrim = nprim;
}

bool HGE_Impl::_BuildTarget(CRenderTargetList *pTarget, GLuint texname, int width, int height,
                            bool zbuffer)
{
  bool okay = true;  // no FBOs? Fake success by default.

  if (pOpenGLDevice->have_GL_EXT_framebuffer_object) {
    pOpenGLDevice->glGenFramebuffersEXT(1, &pTarget->frame);

    if (zbuffer) {
      pOpenGLDevice->glGenRenderbuffersEXT(1, &pTarget->depth);
    }

    pOpenGLDevice->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pTarget->frame);
    pOpenGLDevice->glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
        pOpenGLDevice->TextureTarget, texname, 0);

    if (zbuffer) {
      pOpenGLDevice->glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, pTarget->depth);
      pOpenGLDevice->glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, width, height);
      pOpenGLDevice->glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
          GL_RENDERBUFFER_EXT, pTarget->depth);
    }

    GLenum rc = pOpenGLDevice->glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

    if ((rc == GL_FRAMEBUFFER_COMPLETE_EXT) && (pOpenGLDevice->glGetError() == GL_NO_ERROR)) {
      pOpenGLDevice->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      okay = true;
    } else {
      pOpenGLDevice->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
      pOpenGLDevice->glDeleteRenderbuffersEXT(1, &pTarget->depth);
      pOpenGLDevice->glDeleteFramebuffersEXT(1, &pTarget->frame);
    }

    pOpenGLDevice->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pCurTarget ? pCurTarget->frame : 0);
  }

  return okay;
}

HTARGET CALL HGE_Impl::Target_Create(int width, int height, bool zbuffer)
{
  bool okay = false;
  CRenderTargetList *pTarget = new CRenderTargetList;
  memset(pTarget, '\0', sizeof(CRenderTargetList));

  pTarget->tex = _BuildTexture(width, height, nullptr);
  gltexture *gltex = reinterpret_cast<gltexture *>(pTarget->tex);
  gltex->is_render_target = true;
  gltex->lost = false;
  _ConfigureTexture(gltex, width, height, nullptr);

  pTarget->width = width;
  pTarget->height = height;

  okay = _BuildTarget(pTarget, gltex->name, width, height, zbuffer);

  if (!okay) {
    System_Log("OpenGL: Failed to create render target!");
    Texture_Free(pTarget->tex);
    delete pTarget;
    return 0;
  }

  pTarget->next = pTargets;
  pTargets = pTarget;

  return reinterpret_cast<HTARGET>(pTarget);
}

void CALL HGE_Impl::Target_Free(HTARGET target)
{
  CRenderTargetList *pTarget = pTargets, *pPrevTarget = nullptr;

  while (pTarget) {
    if (reinterpret_cast<CRenderTargetList *>(target) == pTarget) {
      if (pPrevTarget) {
        pPrevTarget->next = pTarget->next;
      } else {
        pTargets = pTarget->next;
      }

      if (pOpenGLDevice->have_GL_EXT_framebuffer_object) {
        if (pCurTarget == reinterpret_cast<CRenderTargetList *>(target)) {
          pOpenGLDevice->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
        }

        if (pTarget->depth) {
          pOpenGLDevice->glDeleteRenderbuffersEXT(1, &pTarget->depth);
        }

        pOpenGLDevice->glDeleteFramebuffersEXT(1, &pTarget->frame);
      }

      if (pCurTarget == reinterpret_cast<CRenderTargetList *>(target)) {
        pCurTarget = 0;
      }

      Texture_Free(pTarget->tex);
      delete pTarget;
      return;
    }

    pPrevTarget = pTarget;
    pTarget = pTarget->next;
  }
}

HTEXTURE CALL HGE_Impl::Target_GetTexture(HTARGET target)
{
  CRenderTargetList *targ = reinterpret_cast<CRenderTargetList *>(target);

  if (target) {
    return targ->tex;
  } else {
    return 0;
  }
}

static inline bool _IsPowerOfTwo(const GLuint x)
{
  return ((x & (x - 1)) == 0);
}

static inline GLuint _NextPowerOfTwo(GLuint x)
{
  x--;

  for (int i = 1; i < static_cast<int>(sizeof(GLuint) * 8); i *= 2) {
    x |= x >> i;
  }

  return x + 1;
}

void HGE_Impl::_ConfigureTexture(gltexture *t, int width, int height, uint32_t *pixels)
{
  GLuint tex = 0;
  pOpenGLDevice->glGenTextures(1, &tex);

  t->lost = false;
  t->name = tex;
  t->width = static_cast<uint32_t>(width);
  t->height = static_cast<uint32_t>(height);
  t->pixels = pixels;
  t->potw = 0;
  t->poth = 0;

  // see if we're backed by a file and not RAM.
  const bool loadFromFile = ((pixels == nullptr) && (t->filename != nullptr));

  if (loadFromFile) {
    uint32_t size = 0;
    uint8_t *data = reinterpret_cast<uint8_t *>(
                      pHGE->Resource_Load(t->filename, &size));

    if (data != nullptr) {
      int w, h;
      pixels = _DecodeImage(data, t->filename, size, w, h);

      if ((w != width) || (h != height)) { // yikes, file changed?
        delete[] pixels;
        pixels = nullptr;
      }

      Resource_Free(data);
    }
  }

  pOpenGLDevice->glBindTexture(pOpenGLDevice->TextureTarget, tex);

  if (pOpenGLDevice->TextureTarget != GL_TEXTURE_RECTANGLE_ARB) {
    pOpenGLDevice->glTexParameterf(pOpenGLDevice->TextureTarget, GL_TEXTURE_MIN_LOD, 0.0f);
    pOpenGLDevice->glTexParameterf(pOpenGLDevice->TextureTarget, GL_TEXTURE_MAX_LOD, 0.0f);
    pOpenGLDevice->glTexParameteri(pOpenGLDevice->TextureTarget, GL_TEXTURE_BASE_LEVEL, 0);
    pOpenGLDevice->glTexParameteri(pOpenGLDevice->TextureTarget, GL_TEXTURE_MAX_LEVEL, 0);
  }

  const GLenum intfmt = pOpenGLDevice->have_GL_EXT_texture_compression_s3tc ?
                        GL_COMPRESSED_RGBA_S3TC_DXT5_EXT : GL_RGBA;

  if ((pOpenGLDevice->have_GL_ARB_texture_rectangle)
      || (pOpenGLDevice->have_GL_ARB_texture_non_power_of_two)
      || (_IsPowerOfTwo(static_cast<uint32_t>(width))
          && _IsPowerOfTwo(static_cast<uint32_t>(height)))) {
    pOpenGLDevice->glTexImage2D(pOpenGLDevice->TextureTarget, 0,
                                static_cast<int32_t>(intfmt),
                                width, height, 0, GL_RGBA,
                                GL_UNSIGNED_BYTE, pixels);
  } else {
    t->potw = _NextPowerOfTwo(static_cast<uint32_t>(width));
    t->poth = _NextPowerOfTwo(static_cast<uint32_t>(height));
    pOpenGLDevice->glTexImage2D(pOpenGLDevice->TextureTarget, 0,
                                static_cast<int32_t>(intfmt),
                                static_cast<GLsizei>(t->potw),
                                static_cast<GLsizei>(t->poth),
                                0, GL_RGBA,
                                GL_UNSIGNED_BYTE, nullptr);
    pOpenGLDevice->glTexSubImage2D(pOpenGLDevice->TextureTarget, 0, 0, 0, width, height, GL_RGBA,
                                   GL_UNSIGNED_BYTE, pixels);
  }

  pOpenGLDevice->glBindTexture(pOpenGLDevice->TextureTarget,
                               CurTexture ?
                               ((reinterpret_cast<gltexture *>(CurTexture))->name) : 0);

  if (loadFromFile) {
    delete[] pixels;
  }
}

HTEXTURE HGE_Impl::_BuildTexture(int width, int height, uint32_t *pixels)
{
  gltexture *retval = new gltexture;
  memset(retval, '\0', sizeof(gltexture));
  retval->lost = true;  // we'll actually generate a texture and upload when forced.
  retval->width = static_cast<uint32_t>(width);
  retval->height = static_cast<uint32_t>(height);
  retval->pixels = pixels;
  return reinterpret_cast<HTEXTURE>(retval);
}

HTEXTURE CALL HGE_Impl::Texture_Create(int width, int height)
{
  uint32_t *pixels = new uint32_t[width * height];
  memset(pixels, '\0', sizeof(uint32_t) * static_cast<uint32_t>(width * height));
  HTEXTURE retval = _BuildTexture(width, height, pixels);

  // the Direct3D renderer doesn't add these to the (textures) list, but we need them for when we "lose" the GL context.
  if (retval != 0) {
    CTextureList *texItem = new CTextureList;
    texItem->tex = retval;
    texItem->width = width;
    texItem->height = height;
    texItem->next = textures;
    textures = texItem;
  }

  return retval;
}

HTEXTURE CALL HGE_Impl::Texture_Load(const char *filename, uint32_t size, bool /*bMipmap*/)
{
  HTEXTURE retval = 0;
  int width = 0;
  int height = 0;

  void *data;
  uint32_t _size;
  CTextureList *texItem;
  const char *fname = nullptr;

  if (size) {
    data = reinterpret_cast<void *>(const_cast<char *>(filename));
    _size = size;
  } else {
    fname = filename;
    data = pHGE->Resource_Load(filename, &_size);

    if (!data) {
      return 0;
    }
  }

  uint32_t *pixels = _DecodeImage(reinterpret_cast<uint8_t *>(data),
                                  fname, _size, width, height);

  if (pixels != nullptr) {
    retval = _BuildTexture(width, height, pixels);
  }

  if (!size) {
    Resource_Free(data);
  }

  if (retval == 0) {
    STUBBED("texture load fail!");
    _PostError("Can't create texture");
  } else {
    texItem = new CTextureList;
    texItem->tex = retval;
    texItem->width = width;
    texItem->height = height;
    texItem->next = textures;
    textures = texItem;

    // force an upload to the GL and lose our copy if it's backed by
    //  a file. We won't keep it here to conserve system RAM.
    if (!size) {
      gltexture *t = reinterpret_cast<gltexture *>(retval);
      _ConfigureTexture(t, static_cast<int32_t>(t->width),
                        static_cast<int32_t>(t->height), t->pixels);
      delete[] t->pixels;
      t->pixels = nullptr;
      t->filename = strcpy(new char[strlen(filename) + 1], filename);
    }
  }

  return retval;
}

void CALL HGE_Impl::Texture_Free(HTEXTURE tex)
{
  if (pOpenGLDevice == nullptr) {
    return;  // in case we already shut down.
  }

  CTextureList *texItem = textures, *texPrev = 0;

  while (texItem) {
    if (texItem->tex == tex) {
      if (texPrev) {
        texPrev->next = texItem->next;
      } else {
        textures = texItem->next;
      }

      delete texItem;
      break;
    }

    texPrev = texItem;
    texItem = texItem->next;
  }

  if (tex) {
    gltexture *pTex = reinterpret_cast<gltexture *>(tex);
    delete[] pTex->filename;
    delete[] pTex->lock_pixels;
    delete[] pTex->pixels;
    pOpenGLDevice->glDeleteTextures(1, &pTex->name);
    delete pTex;
  }
}

int CALL HGE_Impl::Texture_GetWidth(HTEXTURE tex, bool bOriginal)
{
  CTextureList *texItem = textures;

  if (bOriginal) {
    while (texItem) {
      if (texItem->tex == tex) {
        return texItem->width;
      }

      texItem = texItem->next;
    }
  } else {
    return static_cast<int32_t>(reinterpret_cast<gltexture *>(tex)->width);
  }

  return 0;
}


int CALL HGE_Impl::Texture_GetHeight(HTEXTURE tex, bool bOriginal)
{
  CTextureList *texItem = textures;

  if (bOriginal) {
    while (texItem) {
      if (texItem->tex == tex) {
        return texItem->height;
      }

      texItem = texItem->next;
    }
  } else {
    return static_cast<int32_t>(
             reinterpret_cast<gltexture *>(tex)->height
           );
  }

  return 0;
}

// HGE extension!
// fast path for pushing YUV video to a texture instead of having to
//  lock/convert-to-rgba/unlock...current HGE semantics involve a
//  lot of unnecessary overhead on this, not to mention the conversion
//  on the CPU is painful on PowerPC chips.
// This lets us use OpenGL extensions to move data to the hardware
//  without conversion.
// Don't taunt this function. Side effects are probably rampant.
bool CALL HGE_Impl::HGEEXT_Texture_PushYUV422(HTEXTURE tex, const uint8_t *yuv)
{
  if (!pOpenGLDevice->have_GL_APPLE_ycbcr_422) {
    return false;
  }

  gltexture *pTex = reinterpret_cast<gltexture *>(tex);
  assert(!pTex->lock_pixels);

  if (pTex->lost) { // just reupload the whole thing.
    _ConfigureTexture(pTex, static_cast<int32_t>(pTex->width),
                      static_cast<int32_t>(pTex->height), pTex->pixels);
  }

  // Any existing pixels aren't valid anymore.
  if (pTex->pixels) {
    delete[] pTex->pixels;
    pTex->pixels = nullptr;
  }

  pOpenGLDevice->glBindTexture(pOpenGLDevice->TextureTarget, pTex->name);
  pOpenGLDevice->glTexSubImage2D(pOpenGLDevice->TextureTarget, 0, 0, 0,
                                 static_cast<int32_t>(pTex->width),
                                 static_cast<int32_t>(pTex->height),
                                 GL_YCBCR_422_APPLE,
                                 GL_UNSIGNED_SHORT_8_8_APPLE, yuv);
  pOpenGLDevice->glBindTexture(pOpenGLDevice->TextureTarget,
                               CurTexture ? (reinterpret_cast<gltexture *>(CurTexture)->name) : 0);
  return true;
}

uint32_t *CALL HGE_Impl::Texture_Lock(HTEXTURE tex, bool bReadOnly, int left, int top, int width,
                                      int height)
{
  gltexture *pTex = reinterpret_cast<gltexture *>(tex);

  if (pTex->lock_pixels) {
    assert(false && "multiple lock of texture...");
    return 0;
  }

  // see if we're backed by a file and not RAM.
  const bool loadFromFile = ((pTex->pixels == nullptr) && (pTex->filename != nullptr));

  if (loadFromFile) {
    uint32_t size = 0;
    uint8_t *data = reinterpret_cast<uint8_t *>(pHGE->Resource_Load(pTex->filename, &size));

    if (data != nullptr) {
      int w, h;
      pTex->pixels = _DecodeImage(data, pTex->filename, size, w, h);

      if ((static_cast<uint32_t>(w) != pTex->width)
          || (static_cast<uint32_t>(h) != pTex->height)) { // yikes, file changed?
        delete[] pTex->pixels;
        pTex->pixels = nullptr;
      }

      Resource_Free(data);
    }

    if (pTex->pixels != nullptr) {
      // can't go back to file after we lock, since app might change data.
      if (!bReadOnly) {
        delete[] pTex->filename;
        pTex->filename = nullptr;
      }
    }
  }

  if ((pTex->pixels == nullptr) && (!pTex->is_render_target)) { // can't lock this texture...?!
    return 0;
  }

  // !!! FIXME: is this right?
  if ((width == 0) && (height == 0)) {
    width = static_cast<int32_t>(pTex->width);
    height = static_cast<int32_t>(pTex->height);
  }

  // !!! FIXME: do something with this?
  assert(width > 0);
  assert(width <= static_cast<int32_t>(pTex->width));
  assert(height > 0);
  assert(height <= static_cast<int32_t>(pTex->height));
  assert(left >= 0);
  assert(left <= width);
  assert(top >= 0);
  assert(top <= height);

  pTex->lock_readonly = bReadOnly;
  pTex->lock_x = left;
  pTex->lock_y = top;
  pTex->lock_width = width;
  pTex->lock_height = height;
  pTex->lock_pixels = new uint32_t[width * height];

  uint32_t *dst = pTex->lock_pixels;

  if (pTex->is_render_target) {
    assert(false && "need to bind fbo before glReadPixels...");
    uint32_t *upsideDown = new uint32_t[width * height];
    uint32_t *src = upsideDown + ((height - 1) * width);
    pOpenGLDevice->glReadPixels(left,
                                (static_cast<int32_t>(pTex->height) - top) - height,
                                width, height, GL_RGBA,
                                GL_UNSIGNED_BYTE, upsideDown);

    for (int i = 0; i < height; i++) {
      memcpy(dst, src, static_cast<uint32_t>(width) * sizeof(uint32_t));
      dst += width;
      src -= width;
    }

    delete[] upsideDown;
  } else {
    uint32_t *src = pTex->pixels + ((static_cast<uint32_t>(top) * pTex->width)
                                    + static_cast<uint32_t>(left));

    for (int i = 0; i < height; i++) {
      memcpy(dst, src, static_cast<uint32_t>(width) * sizeof(uint32_t));
      dst += width;
      src += pTex->width;
    }
  }

  return pTex->lock_pixels;
}


void CALL HGE_Impl::Texture_Unlock(HTEXTURE tex)
{
  gltexture *pTex = reinterpret_cast<gltexture *>(tex);

  if (pTex->lock_pixels == nullptr) {
    return;  // not locked.
  }

  if (!pTex->lock_readonly) { // have to reupload to the hardware.
    // need to update pTex->pixels ...
    const uint32_t *src = pTex->lock_pixels;
    uint32_t *dst = pTex->pixels + ((static_cast<uint32_t>(pTex->lock_y) * pTex->width)
                                    + static_cast<uint32_t>(pTex->lock_x));

    for (int i = 0; i < pTex->lock_height; i++) {
      memcpy(dst, src, static_cast<uint32_t>(pTex->lock_width) * sizeof(uint32_t));
      dst += pTex->width;
      src += pTex->lock_width;
    }

    if (pTex->lost) { // just reupload the whole thing.
      _ConfigureTexture(pTex, static_cast<int32_t>(pTex->width),
                        static_cast<int32_t>(pTex->height), pTex->pixels);
    } else {
      pOpenGLDevice->glBindTexture(pOpenGLDevice->TextureTarget, pTex->name);
      pOpenGLDevice->glTexSubImage2D(pOpenGLDevice->TextureTarget, 0, pTex->lock_x,
                                     ((static_cast<int32_t>(pTex->height) - pTex->lock_y) - pTex->lock_height),
                                     pTex->lock_width, pTex->lock_height, GL_RGBA,
                                     GL_UNSIGNED_BYTE, pTex->lock_pixels);
      pOpenGLDevice->glBindTexture(pOpenGLDevice->TextureTarget,
                                   CurTexture ?
                                   (reinterpret_cast<gltexture *>(CurTexture)->name) : 0);
    }
  }

  // if we were read-only and we're backed by a file, ditch the uncompressed copy in system RAM.
  if ((pTex->filename != nullptr) && (pTex->lock_readonly)) {
    delete[] pTex->pixels;
    pTex->pixels = nullptr;
  }

  delete[] pTex->lock_pixels;
  pTex->lock_pixels = nullptr;
  pTex->lock_readonly = false;
  pTex->lock_x = -1;
  pTex->lock_y = -1;
  pTex->lock_width = -1;
  pTex->lock_height = -1;
}

//////// Implementation ////////

#define DEBUG_VERTICES 0
#if DEBUG_VERTICES
static inline void print_vertex(const hgeVertex *v)
{
  printf("  (%f, %f, %f), 0x%X, (%f, %f)\n", v->x, v->y, v->z, v->col, v->tx, v->ty);
}
#endif

void HGE_Impl::_render_batch(bool bEndScene)
{
  if (VertArray) {
    if (nPrim) {
      const float h = static_cast<float>((pCurTarget) ? pCurTarget->height : nScreenHeight);

      // texture rectangles range from 0 to size, not 0 to 1.  :/
      float texwmult = 1.0f;
      float texhmult = 1.0f;

      if (CurTexture) {
        _SetTextureFilter();
        const gltexture *pTex = reinterpret_cast<gltexture *>(CurTexture);

        if (pOpenGLDevice->TextureTarget == GL_TEXTURE_RECTANGLE_ARB) {
          texwmult = pTex->width;
          texhmult = pTex->height;
        } else if ((pTex->potw != 0) && (pTex->poth != 0)) {
          texwmult = (static_cast<float>(pTex->width) / static_cast<float>(pTex->potw));
          texhmult = (static_cast<float>(pTex->height) / static_cast<float>(pTex->poth));
        }
      }

      for (int i = 0; i < nPrim * CurPrimType; i++) {
        // (0, 0) is the lower left in OpenGL, upper left in D3D.
        VertArray[i].y = h - VertArray[i].y;

        // Z axis is inverted in OpenGL from D3D.
        VertArray[i].z = -VertArray[i].z;

        // (0, 0) is lower left texcoord in OpenGL, upper left in D3D.
        // Also, scale for texture rectangles vs. 2D textures.
        VertArray[i].tx *= texwmult;
        VertArray[i].ty = (1.0f - VertArray[i].ty) * texhmult;

        // Colors are RGBA in OpenGL, ARGB in Direct3D.
        const uint32_t color = VertArray[i].col;
        uint8_t *col = reinterpret_cast<uint8_t *>(&VertArray[i].col);
        const uint8_t a = ((color >> 24) & 0xFF);
        const uint8_t r = ((color >> 16) & 0xFF);
        const uint8_t g = ((color >>  8) & 0xFF);
        const uint8_t b = ((color >>  0) & 0xFF);
        col[0] = r;
        col[1] = g;
        col[2] = b;
        col[3] = a;
      }

      switch (CurPrimType) {
      case HGEPRIM_QUADS:
        pOpenGLDevice->glDrawElements(GL_TRIANGLES, nPrim * 6, GL_UNSIGNED_SHORT, pIB);
#if DEBUG_VERTICES

        for (int i = 0; i < nPrim * 6; i += 3) {
          printf("QUAD'S TRIANGLE:\n");
          print_vertex(&pVB[pIB[i + 0]]);
          print_vertex(&pVB[pIB[i + 1]]);
          print_vertex(&pVB[pIB[i + 2]]);
        }

        printf("DONE.\n");
#endif
        break;

      case HGEPRIM_TRIPLES:
        pOpenGLDevice->glDrawArrays(GL_TRIANGLES, 0, nPrim * 3);
        break;

      case HGEPRIM_LINES:
        pOpenGLDevice->glDrawArrays(GL_LINES, 0, nPrim * 2);
        break;
      }

      nPrim = 0;
    }

    if (bEndScene) {
      VertArray = 0;
    } else {
      VertArray = pVB;
    }
  }
}

void HGE_Impl::_SetBlendMode(int blend)
{
  if ((blend & BLEND_ALPHABLEND) != (CurBlendMode & BLEND_ALPHABLEND)) {
    if (blend & BLEND_ALPHABLEND) {
      pOpenGLDevice->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
      pOpenGLDevice->glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    }
  }

  if ((blend & BLEND_ZWRITE) != (CurBlendMode & BLEND_ZWRITE)) {
    if (blend & BLEND_ZWRITE) {
      pOpenGLDevice->glDepthMask(GL_TRUE);
    } else {
      pOpenGLDevice->glDepthMask(GL_FALSE);
    }
  }

  if ((blend & BLEND_COLORADD) != (CurBlendMode & BLEND_COLORADD)) {
    if (blend & BLEND_COLORADD) {
      pOpenGLDevice->glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
    } else {
      pOpenGLDevice->glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
  }

  CurBlendMode = blend;
}

void HGE_Impl::_SetProjectionMatrix(int width, int height)
{
  pOpenGLDevice->glMatrixMode(GL_PROJECTION);
  pOpenGLDevice->glLoadIdentity();
  pOpenGLDevice->glOrtho(0, static_cast<float>(width),
                         0, static_cast<float>(height),
                         0.0f, 1.0f);
  bTransforming = false;
  clipX = 0;
  clipY = 0;
  clipW = width;
  clipH = height;
}

void HGE_Impl::_UnloadOpenGLEntryPoints()
{
#define GL_PROC(ext,fn,call,ret,params) pOpenGLDevice->fn = nullptr;
#include "hge_glfuncs.h"
#undef GL_PROC
}

bool HGE_Impl::_HaveOpenGLExtension(const char *extlist, const char *ext)
{
  const char *ptr = strstr(extlist, ext);

  if (ptr == nullptr) {
    return false;
  }

  const char endchar = ptr[strlen(ext)];

  if ((endchar == '\0') || (endchar == ' ')) {
    return true;  // extension is in the list.
  }

  return false;  // just not supported, fail.
}

bool HGE_Impl::_LoadOpenGLEntryPoints()
{
  System_Log("OpenGL: loading entry points and examining extensions...");

  // these can be reset to false below...
  pOpenGLDevice->have_base_opengl = true;
  pOpenGLDevice->have_GL_ARB_texture_rectangle = true;
  pOpenGLDevice->have_GL_ARB_texture_non_power_of_two = true;
  pOpenGLDevice->have_GL_EXT_framebuffer_object = true;
  pOpenGLDevice->have_GL_EXT_texture_compression_s3tc = true;
  pOpenGLDevice->have_GL_ARB_vertex_buffer_object = true;
  pOpenGLDevice->have_GL_APPLE_ycbcr_422 = true;

#define GL_PROC(ext,fn,call,ret,params) \
   if (pOpenGLDevice->have_##ext) { \
     if ((pOpenGLDevice->fn = reinterpret_cast<_HGE_PFN_##fn>(SDL_GL_GetProcAddress(#fn))) == nullptr) { \
       System_Log("Failed to load OpenGL entry point '" #fn "'"); \
       pOpenGLDevice->have_##ext = false; \
     } \
   } else {}
#include "hge_glfuncs.h"
#undef GL_PROC

  if (!pOpenGLDevice->have_base_opengl) {
    _UnloadOpenGLEntryPoints();
    return false;
  }

  System_Log("GL_RENDERER: %s", pOpenGLDevice->glGetString(GL_RENDERER));
  System_Log("GL_VENDOR: %s", pOpenGLDevice->glGetString(GL_VENDOR));
  System_Log("GL_VERSION: %s", pOpenGLDevice->glGetString(GL_VERSION));

  const char *verstr = reinterpret_cast<const char *>(
                         pOpenGLDevice->glGetString(GL_VERSION));
  int maj = 0;
  int min = 0;
  sscanf(verstr, "%d.%d", &maj, &min);

  if ((maj < 1) || ((maj == 1) && (min < 2))) {
    _PostError("OpenGL implementation must be at least version 1.2");
    _UnloadOpenGLEntryPoints();
    return false;
  }

  const char *exts = reinterpret_cast<const char *>(
                       pOpenGLDevice->glGetString(GL_EXTENSIONS));

  // NPOT texture support ...

  if (_HaveOpenGLExtension(exts, "GL_ARB_texture_rectangle")) {
    pOpenGLDevice->have_GL_ARB_texture_rectangle = true;
  } else if (_HaveOpenGLExtension(exts, "GL_EXT_texture_rectangle")) {
    pOpenGLDevice->have_GL_ARB_texture_rectangle = true;
  } else if (_HaveOpenGLExtension(exts, "GL_NV_texture_rectangle")) {
    pOpenGLDevice->have_GL_ARB_texture_rectangle = true;
  } else {
    pOpenGLDevice->have_GL_ARB_texture_rectangle = false;
  }

  if (maj >= 2) {
    pOpenGLDevice->have_GL_ARB_texture_non_power_of_two = true;
  } else if (_HaveOpenGLExtension(exts, "GL_ARB_texture_non_power_of_two")) {
    pOpenGLDevice->have_GL_ARB_texture_non_power_of_two = true;
  } else {
    pOpenGLDevice->have_GL_ARB_texture_non_power_of_two = false;
  }

  if (pOpenGLDevice->have_GL_ARB_texture_rectangle) {
    System_Log("OpenGL: Using GL_ARB_texture_rectangle");
    pOpenGLDevice->TextureTarget = GL_TEXTURE_RECTANGLE_ARB;
  } else if (pOpenGLDevice->have_GL_ARB_texture_non_power_of_two) {
    System_Log("OpenGL: Using GL_ARB_texture_non_power_of_two");
    pOpenGLDevice->TextureTarget = GL_TEXTURE_2D;
  } else {
    // We can fake this with POT textures. Get a real OpenGL!
    System_Log("OpenGL: Using power-of-two textures. This costs more memory!");
    pOpenGLDevice->TextureTarget = GL_TEXTURE_2D;
  }

  // render-to-texture support ...

  // is false if an entry point is missing, but we still need to check for the extension string...
  if (pOpenGLDevice->have_GL_EXT_framebuffer_object) {
    // Disable this on Mac OS X Tiger, since some drivers appear to be buggy.
    if ((pHGE->MacOSXVersion) && (pHGE->MacOSXVersion < 0x1050)) {
      pOpenGLDevice->have_GL_EXT_framebuffer_object = false;
    } else if (_HaveOpenGLExtension(exts, "GL_EXT_framebuffer_object")) {
      pOpenGLDevice->have_GL_EXT_framebuffer_object = true;
    } else {
      pOpenGLDevice->have_GL_EXT_framebuffer_object = false;
    }
  }

  if (pOpenGLDevice->have_GL_EXT_framebuffer_object) {
    System_Log("OpenGL: Using GL_EXT_framebuffer_object");
  } else {
    System_Log("OpenGL: WARNING! No render-to-texture support. Things may render badly.");
  }


  // Texture compression ...

  if (bForceTextureCompression &&
      _HaveOpenGLExtension(exts, "GL_ARB_texture_compression") &&
      _HaveOpenGLExtension(exts, "GL_EXT_texture_compression_s3tc")) {
    pOpenGLDevice->have_GL_EXT_texture_compression_s3tc = true;
  } else {
    pOpenGLDevice->have_GL_EXT_texture_compression_s3tc = false;
  }

  if (pOpenGLDevice->have_GL_EXT_texture_compression_s3tc) {
    System_Log("OpenGL: Using GL_EXT_texture_compression_s3tc");
  } else if (bForceTextureCompression) {
    bForceTextureCompression = false;  // oh well.
    System_Log("OpenGL: WARNING: no texture compression support, in a low-memory system.");
    System_Log("OpenGL:  Performance may be very bad!");
  }

  // YUV textures...

  if (_HaveOpenGLExtension(exts, "GL_APPLE_ycbcr_422")) {
    pOpenGLDevice->have_GL_APPLE_ycbcr_422 = true;
  } else {
    pOpenGLDevice->have_GL_APPLE_ycbcr_422 = false;
  }

  if (pOpenGLDevice->have_GL_APPLE_ycbcr_422) {
    System_Log("OpenGL: Using GL_APPLE_ycbcr_422 to render YUV frames.");
  } else {
    System_Log("OpenGL: WARNING: no YUV texture support; videos may render slowly.");
  }

  // VBOs...

  // is false if an entry point is missing, but we still need to check for the extension string...
  if (pOpenGLDevice->have_GL_ARB_vertex_buffer_object) {
    if (_HaveOpenGLExtension(exts, "GL_ARB_vertex_buffer_object")) {
      pOpenGLDevice->have_GL_ARB_vertex_buffer_object = true;
    } else {
      pOpenGLDevice->have_GL_ARB_vertex_buffer_object = false;
    }
  }

  if (pOpenGLDevice->have_GL_ARB_vertex_buffer_object) {
    System_Log("OpenGL: Using GL_ARB_vertex_buffer_object");
  } else {
    System_Log("OpenGL: WARNING! No VBO support; performance may suffer.");
  }

  return true;
}

bool HGE_Impl::_GfxInit()
{
  CurTexture = 0;

// Init OpenGL ... SDL should have created a context at this point.
  assert(pOpenGLDevice == nullptr);
  pOpenGLDevice = new COpenGLDevice;

  if (!_LoadOpenGLEntryPoints()) {
    return false;  // already called _PostError().
  }

  nScreenBPP = SDL_GetVideoSurface()->format->BitsPerPixel;

  _AdjustWindow();

  System_Log("Mode: %d x %d\n", nScreenWidth, nScreenHeight);

// Create vertex batch buffer

  VertArray = 0;
  textures = 0;
  IndexBufferObject = 0;

// Init all stuff that can be lost

  if (!_init_lost()) {
    return false;
  }

  // make sure the framebuffers are cleared and force to screen
  pOpenGLDevice->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  SDL_GL_SwapBuffers();
  pOpenGLDevice->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  SDL_GL_SwapBuffers();

  return true;
}

void HGE_Impl::_AdjustWindow()
{
  // no-op.
}

void HGE_Impl::_Resize(int /*width*/, int /*height*/)
{
  if (hwndParent) {
    //if(procFocusLostFunc) procFocusLostFunc();
    STUBBED("resize");
#if 0
    d3dppW.BackBufferWidth = width;
    d3dppW.BackBufferHeight = height;
    nScreenWidth = width;
    nScreenHeight = height;

    _SetProjectionMatrix(nScreenWidth, nScreenHeight);
    _GfxRestore();
#endif

    //if(procFocusGainFunc) procFocusGainFunc();
  }
}

void HGE_Impl::_GfxDone()
{
  //CRenderTargetList *target=pTargets;

  while (textures) {
    Texture_Free(textures->tex);
  }

  while (pTargets) {
    Target_Free(reinterpret_cast<HTARGET>(pTargets));
  }

  textures = 0;
  pTargets = 0;

  VertArray = 0;
  delete[] pVB;
  pVB = 0;
  delete[] pIB;
  pIB = 0;

  if (pOpenGLDevice) {
    if (pOpenGLDevice->have_GL_ARB_vertex_buffer_object) {
      if (IndexBufferObject != 0) {
        pOpenGLDevice->glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
        pOpenGLDevice->glDeleteBuffersARB(1, &IndexBufferObject);
        IndexBufferObject = 0;
      }
    }

    delete pOpenGLDevice;
    pOpenGLDevice = 0;
  }
}


bool HGE_Impl::_GfxRestore()
{
  if (!pOpenGLDevice) {
    return false;
  }

  delete[] pVB;
  pVB = 0;

  delete[] pIB;
  pIB = 0;

  _UnloadOpenGLEntryPoints();

  if (!_LoadOpenGLEntryPoints()) {
    return false;
  }

  if (!_init_lost()) {
    return false;
  }

  if (procGfxRestoreFunc) {
    return procGfxRestoreFunc();
  }

  return true;
}


bool HGE_Impl::_init_lost()
{
  _BindTexture(nullptr);  // make sure nothing is bound, so everything that we do bind regenerates.

  for (CTextureList *item = textures; item != nullptr; item = item->next) {
    gltexture *t = reinterpret_cast<gltexture *>(item->tex);

    if (t == nullptr) {
      continue;
    }

    t->lost = true;
    t->name = 0;
  }

  CRenderTargetList *target = pTargets;

  while (target) {
    gltexture *tex = reinterpret_cast<gltexture *>(target->tex);
    _BindTexture(tex);  // force texture recreation.
    _BindTexture(nullptr);
    _BuildTarget(target, tex ? tex->name : 0, target->width, target->height, target->depth != 0);
    target = target->next;
  }

// Create Vertex buffer
  // We just use a client-side array, since you can reasonably count on support
  //  existing in any GL, and it basically offers the same functionality that
  //  HGE uses in Direct3D: it locks the vertex buffer, unlocks in time to
  //  draw something, then relocks again immediately...more or less, that method
  //  offers the same performance metrics as a client-side array.
  // We _will_ stuff the indices in a buffer object, though, if possible,
  //  since they never change...this matches the D3D behaviour better, since
  //  they lock, store, and forget about it, but without a buffer object,
  //  we'd have to pass the array over the bus every glDrawElements() call.
  //  It's not worth the tapdance for vertex buffer objects though, due to
  //  HGE's usage patterns.
  pVB = new hgeVertex[VERTEX_BUFFER_SIZE];

// Create and setup Index buffer
  pIB = new GLushort[VERTEX_BUFFER_SIZE * 6 / 4];
  GLushort *pIndices = pIB;
  int n = 0;

  for (int i = 0; i < VERTEX_BUFFER_SIZE / 4; i++) {
    *pIndices++ = static_cast<GLushort>(n);
    *pIndices++ = static_cast<GLushort>(n + 1);
    *pIndices++ = static_cast<GLushort>(n + 2);
    *pIndices++ = static_cast<GLushort>(n + 2);
    *pIndices++ = static_cast<GLushort>(n + 3);
    *pIndices++ = static_cast<GLushort>(n);
    n += 4;
  }

#if !DEBUG_VERTICES  // need pIB for DEBUG_VERTICES.

  if (pOpenGLDevice->have_GL_ARB_vertex_buffer_object) {
    // stay bound forever. The Index Buffer Object never changes.
    pOpenGLDevice->glGenBuffersARB(1, &IndexBufferObject);
    pOpenGLDevice->glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, IndexBufferObject);
    pOpenGLDevice->glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER,
                                   sizeof(GLushort) * ((VERTEX_BUFFER_SIZE * 6) / 4), pIB, GL_STATIC_DRAW);
    delete[] pIB;
    pIB = 0;
  }

#endif

  // always use client-side arrays; set it up once at startup.
  pOpenGLDevice->glVertexPointer(3, GL_FLOAT, sizeof(hgeVertex), &pVB[0].x);
  pOpenGLDevice->glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(hgeVertex), &pVB[0].col);
  pOpenGLDevice->glTexCoordPointer(2, GL_FLOAT, sizeof(hgeVertex), &pVB[0].tx);
  pOpenGLDevice->glEnableClientState(GL_VERTEX_ARRAY);
  pOpenGLDevice->glEnableClientState(GL_COLOR_ARRAY);
  pOpenGLDevice->glEnableClientState(GL_TEXTURE_COORD_ARRAY);

// Set common render states

  pOpenGLDevice->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  pOpenGLDevice->glPixelStorei(GL_PACK_ALIGNMENT, 1);

  //pD3DDevice->SetRenderState( D3DRS_LASTPIXEL, FALSE );
  pOpenGLDevice->glDisable(GL_TEXTURE_2D);

  if (pOpenGLDevice->have_GL_ARB_texture_rectangle) {
    pOpenGLDevice->glDisable(GL_TEXTURE_RECTANGLE_ARB);
  }

  pOpenGLDevice->glEnable(pOpenGLDevice->TextureTarget);
  pOpenGLDevice->glEnable(GL_SCISSOR_TEST);
  pOpenGLDevice->glDisable(GL_CULL_FACE);
  pOpenGLDevice->glDisable(GL_LIGHTING);
  pOpenGLDevice->glDepthFunc(GL_GEQUAL);

  if (bZBuffer) {
    pOpenGLDevice->glEnable(GL_DEPTH_TEST);
  } else {
    pOpenGLDevice->glDisable(GL_DEPTH_TEST);
  }

  pOpenGLDevice->glEnable(GL_BLEND);
  pOpenGLDevice->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  pOpenGLDevice->glEnable(GL_ALPHA_TEST);
  pOpenGLDevice->glAlphaFunc(GL_GEQUAL, 1.0f / 255.0f);

  pOpenGLDevice->glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  _SetTextureFilter();

  // !!! FIXME: this isn't what HGE's Direct3D code does, but the game I'm working with
  // !!! FIXME:  forces clamping outside of HGE, so I just wedged it in here.
  // Apple says texture rectangle on ATI X1000 chips only supports CLAMP_TO_EDGE.
  // Texture rectangle only supports CLAMP* wrap modes anyhow.
  pOpenGLDevice->glTexParameteri(pOpenGLDevice->TextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  pOpenGLDevice->glTexParameteri(pOpenGLDevice->TextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  pOpenGLDevice->glTexParameteri(pOpenGLDevice->TextureTarget, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  nPrim = 0;
  CurPrimType = HGEPRIM_QUADS;
  CurBlendMode = BLEND_DEFAULT;
  CurTexture = 0;

  pOpenGLDevice->glScissor(0, 0, nScreenWidth, nScreenHeight);
  pOpenGLDevice->glViewport(0, 0, nScreenWidth, nScreenHeight);

  // make sure the framebuffer is cleared and force to screen
  pOpenGLDevice->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  _SetProjectionMatrix(nScreenWidth, nScreenHeight);
  pOpenGLDevice->glMatrixMode(GL_MODELVIEW);
  pOpenGLDevice->glLoadIdentity();

  return true;
}
