/*
** Haaf's Game Engine 1.7
** Copyright (C) 2003-2007, Relish Games
** hge.relishgames.com
**
** hgeResourceManager resources implementation
*/


#include "../../include/hgeresource.h"
#include "parser.h"
#include "resources.h"


HGE *IResource::hge=0;


/////////////// COMMON //

void AddRes(hgeResourceManager *rm, int type, IResource *resource)
{
  resource->next=rm->res[type];
  rm->res[type]=resource;
}

IResource *FindRes(hgeResourceManager *rm, int type, const char *name)
{
  IResource *rc;

  rc=rm->res[type];

  while(rc) {
    if(!strcmp(name, rc->name)) {
      return rc;
    }

    rc=rc->next;
  }

  return 0;
}

static bool ScriptSkipToNextParameter(RScriptParser *sp, bool bIgnore)
{
  bool bToBeIgnored=bIgnore;

  if(bIgnore) {
    sp->put_back();
  }

  for(;;) {
    sp->get_token();

    if(sp->tokentype == TTCLOSEBLOCK) {
      if(bIgnore) {
        sp->put_back();
        return true;
      }

      return false;
    }

    if((sp->tokentype > TTRES__FIRST && sp->tokentype < TTRES__LAST)  || sp->tokentype == TTEND) {
      sp->put_back();

      if(bIgnore) {
        return true;
      }

      sp->ScriptPostError("'}' missed, "," encountered.");
      return false;
    }

    if((sp->tokentype <= TTPAR__FIRST && sp->tokentype >= TTPAR__LAST) || bToBeIgnored) {
      bToBeIgnored=false;
      sp->ScriptPostError("Unsupported resource parameter ",".");

      do {
        sp->get_token();
      } while((sp->tokentype <= TTPAR__FIRST || sp->tokentype >= TTPAR__LAST) &&
              (sp->tokentype <= TTRES__FIRST || sp->tokentype >= TTRES__LAST) &&
              sp->tokentype != TTCLOSEBLOCK && sp->tokentype != TTEND);

      sp->put_back();
    } else {
      if(bIgnore) {
        sp->put_back();
      }

      return true;
    }
  }
}

static void ScriptParseFileResource(hgeResourceManager *rm, RScriptParser *sp,
                                    const char *name, const char *basename,
                                    IResource *rr, int restype)
{
  RResource *rc = reinterpret_cast<RResource *>(rr);
  RResource *base;

  base = reinterpret_cast<RResource *>(FindRes(rm, restype, basename));

  if(base) {
    //*rc=*base;
    rc->copy_from(base);
  } else {
    rc->resgroup=0;
    rc->filename[0]=0;
  }

  rc->handle=0;
  strcpy(rc->name, name);

  while(ScriptSkipToNextParameter(sp,false)) {
    switch(sp->tokentype) {
    case TTPAR_FILENAME:
      sp->get_token();
      sp->get_token();
      strcpy(rc->filename, sp->tkn_string());
      break;

    case TTPAR_RESGROUP:
      sp->get_token();
      sp->get_token();
      rc->resgroup=sp->tkn_int();
      break;

    default:
      ScriptSkipToNextParameter(sp,true);
      break;
    }
  }

  AddRes(rm, restype, rc);
}

static void ScriptParseBlendMode(RScriptParser *sp, int *blend)
{
  for(;;) {
    sp->get_token();

    if(sp->tokentype != TTEQUALS && sp->tokentype != TTSEPARATOR) {
      sp->put_back();
      return;
    }

    switch(sp->get_token()) {
    case TTCON_COLORMUL:
      *blend &= ~BLEND_COLORADD;
      break;

    case TTCON_COLORADD:
      *blend |= BLEND_COLORADD;
      break;

    case TTCON_ALPHABLND:
      *blend |= BLEND_ALPHABLEND;
      break;

    case TTCON_ALPHAADD:
      *blend &= ~BLEND_ALPHABLEND;
      break;

    case TTCON_ZWRITE:
      *blend |= BLEND_ZWRITE;
      break;

    case TTCON_NOZWRITE:
      *blend &= ~BLEND_ZWRITE;
      break;

    default:
      sp->ScriptPostError("Unsupported value ",".");
      break;
    }
  }
}

