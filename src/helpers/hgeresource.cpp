/*
** Haaf's Game Engine 1.7
** Copyright (C) 2003-2007, Relish Games
** hge.relishgames.com
**
** hgeResourceManager helper class implementation
*/


#include "../../include/hgeresource.h"
#include "parser.h"
#include "resources.h"


HGE *hgeResourceManager::hge=0;


hgeResourceManager::hgeResourceManager(const char *scriptname)
{
  hge=hgeCreate(HGE_VERSION);

  for(int i=0; i<RESTYPES; i++) {
    res[i]=0;
  }

  _parse_script(scriptname);
}

hgeResourceManager::~hgeResourceManager()
{
  _remove_all();
  hge->Release();
}

void hgeResourceManager::_parse_script(const char *scriptname)
{
  IResource *rc, *rcnext;

  if(scriptname) {
    RScript::Parse(this, NULL, scriptname, NULL);

    rc=res[RES_SCRIPT];

    while(rc) {
      rc->Free();
      rcnext=rc->next;
      delete rc;
      rc=rcnext;
    }

    res[RES_SCRIPT]=0;
  }
}

void hgeResourceManager::_remove_all()
{
  int i;
  IResource *rc, *rcnext;

  for(i=0; i<RESTYPES; i++) {
    rc=res[i];

    while(rc) {
      rc->Free();
      rcnext=rc->next;
      delete rc;
      rc=rcnext;
    }

    res[i]=0;
  }
}

void hgeResourceManager::ChangeScript(const char *scriptname)
{
  _remove_all();
  _parse_script(scriptname);
}

bool hgeResourceManager::Precache(int groupid)
{
  int i;
  IResource *rc;
  bool bResult=true;

  for(i=0; i<RESTYPES; i++) {
    rc=res[i];

    while(rc) {
      if(!groupid || groupid==rc->resgroup) {
        bResult=bResult && (rc->Get(this)!=0);
      }

      rc=rc->next;
    }
  }

  return bResult;
}

void hgeResourceManager::Purge(int groupid)
{
  int i;
  IResource *rc;

  for(i=0; i<RESTYPES; i++) {
    rc=res[i];

    while(rc) {
      if(!groupid || groupid==rc->resgroup) {
        rc->Free();
      }

      rc=rc->next;
    }
  }
}

void* hgeResourceManager::GetResource(const char *name, int resgroup)
{
  void *reshandle;
  RResource *resource;
  IResource *Res=FindRes(this, RES_RESOURCE, name);

  if(Res) {
    return reinterpret_cast<void *>(Res->Get(this));
  } else {
    reshandle=hge->Resource_Load(name);

    if(reshandle) {
      resource=new RResource();
      resource->handle = reinterpret_cast<size_t>(reshandle);
      resource->resgroup=resgroup;
      strcpy(resource->name, name);
      strcpy(resource->filename, name);
      AddRes(this, RES_RESOURCE, resource);

      return reshandle;
    }
  }

  return 0;
}

HTEXTURE hgeResourceManager::GetTexture(const char *name, int resgroup)
{
  HTEXTURE reshandle;
  RTexture *resource;
  IResource *Res=FindRes(this, RES_TEXTURE, name);

  if(Res) {
    return reinterpret_cast<HTEXTURE>(Res->Get(this));
  } else {
    reshandle=hge->Texture_Load(name);

    if(reshandle) {
      resource=new RTexture();
      resource->handle=reshandle;
      resource->resgroup=resgroup;
      resource->mipmap=false;
      strcpy(resource->name, name);
      strcpy(resource->filename, name);
      AddRes(this, RES_TEXTURE, resource);

      return reshandle;
    }
  }

  return 0;
}

HEFFECT hgeResourceManager::GetEffect(const char *name, int resgroup)
{
  HEFFECT reshandle;
  REffect *resource;
  IResource *Res=FindRes(this, RES_EFFECT, name);

  if(Res) {
    return reinterpret_cast<HEFFECT>(Res->Get(this));
  } else {
    reshandle=hge->Effect_Load(name);

    if(reshandle) {
      resource=new REffect();
      resource->handle=reshandle;
      resource->resgroup=resgroup;
      strcpy(resource->name, name);
      strcpy(resource->filename, name);
      AddRes(this, RES_EFFECT, resource);

      return reshandle;
    }
  }

  return 0;
}

