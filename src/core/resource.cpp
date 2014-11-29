/*
** Haaf's Game Engine 1.8
** Copyright (C) 2003-2007, Relish Games
** hge.relishgames.com
**
** Core functions implementation: resources management
*/

#include "hge_impl.h"

#ifndef HGE_WINDOWS
static void strupr(char *s)
{
  while (*s) {
    *s = static_cast<char>(toupper(*s));
    s++;
  }
}
#endif

#include <zlib/zlib.h>

//#define NOCRYPT
//#define NOUNCRYPT
#include <zlib/contrib/minizip/unzip.h>


bool CALL HGE_Impl::Resource_AttachPack(const char *filename, const char *password)
{
  char *szName;
  CResourceList *resItem = res;
  unzFile zip;

  szName = Resource_MakePath(filename);
  strupr(szName);

  while (resItem) {
    if (!strcmp(szName, resItem->filename)) {
      return false;
    }

    resItem = resItem->next;
  }

  zip = unzOpen(szName);

  if (!zip) {
    return false;
  }

  unzClose(zip);

  resItem = new CResourceList;
  strcpy(resItem->filename, szName);

  if (password) {
    strcpy(resItem->password, password);
  } else {
    resItem->password[0] = 0;
  }

  resItem->next = res;
  res = resItem;

  return true;
}

void CALL HGE_Impl::Resource_RemovePack(const char *filename)
{
  char *szName;
  CResourceList *resItem = res, *resPrev = 0;

  szName = Resource_MakePath(filename);
  strupr(szName);

  while (resItem) {
    if (!strcmp(szName, resItem->filename)) {
      if (resPrev) {
        resPrev->next = resItem->next;
      } else {
        res = resItem->next;
      }

      delete resItem;
      break;
    }

    resPrev = resItem;
    resItem = resItem->next;
  }
}

void CALL HGE_Impl::Resource_RemoveAllPacks()
{
  CResourceList *resItem = res, *resNextItem;

  while (resItem) {
    resNextItem = resItem->next;
    delete resItem;
    resItem = resNextItem;
  }

  res = 0;
}

hgeResHandle CALL HGE_Impl::Resource_Load(const char *filename, uint32_t *size)
{
  const char *res_err = "Can't load resource: %s";

  CResourceList *resItem = res;
  char szName[_MAX_PATH];
  char szZipName[_MAX_PATH];
  unzFile zip;
  unz_file_info file_info;
  int done, i;
  void *ptr;
  FILE *hF;

  if (filename[0] == '\\' || filename[0] == '/' || filename[1] == ':') {
    goto _fromfile;  // skip absolute paths
  }

  // Load from pack

  strcpy(szName, filename);
  strupr(szName);

  for (i = 0; szName[i]; i++) {
    if (szName[i] == '/') {
      szName[i] = '\\';
    }
  }

  while (resItem) {
    zip = unzOpen(resItem->filename);
    done = unzGoToFirstFile(zip);

    while (done == UNZ_OK) {
      unzGetCurrentFileInfo(zip, &file_info, szZipName, sizeof(szZipName), nullptr, 0, nullptr, 0);
      strupr(szZipName);

      for (i = 0; szZipName[i]; i++) {
        if (szZipName[i] == '/') {
          szZipName[i] = '\\';
        }
      }

      if (!strcmp(szName, szZipName)) {
        if (unzOpenCurrentFilePassword(zip, resItem->password[0] ? resItem->password : 0) != UNZ_OK) {
          unzClose(zip);
          sprintf(szName, res_err, filename);
          _PostError(szName);
          return 0;
        }

        ptr = malloc(file_info.uncompressed_size);

        if (!ptr) {
          unzCloseCurrentFile(zip);
          unzClose(zip);
          sprintf(szName, res_err, filename);
          _PostError(szName);
          return 0;
        }

        if (unzReadCurrentFile(
              zip, ptr, static_cast<uint32_t>(file_info.uncompressed_size)) < 0) {
          unzCloseCurrentFile(zip);
          unzClose(zip);
          free(ptr);
          sprintf(szName, res_err, filename);
          _PostError(szName);
          return 0;
        }

        unzCloseCurrentFile(zip);
        unzClose(zip);

        if (size) {
          *size = static_cast<uint32_t>(file_info.uncompressed_size);
        }

        return reinterpret_cast<hgeResHandle>(ptr);
      }

      done = unzGoToNextFile(zip);
    }

    unzClose(zip);
    resItem = resItem->next;
  }

  // Load from file
_fromfile:

  hF = fopen(Resource_MakePath(filename), "rb");

  if (hF == nullptr) {
    sprintf(szName, res_err, filename);
    _PostError(szName);
    return 0;
  }

  struct stat statbuf;

  if (fstat(fileno(hF), &statbuf) == -1) {
    fclose(hF);
    sprintf(szName, res_err, filename);
    _PostError(szName);
    return 0;
  }

  file_info.uncompressed_size = static_cast<size_t>(statbuf.st_size);
  ptr = malloc(file_info.uncompressed_size);

  if (!ptr) {
    fclose(hF);
    sprintf(szName, res_err, filename);
    _PostError(szName);
    return 0;
  }

  if (fread(ptr, file_info.uncompressed_size, 1, hF) != 1) {
    fclose(hF);
    free(ptr);
    sprintf(szName, res_err, filename);
    _PostError(szName);
    return 0;
  }

  fclose(hF);

  if (size) {
    *size = static_cast<uint32_t>(file_info.uncompressed_size);
  }

  return reinterpret_cast<hgeResHandle>(ptr);
}


void CALL HGE_Impl::Resource_Free(void *res0)
{
  if (res0) {
    free(res0);
  }
}

char *CALL HGE_Impl::Resource_MakePath(const char *filename)
{
  return m_file_finder.Resource_MakePath(filename);
}

char *CALL HGE_Impl::Resource_EnumFiles(const char *wildcard)
{
  if (wildcard) {
    if (!m_file_finder._PrepareFileEnum(wildcard)) {
      return 0;
    }
  }

  return m_file_finder._DoEnumIteration(false);
}

char *CALL HGE_Impl::Resource_EnumFolders(const char *wildcard)
{
  if (wildcard) {
    if (!m_file_finder._PrepareFileEnum(wildcard)) {
      return 0;
    }
  }

  return m_file_finder._DoEnumIteration(true);
}
