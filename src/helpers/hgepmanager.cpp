// PLEASE NOTE that this is not the 1.81 version of hgeparticle.cpp ...
//  the game I'm working on used an older HGE that breaks with the 1.81
//  particle system. If you want 1.81, just overwrite this file.  --ryan.

/*
** Haaf's Game Engine 1.7
** Copyright (C) 2003-2007, Relish Games
** hge.relishgames.com
**
** hgeParticleManager helper class implementation
*/


#include "../../include/hgeparticle.h"

#include "../common/hge_utils.h"
using namespace hgeut;

hgeParticleManager::hgeParticleManager(const float fps)
{
  nPS=0;
  fFPS=fps;
  tX=tY=0.0f;
}

hgeParticleManager::~hgeParticleManager()
{
  int i;

  for(i=0; i<nPS; i++) {
    delete psList[i];
  }
}

void hgeParticleManager::Update(float dt)
{
  int i;

  for(i=0; i<nPS; i++) {
    psList[i]->Update(dt);

    if(flt_equal(psList[i]->GetAge(), -2.0f)
       && psList[i]->GetParticlesAlive()==0) {
      delete psList[i];
      psList[i]=psList[nPS-1];
      nPS--;
      i--;
    }
  }
}

void hgeParticleManager::Render()
{
  int i;

  for(i=0; i<nPS; i++) {
    psList[i]->Render();
  }
}

hgeParticleSystem* hgeParticleManager::SpawnPS(hgeParticleSystemInfo *psi, float x, float y)
{
  if(nPS==MAX_PSYSTEMS) {
    return 0;
  }

  psList[nPS]=new hgeParticleSystem(psi,fFPS);
  psList[nPS]->FireAt(x,y);
  psList[nPS]->Transpose(tX,tY);
  nPS++;
  return psList[nPS-1];
}

bool hgeParticleManager::IsPSAlive(hgeParticleSystem *ps) const
{
  int i;

  for(i=0; i<nPS; i++) if(psList[i]==ps) {
      return true;
    }

  return false;
}

void hgeParticleManager::Transpose(float x, float y)
{
  int i;

  for(i=0; i<nPS; i++) {
    psList[i]->Transpose(x,y);
  }

  tX=x;
  tY=y;
}

void hgeParticleManager::KillPS(hgeParticleSystem *ps)
{
  int i;

  for(i=0; i<nPS; i++) {
    if(psList[i]==ps) {
      delete psList[i];
      psList[i]=psList[nPS-1];
      nPS--;
      return;
    }
  }
}

void hgeParticleManager::KillAll()
{
  int i;

  for(i=0; i<nPS; i++) {
    delete psList[i];
  }

  nPS=0;
}