static void ScriptParseSpriteAnim(RScriptParser *sp, RSprite *rc, bool anim)
{
  while(ScriptSkipToNextParameter(sp,false)) {
    switch(sp->tokentype) {
    case TTPAR_TEXTURE:
      sp->get_token();
      sp->get_token();
      strcpy(rc->texname,sp->tkn_string());
      break;

    case TTPAR_RECT:
      sp->get_token();
      sp->get_token();
      rc->tx=sp->tkn_float();
      sp->get_token();
      sp->get_token();
      rc->ty=sp->tkn_float();
      sp->get_token();
      sp->get_token();
      rc->w=sp->tkn_float();
      sp->get_token();
      sp->get_token();
      rc->h=sp->tkn_float();
      break;

    case TTPAR_HOTSPOT:
      sp->get_token();
      sp->get_token();
      rc->hotx=sp->tkn_float();
      sp->get_token();
      sp->get_token();
      rc->hoty=sp->tkn_float();
      break;

    case TTPAR_BLENDMODE:
      ScriptParseBlendMode(sp, &rc->blend);
      break;

    case TTPAR_COLOR:
      sp->get_token();
      sp->get_token();
      rc->color=sp->tkn_hex();
      break;

    case TTPAR_ZORDER:
      sp->get_token();
      sp->get_token();
      rc->z=sp->tkn_float();
      break;

    case TTPAR_FLIP:
      sp->get_token();
      sp->get_token();
      rc->bXFlip=sp->tkn_bool();
      sp->get_token();
      sp->get_token();
      rc->bYFlip=sp->tkn_bool();
      break;

    case TTPAR_RESGROUP:
      sp->get_token();
      sp->get_token();
      rc->resgroup=sp->tkn_int();
      break;

    case TTPAR_FRAMES:
      if(anim) {
        sp->get_token();
        sp->get_token();
        reinterpret_cast<RAnimation *>(rc)->frames = sp->tkn_int();
        break;
      }

      break;

    case TTPAR_FPS:
      if(anim) {
        sp->get_token();
        sp->get_token();
        reinterpret_cast<RAnimation *>(rc)->fps=sp->tkn_float();
        break;
      }

      break;

    case TTPAR_MODE:
      if(anim) {
        for(;;) {
          sp->get_token();

          if(sp->tokentype != TTEQUALS && sp->tokentype != TTSEPARATOR) {
            sp->put_back();
            break;
          }

          switch(sp->get_token()) {
          case TTCON_FORWARD:
            reinterpret_cast<RAnimation *>(rc)->mode &= ~HGEANIM_REV;
            break;

          case TTCON_REVERSE:
            reinterpret_cast<RAnimation *>(rc)->mode |= HGEANIM_REV;
            break;

          case TTCON_NOPINGPONG:
            reinterpret_cast<RAnimation *>(rc)->mode &= ~HGEANIM_PINGPONG;
            break;

          case TTCON_PINGPONG:
            reinterpret_cast<RAnimation *>(rc)->mode |= HGEANIM_PINGPONG;
            break;

          case TTCON_NOLOOP:
            reinterpret_cast<RAnimation *>(rc)->mode &= ~HGEANIM_LOOP;
            break;

          case TTCON_LOOP:
            reinterpret_cast<RAnimation *>(rc)->mode |= HGEANIM_LOOP;
            break;

          default:
            sp->ScriptPostError("Unsupported value ",".");
            break;
          }
        }

        break;
      }

      break;

    default:
      ScriptSkipToNextParameter(sp,true);
      break;
    }
  }
}


/////////////// RScript //