HMUSIC hgeResourceManager::GetMusic(const char *name, int resgroup)
{
  HMUSIC reshandle;
  RMusic *resource;
  IResource *Res=FindRes(this, RES_MUSIC, name);

  if(Res) {
    return reinterpret_cast<HMUSIC>(Res->Get(this));
  } else {
    reshandle=hge->Music_Load(name);

    if(reshandle) {
      resource=new RMusic();
      resource->handle=reshandle;
      resource->resgroup=resgroup;
      strcpy(resource->name, name);
      strcpy(resource->filename, name);
      AddRes(this, RES_MUSIC, resource);

      return reshandle;
    }
  }

  return 0;
}

HSTREAM hgeResourceManager::GetStream(const char *name, int resgroup)
{
  HSTREAM reshandle;
  RStream *resource;
  IResource *Res=FindRes(this, RES_STREAM, name);

  if(Res) {
    return reinterpret_cast<HSTREAM>(Res->Get(this));
  } else {
    reshandle=hge->Stream_Load(name);

    if(reshandle) {
      resource=new RStream();
      resource->handle=reshandle;
      resource->resgroup=resgroup;
      strcpy(resource->name, name);
      strcpy(resource->filename, name);
      AddRes(this, RES_STREAM, resource);

      return reshandle;
    }
  }

  return 0;
}

HTARGET hgeResourceManager::GetTarget(const char *name)
{
  IResource *Res=FindRes(this, RES_TARGET, name);

  if(Res) {
    return reinterpret_cast<HTARGET>(Res->Get(this));
  } else {
    return 0;
  }
}

hgeSprite* hgeResourceManager::GetSprite(const char *name)
{
  IResource *Res=FindRes(this, RES_SPRITE, name);

  if(Res) {
    return reinterpret_cast<hgeSprite *>(Res->Get(this));
  } else {
    return 0;
  }
}

hgeAnimation* hgeResourceManager::GetAnimation(const char *name)
{
  IResource *Res=FindRes(this, RES_ANIMATION, name);

  if(Res) {
    return reinterpret_cast<hgeAnimation *>(Res->Get(this));
  } else {
    return 0;
  }
}

hgeFont* hgeResourceManager::GetFont(const char *name)
{
  IResource *Res=FindRes(this, RES_FONT, name);

  if(Res) {
    return reinterpret_cast<hgeFont *>(Res->Get(this));
  } else {
    return 0;
  }
}

hgeParticleSystem* hgeResourceManager::GetParticleSystem(const char *name)
{
  IResource *Res=FindRes(this, RES_PARTICLE, name);

  if(Res) {
    return reinterpret_cast<hgeParticleSystem *>(Res->Get(this));
  } else {
    return 0;
  }
}

hgeDistortionMesh* hgeResourceManager::GetDistortionMesh(const char *name)
{
  IResource *Res=FindRes(this, RES_DISTORT, name);

  if(Res) {
    return reinterpret_cast<hgeDistortionMesh *>(Res->Get(this));
  } else {
    return 0;
  }
}

hgeStringTable* hgeResourceManager::GetStringTable(const char *name, int resgroup)
{
  hgeStringTable *strtable;
  RStringTable *resource;
  IResource *Res=FindRes(this, RES_STRTABLE, name);

  if(Res) {
    return reinterpret_cast<hgeStringTable*>(Res->Get(this));
  } else {
    strtable=new hgeStringTable(name);

    if(strtable) {
      resource=new RStringTable();
      resource->handle= reinterpret_cast<size_t>(strtable);
      resource->resgroup=resgroup;
      strcpy(resource->name, name);
      strcpy(resource->filename, name);
      AddRes(this, RES_STRTABLE, resource);

      return strtable;
    }
  }

  return 0;
}

IResource::~IResource()
{
  hge->Release();
}
