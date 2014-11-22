/*
 * File:  ximabmp.h
 * Purpose: BMP Image Class Loader and Writer
 */
/* ==========================================================
 * CxImageBMP (c) 07/Aug/2001 Davide Pizzolato - www.xdp.it
 * For conditions of distribution and use, see copyright notice in ximage.h
 *
 * Special thanks to Troels Knakkergaard for new features, enhancements and bugfixes
 *
 * original CImageBMP  and CImageIterator implementation are:
 * Copyright: (c) 1995, Alejandro Aguilar Sierra <asierra(at)servidor(dot)unam(dot)mx>
 *
 * ==========================================================
 */

#if !defined(__ximaBMP_h)
#define __ximaBMP_h

#include "ximage.h"

const int32_t RLE_COMMAND     = 0;
const int32_t RLE_ENDOFLINE   = 0;
const int32_t RLE_ENDOFBITMAP = 1;
const int32_t RLE_DELTA       = 2;

#if !defined(BI_RLE8)
#define BI_RLE8  1L
#endif
#if !defined(BI_RLE4)
#define BI_RLE4  2L
#endif

#if CXIMAGE_SUPPORT_BMP

class CxImageBMP: public CxImage {
public:
  CxImageBMP(): CxImage(CXIMAGE_FORMAT_BMP) {}
  virtual ~CxImageBMP();

  bool Decode(CxFile * hFile);
  bool Decode(FILE *hFile)
  {
    CxIOFile file(hFile);
    return Decode(&file);
  }

#if CXIMAGE_SUPPORT_ENCODE
  bool Encode(CxFile * hFile);
  bool Encode(FILE *hFile)
  {
    CxIOFile file(hFile);
    return Encode(&file);
  }
#endif // CXIMAGE_SUPPORT_ENCODE

protected:
  bool DibReadBitmapInfo(CxFile* fh, BITMAPINFOHEADER *pdib);
};

#define BFT_ICON   0x4349   /* 'IC' */
#define BFT_BITMAP 0x4d42   /* 'BM' */
#define BFT_CURSOR 0x5450   /* 'PT' */

//#ifndef WIDTHBYTES
inline uint32_t WIDTHBYTES(uint32_t i) {
  return (static_cast<uint32_t>((i+31) & static_cast<uint32_t>(~31))/8);  /* ULONG aligned ! */
}
//#endif

#endif

inline uint32_t DibWidthBytesN(BITMAPINFOHEADER *lpbi, uint32_t n) {
  return static_cast<uint32_t>(
        WIDTHBYTES(static_cast<uint32_t>(lpbi->biWidth) * n)
        );
}
inline uint32_t DibWidthBytes(BITMAPINFOHEADER *lpbi) {
  return DibWidthBytesN(lpbi, (lpbi)->biBitCount);
}

inline uint32_t DibSizeImage(BITMAPINFOHEADER *lpbi) {
  return (lpbi->biSizeImage == 0
    ? (static_cast<uint32_t>(DibWidthBytes(lpbi))
       * static_cast<uint32_t>(lpbi->biHeight))
    : lpbi->biSizeImage);
}

inline uint32_t DibNumColors(BITMAPINFOHEADER *lpbi) {
  return (lpbi->biClrUsed == 0 && lpbi->biBitCount <= 8
          ? static_cast<uint32_t>(1 << static_cast<uint32_t>(lpbi->biBitCount))
          : static_cast<uint32_t>(lpbi->biClrUsed));
}

inline void FixBitmapInfo(BITMAPINFOHEADER *lpbi) {
  if (lpbi->biSizeImage == 0) {
    lpbi->biSizeImage = DibSizeImage(lpbi);
  }
  if (lpbi->biClrUsed == 0) {
    lpbi->biClrUsed = DibNumColors(lpbi);
  }
}
 
#endif