void RScript::Parse(hgeResourceManager *rm, RScriptParser *sp, const char *sname,
                    const char * /*sbasename*/)
{
  RScriptParser *np;
  RScript *res_script;
  void *data;
  uint32_t size;
  char *script, name[MAXRESCHARS], basename[MAXRESCHARS];
  int restype;

  if(!FindRes(rm, RES_SCRIPT, sname)) {
    res_script = new RScript(); // hack! we need an instance of RScript to access hge
    // if all ok, this object is used later to keep the script

    data=hge->Resource_Load(sname, &size);

    if(!data) {
      if(sp) {
        sp->ScriptPostError("Script "," not found.");
      } else {
        hge->System_Log("Script '%s' not found.",sname);
      }

      delete res_script;
      return;
    } else {
      script= new char[size+1];
      memcpy(script, data, size);
      script[size]=0;
      hge->Resource_Free(data);

      strcpy(res_script->name, sname);
      AddRes(rm, RES_SCRIPT, res_script);
      np = new RScriptParser(res_script->name, script);

      for(;;) {
        np->get_token();

        if(np->tokentype == TTEND) {
          break;
        }

        else if(np->tokentype == TTRES_INCLUDE) {
          np->get_token();
          RScript::Parse(rm, np, np->tkn_string(), NULL);
        }

        else if(np->tokentype > TTRES__FIRST && np->tokentype < TTRES__LAST) {
          restype=np->tokentype-TTRES__FIRST-1;
          name[0]=basename[0]=0;

          np->get_token();

          if(FindRes(rm, restype, np->tkn_string())) {
            np->ScriptPostError("Resource "," of the same type already has been defined.");

            while((np->tokentype <= TTRES__FIRST || np->tokentype >= TTRES__LAST) && np->tokentype != TTEND) {
              np->get_token();
            }

            np->put_back();
            continue;
          }

          strcpy(name, np->tkn_string());

          np->get_token();

          if(np->tokentype == TTBASED) {
            np->get_token();

            if(!FindRes(rm, restype, np->tkn_string())) {
              np->ScriptPostError("Base resource "," is not defined.");
            } else {
              strcpy(basename, np->tkn_string());
            }

            np->get_token();
          }

          if(np->tokentype == TTOPENBLOCK) {
            switch(restype) {
            case RES_RESOURCE:
              RResource::Parse(rm, np, name, basename);
              break;

            case RES_TEXTURE:
              RTexture::Parse(rm, np, name, basename);
              break;

            case RES_EFFECT:
              REffect::Parse(rm, np, name, basename);
              break;

            case RES_MUSIC:
              RMusic::Parse(rm, np, name, basename);
              break;

            case RES_STREAM:
              RStream::Parse(rm, np, name, basename);
              break;

            case RES_TARGET:
              RTarget::Parse(rm, np, name, basename);
              break;

            case RES_SPRITE:
              RSprite::Parse(rm, np, name, basename);
              break;

            case RES_ANIMATION:
              RAnimation::Parse(rm, np, name, basename);
              break;

            case RES_FONT:
              RFont::Parse(rm, np, name, basename);
              break;

            case RES_PARTICLE:
              RParticle::Parse(rm, np, name, basename);
              break;

            case RES_DISTORT:
              RDistort::Parse(rm, np, name, basename);
              break;

            case RES_STRTABLE:
              RStringTable::Parse(rm, np, name, basename);
              break;
            }
          } else {
            np->ScriptPostError("Illegal resource syntax, "," found; '{' expected.");

            while((np->tokentype <= TTRES__FIRST || np->tokentype >= TTRES__LAST) && np->tokentype != TTEND) {
              np->get_token();
            }

            np->put_back();
          }
        }

        else {
          np->ScriptPostError("Unrecognized resource specificator ",".");

          while((np->tokentype <= TTRES__FIRST || np->tokentype >= TTRES__LAST) && np->tokentype != TTEND) {
            np->get_token();
          }

          np->put_back();
        }
      }

      delete np;
      delete[] script;
    }
  } else {
    sp->ScriptPostError("Script "," already has been parsed.");
  }
}

void RScript::Free() {}

void RScript::copy_from(IResource *r)
{
  IResource::copy_from_base(r);
}

/////////////// RResource //

void RResource::Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                      const char *basename)
{
  ScriptParseFileResource(rm, sp, name, basename, new RResource(), RES_RESOURCE);
}

hgeResHandle RResource::Get(hgeResourceManager * /*rm*/)
{
  if(!handle) {
    handle = reinterpret_cast<size_t>(hge->Resource_Load(filename));
  }

  return handle;
}

void RResource::Free()
{
  if(handle) {
    hge->Resource_Free(reinterpret_cast<void *>(handle));
  }

  handle=0;
}

void RResource::copy_from(IResource *r)
{
  auto src = dynamic_cast<RResource *>(r);
  memcpy(filename, src->filename, sizeof(filename));
  IResource::copy_from_base(r);
}

/////////////// RTexture //

