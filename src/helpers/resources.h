/*
** Haaf's Game Engine 1.7
** Copyright (C) 2003-2007, Relish Games
** hge.relishgames.com
**
** hgeResourceManager resources header
*/

#ifndef HGERESOURCES_H
#define HGERESOURCES_H


#include "../../include/hgeresource.h"
#include "parser.h"


#define RES_SCRIPT    0

#define RES_RESOURCE  1
#define RES_TEXTURE   2
#define RES_EFFECT    3
#define RES_MUSIC   4
#define RES_STREAM    5
#define RES_TARGET    6
#define RES_SPRITE    7
#define RES_ANIMATION 8
#define RES_FONT    9
#define RES_PARTICLE  10
#define RES_DISTORT   11
#define RES_STRTABLE  12


void    AddRes(hgeResourceManager *rm, int type, IResource *resource);
IResource*  FindRes(hgeResourceManager *rm, int type, const char *name);


struct RScript : public IResource {
  static  void  Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                      const char *basename);
  virtual hgeResHandle Get(hgeResourceManager * /*rm*/)
  {
    return 0;
  }
  virtual void  Free();
  virtual void copy_from(IResource * r);
};

struct RResource : public IResource {
  char      filename[MAXRESCHARS];

  static  void  Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                      const char *basename);
  virtual hgeResHandle Get(hgeResourceManager *rm);
  virtual void  Free();
  virtual void copy_from(IResource * r);
};

struct RTexture : public IResource {
  char      filename[MAXRESCHARS];
  bool      mipmap;

  static  void  Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                      const char *basename);
  virtual hgeResHandle Get(hgeResourceManager *rm);
  virtual void  Free();

  virtual void copy_from(IResource * r);
};

struct REffect : public IResource {
  char      filename[MAXRESCHARS];

  static  void  Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                      const char *basename);
  virtual hgeResHandle Get(hgeResourceManager *rm);
  virtual void  Free();

  virtual void copy_from(IResource * r);
};

struct RMusic : public IResource {
  char      filename[MAXRESCHARS];
  int       amplify;

  static  void  Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                      const char *basename);
  virtual hgeResHandle Get(hgeResourceManager *rm);
  virtual void  Free();

  virtual void copy_from(IResource * r);
};

struct RStream : public IResource {
  char      filename[MAXRESCHARS];

  static  void  Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                      const char *basename);
  virtual hgeResHandle Get(hgeResourceManager *rm);
  virtual void  Free();

  virtual void copy_from(IResource * r);
};

struct RTarget : public IResource {
  int     width;
  int     height;
  bool    zbuffer;

  static  void  Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                      const char *basename);
  virtual hgeResHandle Get(hgeResourceManager *rm);
  virtual void  Free();

  virtual void copy_from(IResource * r);

};

struct RSprite : public IResource {
  char    texname[MAXRESCHARS];
  float   tx, ty, w, h;
  float   hotx, hoty;
  int     blend;
  uint32_t   color;
  float   z;
  bool    bXFlip, bYFlip;
//  float   x,y;
//  float   scale;
//  float   rotation;
//  int     collision;

  static  void  Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                      const char *basename);
  virtual hgeResHandle Get(hgeResourceManager *rm);
  virtual void  Free();

  virtual void copy_from(IResource * r);

};

struct RAnimation : public RSprite {
  int     frames;
  float   fps;
  int     mode;

  static  void  Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                      const char *basename);
  virtual hgeResHandle Get(hgeResourceManager *rm);
  virtual void  Free();

  virtual void copy_from(IResource * r);
};

struct RFont : public IResource {
  char    filename[MAXRESCHARS];
  bool    mipmap;
  int     blend;
  uint32_t   color;
  float   z;
  float   scale;
  float   proportion;
  float   tracking;
  float   spacing;
  float   rotation;

  static  void  Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                      const char *basename);
  virtual hgeResHandle Get(hgeResourceManager *rm);
  virtual void  Free();

  virtual void copy_from(IResource * r);
};

struct RParticle : public IResource {
  char    filename[MAXRESCHARS];
  char    spritename[MAXRESCHARS];

  static  void  Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                      const char *basename);
  virtual hgeResHandle Get(hgeResourceManager *rm);
  virtual void  Free();

  virtual void copy_from(IResource * r) {
    auto src = dynamic_cast<RParticle *>(r);
    memcpy(filename, src->filename, sizeof(filename));
    memcpy(spritename, src->spritename, sizeof(spritename));
    IResource::copy_from_base(r);
  }
};

struct RDistort : public IResource {
  char    texname[MAXRESCHARS];
  float   tx, ty, w, h;
  int     cols, rows;
  int     blend;
  uint32_t   color;
  float   z;

  static  void  Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                      const char *basename);
  virtual hgeResHandle Get(hgeResourceManager *rm);
  virtual void  Free();

  virtual void copy_from(IResource * r);
};


struct RStringTable : public IResource {
  char      filename[MAXRESCHARS];

  static  void  Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                      const char *basename);
  virtual hgeResHandle Get(hgeResourceManager *rm);
  virtual void  Free();

  virtual void copy_from(IResource * r);
};

#endif
