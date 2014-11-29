#include "hge_portable.h"
#include <stdio.h>

namespace hgeos {
  Finder::Finder(const char *apppath) {
    strcpy(szAppPath, apppath);
  }

  Finder::~Finder() {
#ifdef HGE_WINDOWS
    FindClose(m_search);
#else
    if (hSearch) {
      closedir(m_search);
      hSearch = 0;
    }
#endif
  }

  // !!! FIXME: kinda messy, and probably doesn't get all the corner cases right.
  bool Finder::_WildcardMatch(const char *str, const char *wildcard)
  {
    if ((str == nullptr) || (wildcard == nullptr)) {
      return false;
    }

    while ((*str) && (*wildcard)) {
      const char wildch = *wildcard;
      const char strch = *str;

      if (wildch == '?')
        ; // okay.
      else if (wildch == '*') {
        do {
          wildcard++;
        } while (((*wildcard == '*') || (*wildcard == '?')) && (*wildcard != '\0'));

        const char newwild = *wildcard;

        if (newwild == '\0') {
          return true;
        }

        const char *ptr = str;

        while (*ptr) { // find the greediest match possible...
          if (*ptr == newwild) {
            str = ptr;
          }

          ptr++;
        }
      }
      else if ((toupper(strch)) != (toupper(wildch))) {
        return false;
      }

      str++;
      wildcard++;
    }

    while (*wildcard == '*') {
      wildcard++;
    }

    return ((*str == '\0') && (*wildcard == '\0'));
  }

  bool Finder::_PrepareFileEnum(const char *wildcard)
  {
    if (m_search) {
#ifdef HGE_WINDOWS
      FindClose(m_search);
      m_search = nullptr;
#else
      closedir(m_search);
      hSearch = 0;
#endif
    }

    char *madepath = Resource_MakePath(wildcard);
    const char *fname = strrchr(madepath, '/');
    const char *dir = nullptr;

    if (fname == nullptr) {
      dir = ".";
      fname = madepath;
    }
    else {
      dir = madepath;
      char *ptr = const_cast<char *>(fname); // what is this? remove const?
      *ptr = '\0';  // split dir and filename.
      fname++;
    }

    strcpy(szSearchDir, dir);
    strcpy(szSearchWildcard, fname);

#ifdef HGE_WINDOWS
    m_search = FindFirstFile(dir, &m_fdfile);
    return (m_search != INVALID_HANDLE_VALUE);
#else
    m_search = opendir(dir);
    return (m_search != 0);
#endif
  }

  char *Finder::Resource_MakePath(const char *filename)
  {
    int i;

    if (!filename) {
      strcpy(szTmpFilename, szAppPath);
    }
    else if (filename[0] == '\\' || filename[0] == '/' || filename[1] == ':') {
      strcpy(szTmpFilename, filename);
    }
    else {
      strcpy(szTmpFilename, szAppPath);

      if (filename) {
        strcat(szTmpFilename, filename);
      }
    }

    for (i = 0; szTmpFilename[i]; i++) {
      if (szTmpFilename[i] == '\\') {
        szTmpFilename[i] = '/';
      }
    }

    locateCorrectCase(szTmpFilename);

    return szTmpFilename;
  }


  char *Finder::_DoEnumIteration(const bool wantdir)
  {
    if (!m_search) {
      return 0;
    }

    while (true) {
      struct dirent *dent = readdir(m_search);

      if (dent == nullptr) {
        closedir(m_search);
        hSearch = 0;
        return 0;
      }

      if ((strcmp(dent->d_name, ".") == 0) || (strcmp(dent->d_name, "..") == 0)) {
        continue;
      }

      if (!_WildcardMatch(dent->d_name, szSearchWildcard)) {
        continue;
      }

      char fullpath[_MAX_PATH];
      snprintf(fullpath, sizeof(fullpath), "%s/%s", szSearchDir, dent->d_name);
      struct stat statbuf;

      if (stat(fullpath, &statbuf) == -1) { // this follows symlinks.
        continue;
      }

      const bool isdir = ((S_ISDIR(statbuf.st_mode)) != 0);

      if (isdir == wantdir) { // this treats pipes, devs, etc, as "files" ...
        strcpy(szSearchResult, dent->d_name);
        return szSearchResult;
      }
    }

    //return 0;
  }

  // this is from PhysicsFS originally ( http://icculus.org/physfs/ )
  //  (also zlib-licensed.)
  //static 
  int Finder::locateOneElement(char *buf)
  {
    char *ptr = nullptr;
    DIR *dirp = nullptr;
    struct dirent *dent = nullptr;

    if (access(buf, F_OK) == 0) {
      return 1;  /* quick rejection: exists in current case. */
    }

    ptr = strrchr(buf, '/');  /* find entry at end of path. */

    if (ptr == nullptr) {
      dirp = opendir(".");
      ptr = buf;
    }
    else {
      *ptr = '\0';
      dirp = opendir(buf);
      *ptr = '/';
      ptr++;  /* point past dirsep to entry itself. */
    }

    while ((dent = readdir(dirp)) != nullptr) {
      if (strcasecmp(dent->d_name, ptr) == 0) {
        strcpy(ptr, dent->d_name); /* found a match. Overwrite with this case. */
        closedir(dirp);
        return 1;
      }
    }

    /* no match at all... */
    closedir(dirp);
    return 0;
  }

  //static 
  int Finder::locateCorrectCase(char *buf)
  {
    char *ptr = buf;
    //char *prevptr = buf;

    while ((ptr = strchr(ptr + 1, '/'))) {
      *ptr = '\0';  /* block this path section off */

      if (!locateOneElement(buf)) {
        *ptr = '/'; /* restore path separator */
        return -2;  /* missing element in path. */
      }

      *ptr = '/'; /* restore path separator */
    }

    /* check final element... */
    return locateOneElement(buf) ? 0 : -1;
  }


  int c99_snprintf(char* str, size_t size, const char* format, ...)
  {
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(str, size, format, ap);
    va_end(ap);

    return count;
  }

  int c99_vsnprintf(char* str, size_t size, const char* format, va_list ap)
  {
    int count = -1;

    if (size != 0)
      count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
    if (count == -1)
      count = _vscprintf(format, ap);

    return count;
  }

} // ns hgeos