void RTexture::Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                     const char *basename)
{
  RTexture *rc, *base;

  rc=new RTexture();
  base = reinterpret_cast<RTexture *>(FindRes(rm, RES_TEXTURE, basename));

  if(base) {
    //*rc=*base;
    rc->copy_from(base);
  } else {
    rc->resgroup=0;
    rc->mipmap=false;
  }

  rc->handle=0;
  strcpy(rc->name, name);

  while(ScriptSkipToNextParameter(sp,false)) {
    switch(sp->tokentype) {
    case TTPAR_FILENAME:
      sp->get_token();
      sp->get_token();
      strcpy(rc->filename, sp->tkn_string());
      break;

    case TTPAR_RESGROUP:
      sp->get_token();
      sp->get_token();
      rc->resgroup=sp->tkn_int();
      break;

    case TTPAR_MIPMAP:
      sp->get_token();
      sp->get_token();
      rc->mipmap=sp->tkn_bool();
      break;

    default:
      ScriptSkipToNextParameter(sp,true);
      break;
    }
  }

  AddRes(rm, RES_TEXTURE, rc);
}

hgeResHandle RTexture::Get(hgeResourceManager * /*rm*/)
{
  if(!handle) {
    handle = reinterpret_cast<hgeResHandle>(
               hge->Texture_Load(filename, 0, mipmap));
  }

  return handle;
}

void RTexture::Free()
{
  if(handle) {
    hge->Texture_Free(reinterpret_cast<HTEXTURE>(handle));
  }

  handle=0;
}

void RTexture::copy_from(IResource *r)
{
  auto src = dynamic_cast<RTexture *>(r);
  memcpy(filename, src->filename, sizeof(filename));
  mipmap = src->mipmap;
  IResource::copy_from_base(r);
}

/////////////// REffect //

void REffect::Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                    const char *basename)
{
  ScriptParseFileResource(rm, sp, name, basename, new REffect(), RES_EFFECT);
}

hgeResHandle REffect::Get(hgeResourceManager * /*rm*/)
{
  if(!handle) {
    handle = static_cast<hgeResHandle>(hge->Effect_Load(filename));
  }

  return handle;
}

void REffect::Free()
{
  if(handle) {
    hge->Effect_Free(reinterpret_cast<HEFFECT>(handle));
  }

  handle=0;
}

void REffect::copy_from(IResource *r)
{
  auto src = dynamic_cast<REffect *>(r);
  memcpy(filename, src->filename, sizeof(filename));
  IResource::copy_from_base(r);
}

/////////////// RMusic //

void RMusic::Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                   const char *basename)
{
  //  ScriptParseFileResource(rm, sp, name, basename, new RMusic(), RES_MUSIC);


  RMusic *rc, *base;

  rc=new RMusic();
  base = reinterpret_cast<RMusic *>(FindRes(rm, RES_MUSIC, basename));

  if(base) {
    //*rc=*base;
    rc->copy_from(base);
  } else {
    rc->resgroup=0;
    rc->amplify=50;
  }

  rc->handle=0;
  strcpy(rc->name, name);

  while(ScriptSkipToNextParameter(sp,false)) {
    switch(sp->tokentype) {
    case TTPAR_FILENAME:
      sp->get_token();
      sp->get_token();
      strcpy(rc->filename, sp->tkn_string());
      break;

    case TTPAR_RESGROUP:
      sp->get_token();
      sp->get_token();
      rc->resgroup=sp->tkn_int();
      break;

    case TTPAR_AMPLIFY:
      sp->get_token();
      sp->get_token();
      rc->amplify=sp->tkn_int();
      break;

    default:
      ScriptSkipToNextParameter(sp,true);
      break;
    }
  }

  AddRes(rm, RES_MUSIC, rc);
}

hgeResHandle RMusic::Get(hgeResourceManager * /*rm*/)
{
  if(!handle) {
    handle = static_cast<hgeResHandle>(hge->Music_Load(filename));
    hge->Music_SetAmplification(handle, amplify);
  }

  return handle;
}

void RMusic::Free()
{
  if(handle) {
    hge->Music_Free(reinterpret_cast<HMUSIC>(handle));
  }

  handle=0;
}

void RMusic::copy_from(IResource *r)
{
  auto src = dynamic_cast<RMusic *>(r);
  memcpy(filename, src->filename, sizeof(filename));
  amplify = src->amplify;
  IResource::copy_from_base(r);
}

/////////////// RStream //

void RStream::Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                    const char *basename)
{
  ScriptParseFileResource(rm, sp, name, basename, new RStream(), RES_STREAM);
}

hgeResHandle RStream::Get(hgeResourceManager * /*rm*/)
{
  if(!handle) {
    handle = reinterpret_cast<hgeResHandle>(hge->Stream_Load(filename));
  }

  return handle;
}

void RStream::Free()
{
  if(handle) {
    hge->Stream_Free(reinterpret_cast<HSTREAM>(handle));
  }

  handle=0;
}

void RStream::copy_from(IResource *r)
{
  auto src = dynamic_cast<RStream *>(r);
  memcpy(filename, src->filename, sizeof(filename));
  IResource::copy_from_base(r);
}

/////////////// RTarget //

void RTarget::Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                    const char *basename)
{
  RTarget *rc, *base;

  rc = new RTarget();
  base = reinterpret_cast<RTarget *>(FindRes(rm, RES_TARGET, basename));

  if(base) {
    //*rc=*base;
    rc->copy_from(base);
  } else {
    rc->resgroup=0;
    rc->width=256;
    rc->height=256;
    rc->zbuffer=false;
  }

  rc->handle=0;
  strcpy(rc->name, name);

  while(ScriptSkipToNextParameter(sp, false)) {
    switch(sp->tokentype) {
    case TTPAR_SIZE:
      sp->get_token();
      sp->get_token();
      rc->width=sp->tkn_int();
      sp->get_token();
      sp->get_token();
      rc->height=sp->tkn_int();
      break;

    case TTPAR_ZBUFFER:
      sp->get_token();
      sp->get_token();
      rc->zbuffer=sp->tkn_bool();
      break;

    case TTPAR_RESGROUP:
      sp->get_token();
      sp->get_token();
      rc->resgroup=sp->tkn_int();
      break;

    default:
      ScriptSkipToNextParameter(sp, true);
      break;
    }
  }

  AddRes(rm, RES_TARGET, rc);
}

hgeResHandle RTarget::Get(hgeResourceManager * /*rm*/)
{
  if(!handle) {
    handle = reinterpret_cast<hgeResHandle>(
               hge->Target_Create(width, height, zbuffer));
  }

  return handle;
}

void RTarget::Free()
{
  if(handle) {
    hge->Target_Free(reinterpret_cast<HTARGET>(handle));
  }

  handle=0;
}

void RTarget::copy_from(IResource *r)
{
  auto src = dynamic_cast<RTarget *>(r);
  width = src->width;
  height = src->height;
  zbuffer = src->zbuffer;
  IResource::copy_from_base(r);
}

/////////////// RSprite //

void RSprite::Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                    const char *basename)
{
  RSprite *rc, *base;

  rc = new RSprite();
  base = reinterpret_cast<RSprite *>(FindRes(rm, RES_SPRITE, basename));

  if(base) {
    //*rc=*base;
    rc->copy_from(base);
  } else {
    rc->resgroup=0;
    rc->texname[0]=0;
    rc->tx=rc->ty=0;
    rc->w=rc->h=0;
    rc->hotx=rc->hoty=0;
    rc->blend=BLEND_COLORMUL | BLEND_ALPHABLEND | BLEND_NOZWRITE;
    rc->color=0xFFFFFFFF;
    rc->z=0.5f;
    rc->bXFlip=false;
    rc->bYFlip=false;
//    rc->x=rc->y=0;
//    rc->scale=1.0f;
//    rc->rotation=0.0f;
//    rc->collision=HGECOL_RECT;
  }

  rc->handle=0;
  strcpy(rc->name, name);

  ScriptParseSpriteAnim(sp, rc, false);
  AddRes(rm, RES_SPRITE, rc);
}

hgeResHandle RSprite::Get(hgeResourceManager *rm)
{
  hgeSprite *spr;

  if(!handle) {
    spr = new hgeSprite(rm->GetTexture(texname, resgroup), tx, ty, w, h);
    spr->SetColor(color);
    spr->SetZ(z);
    spr->SetBlendMode(blend);
    spr->SetHotSpot(hotx,hoty);
    spr->SetFlip(bXFlip, bYFlip);
//    spr->MoveTo(x,y);
//    spr->SetScale(scale);
//    spr->SetRotation(rotation);
//    spr->SetCollisionType(collision);

    handle = reinterpret_cast<hgeResHandle>(spr);
  }

  return handle;
}

void RSprite::Free()
{
  if(handle) {
    delete reinterpret_cast<hgeSprite *>(handle);
  }

  handle=0;
}

void RSprite::copy_from(IResource *r)
{
  auto src = dynamic_cast<RSprite *>(r);
  memcpy(texname, src->texname, sizeof(texname));
  tx = src->tx;
  ty = src->ty;
  w = src->w;
  h = src->h;
  hotx = src->hotx;
  hoty = src->hoty;
  blend = src->blend;
  color = src->color;
  z = src->z;
  bXFlip = src->bXFlip;
  bYFlip = src->bYFlip;
  IResource::copy_from_base(r);
}

/////////////// RAnimation //

void RAnimation::Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                       const char *basename)
{
  RAnimation *rc, *base;

  rc = new RAnimation();
  base = reinterpret_cast<RAnimation *>(FindRes(rm, RES_ANIMATION, basename));

  if(base) {
    //*rc=*base;
    rc->copy_from(base);
  } else {
    rc->resgroup=0;
    rc->texname[0]=0;
    rc->tx=rc->ty=0;
    rc->w=rc->h=0;
    rc->hotx=rc->hoty=0;
    rc->blend=BLEND_COLORMUL | BLEND_ALPHABLEND | BLEND_NOZWRITE;
    rc->color=0xFFFFFFFF;
    rc->z=0.5f;
    rc->bXFlip=false;
    rc->bYFlip=false;
//    rc->x=rc->y=0;
//    rc->scale=1.0f;
//    rc->rotation=0.0f;
//    rc->collision=HGECOL_RECT;
    rc->frames=1;
    rc->fps=12.0f;
    rc->mode=HGEANIM_FWD | HGEANIM_LOOP;
  }

  rc->handle=0;
  strcpy(rc->name, name);

  ScriptParseSpriteAnim(sp, rc, true);
  AddRes(rm, RES_ANIMATION, rc);
}

hgeResHandle RAnimation::Get(hgeResourceManager *rm)
{
  hgeAnimation *spr;

  if(!handle) {
    spr = new hgeAnimation(rm->GetTexture(texname, resgroup), frames, fps, tx, ty, w, h);
    spr->SetColor(color);
    spr->SetZ(z);
    spr->SetBlendMode(blend);
    spr->SetHotSpot(hotx,hoty);
    spr->SetFlip(bXFlip, bYFlip);
//    spr->MoveTo(x,y);
//    spr->SetScale(scale);
//    spr->SetRotation(rotation);
//    spr->SetCollisionType(collision);
    spr->SetMode(mode);

    handle = reinterpret_cast<hgeResHandle>(spr);
  }

  return handle;
}

void RAnimation::Free()
{
  if(handle) {
    delete reinterpret_cast<hgeAnimation *>(handle);
  }

  handle=0;
}

void RAnimation::copy_from(IResource *r)
{
  auto src = dynamic_cast<RAnimation *>(r);
  frames = src->frames;
  fps = src->fps;
  mode = src->mode;
  IResource::copy_from_base(r);
}

/////////////// RFont //

void RFont::Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name, const char *basename)
{
  RFont *rc, *base;

  rc = new RFont();
  base = reinterpret_cast<RFont *>(FindRes(rm, RES_FONT, basename));

  if(base) {
//    *rc=*base;
    rc->copy_from(base);
  } else {
    rc->resgroup=0;
    rc->mipmap=false;
    rc->filename[0]=0;
    rc->blend=BLEND_COLORMUL | BLEND_ALPHABLEND | BLEND_NOZWRITE;
    rc->color=0xFFFFFFFF;
    rc->z=0.5f;
    rc->scale=1.0f;
    rc->proportion=1.0f;
    rc->tracking=0.0f;
    rc->spacing=1.0f;
    rc->rotation=0.0f;
  }

  rc->handle=0;
  strcpy(rc->name, name);

  while(ScriptSkipToNextParameter(sp,false)) {
    switch(sp->tokentype) {
    case TTPAR_FILENAME:
      sp->get_token();
      sp->get_token();
      strcpy(rc->filename, sp->tkn_string());
      break;

    case TTPAR_BLENDMODE:
      ScriptParseBlendMode(sp, &rc->blend);
      break;

    case TTPAR_COLOR:
      sp->get_token();
      sp->get_token();
      rc->color=sp->tkn_hex();
      break;

    case TTPAR_ZORDER:
      sp->get_token();
      sp->get_token();
      rc->z=sp->tkn_float();
      break;

    case TTPAR_SCALE:
      sp->get_token();
      sp->get_token();
      rc->scale=sp->tkn_float();
      break;

    case TTPAR_PROPORTION:
      sp->get_token();
      sp->get_token();
      rc->proportion=sp->tkn_float();
      break;

    case TTPAR_ROTATION:
      sp->get_token();
      sp->get_token();
      rc->rotation=sp->tkn_float();
      break;

    case TTPAR_TRACKING:
      sp->get_token();
      sp->get_token();
      rc->tracking=sp->tkn_float();
      break;

    case TTPAR_SPACING:
      sp->get_token();
      sp->get_token();
      rc->spacing=sp->tkn_float();
      break;

    case TTPAR_RESGROUP:
      sp->get_token();
      sp->get_token();
      rc->resgroup=sp->tkn_int();
      break;

    case TTPAR_MIPMAP:
      sp->get_token();
      sp->get_token();
      rc->mipmap=sp->tkn_bool();
      break;

    default:
      ScriptSkipToNextParameter(sp, true);
      break;
    }
  }

  AddRes(rm, RES_FONT, rc);
}

hgeResHandle RFont::Get(hgeResourceManager * /*rm*/)
{
  hgeFont *fnt;

  if(!handle) {
    fnt = new hgeFont(filename, mipmap);
    fnt->SetColor(color);
    fnt->SetZ(z);
    fnt->SetBlendMode(blend);
    fnt->SetScale(scale);
    fnt->SetProportion(proportion);
    fnt->SetTracking(tracking);
    fnt->SetSpacing(spacing);
    fnt->SetRotation(rotation);

    handle = reinterpret_cast<hgeResHandle>(fnt);
  }

  return handle;
}

void RFont::Free()
{
  if(handle) {
    delete reinterpret_cast<hgeFont *>(handle);
  }

  handle=0;
}

void RFont::copy_from(IResource *r)
{
  auto src = dynamic_cast<RFont *>(r);
  memcpy(filename, src->filename, sizeof(filename));
  mipmap = src->mipmap;
  blend = src->blend;
  color = src->color;
  z = src->z;
  scale = src->scale;
  proportion = src->proportion;
  tracking = src->tracking;
  spacing = src->spacing;
  rotation = src->rotation;
  IResource::copy_from_base(r);
}

/////////////// RParticle //

void RParticle::Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                      const char *basename)
{
  RParticle *rc, *base;

  rc = new RParticle();
  base = reinterpret_cast<RParticle *>(FindRes(rm, RES_PARTICLE, basename));

  if(base) {
    //*rc=*base;
    rc->copy_from(base);
  } else {
    rc->resgroup=0;
    rc->filename[0]=0;
    rc->spritename[0]=0;
  }

  rc->handle=0;
  strcpy(rc->name, name);

  while(ScriptSkipToNextParameter(sp, false)) {
    switch(sp->tokentype) {
    case TTPAR_FILENAME:
      sp->get_token();
      sp->get_token();
      strcpy(rc->filename, sp->tkn_string());
      break;

    case TTPAR_SPRITE:
      sp->get_token();
      sp->get_token();
      strcpy(rc->spritename, sp->tkn_string());
      break;

    case TTPAR_RESGROUP:
      sp->get_token();
      sp->get_token();
      rc->resgroup=sp->tkn_int();
      break;

    default:
      ScriptSkipToNextParameter(sp, true);
      break;
    }
  }

  AddRes(rm, RES_PARTICLE, rc);
}

hgeResHandle RParticle::Get(hgeResourceManager *rm)
{
  hgeParticleSystem *par;

  if(!handle) {
    par = new hgeParticleSystem(filename, rm->GetSprite(spritename));

    handle = reinterpret_cast<hgeResHandle>(par);
  }

  return handle;
}

void RParticle::Free()
{
  if(handle) {
    delete reinterpret_cast<hgeParticleSystem *>(handle);
  }

  handle=0;
}

/////////////// RDistort //

void RDistort::Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                     const char *basename)
{
  RDistort *rc, *base;

  rc = new RDistort();
  base = reinterpret_cast<RDistort *>(FindRes(rm, RES_DISTORT, basename));

  if(base) {
//    *rc=*base;
    rc->copy_from(base);
  } else {
    rc->resgroup=0;
    rc->texname[0]=0;
    rc->tx=rc->ty=0;
    rc->w=rc->h=0;
    rc->cols=rc->rows=2;
    rc->blend=BLEND_COLORMUL | BLEND_ALPHABLEND | BLEND_NOZWRITE;
    rc->color=0xFFFFFFFF;
    rc->z=0.5f;
  }

  rc->handle=0;
  strcpy(rc->name, name);

  while(ScriptSkipToNextParameter(sp, false)) {
    switch(sp->tokentype) {
    case TTPAR_TEXTURE:
      sp->get_token();
      sp->get_token();
      strcpy(rc->texname, sp->tkn_string());
      break;

    case TTPAR_RECT:
      sp->get_token();
      sp->get_token();
      rc->tx=sp->tkn_float();
      sp->get_token();
      sp->get_token();
      rc->ty=sp->tkn_float();
      sp->get_token();
      sp->get_token();
      rc->w=sp->tkn_float();
      sp->get_token();
      sp->get_token();
      rc->h=sp->tkn_float();
      break;

    case TTPAR_MESH:
      sp->get_token();
      sp->get_token();
      rc->cols=sp->tkn_int();
      sp->get_token();
      sp->get_token();
      rc->rows=sp->tkn_int();
      break;

    case TTPAR_BLENDMODE:
      ScriptParseBlendMode(sp, &rc->blend);
      break;

    case TTPAR_COLOR:
      sp->get_token();
      sp->get_token();
      rc->color=sp->tkn_hex();
      break;

    case TTPAR_ZORDER:
      sp->get_token();
      sp->get_token();
      rc->z=sp->tkn_float();
      break;

    case TTPAR_RESGROUP:
      sp->get_token();
      sp->get_token();
      rc->resgroup=sp->tkn_int();
      break;

    default:
      ScriptSkipToNextParameter(sp, true);
      break;
    }
  }

  AddRes(rm, RES_DISTORT, rc);
}

hgeResHandle RDistort::Get(hgeResourceManager *rm)
{
  hgeDistortionMesh *dis;

  if(!handle) {
    dis = new hgeDistortionMesh(cols, rows);
    dis->SetTexture(rm->GetTexture(texname, resgroup));
    dis->SetTextureRect(tx,ty,w,h);
    dis->SetBlendMode(blend);
    dis->Clear(color,z);

    handle = reinterpret_cast<hgeResHandle>(dis);
  }

  return handle;
}

void RDistort::Free()
{
  if(handle) {
    delete reinterpret_cast<hgeDistortionMesh *>(handle);
  }

  handle=0;
}

void RDistort::copy_from(IResource *r)
{
  auto src = dynamic_cast<RDistort *>(r);
  memcpy(texname, src->texname, sizeof(texname));
  tx = src->tx;
  ty = src->ty;
  w = src->w;
  h = src->h;
  cols = src->cols;
  rows = src->rows;
  blend = src->blend;
  color = src->color;
  z = src->z;
  IResource::copy_from_base(r);
}

/////////////// RStringTable //

void RStringTable::Parse(hgeResourceManager *rm, RScriptParser *sp, const char *name,
                         const char *basename)
{
  ScriptParseFileResource(rm, sp, name, basename, new RStringTable(), RES_STRTABLE);
}

hgeResHandle RStringTable::Get(hgeResourceManager * /*rm*/)
{
  if(!handle) {
    handle = reinterpret_cast<hgeResHandle>(new hgeStringTable(filename));
  }

  return handle;
}

void RStringTable::Free()
{
  if(handle) {
    delete reinterpret_cast<hgeStringTable *>(handle);
  }

  handle=0;
}

void RStringTable::copy_from(IResource *r)
{
  auto src = dynamic_cast<RStringTable *>(r);
  memcpy(filename, src->filename, sizeof(filename));
  IResource::copy_from_base(r);
}
