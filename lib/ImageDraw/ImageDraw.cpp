#include "ImageDraw.h"

ImageDraw* ImageDraw::currentInstance = nullptr;

ImageDraw::ImageDraw(Matrix* matrix) : matrix(matrix) {
  currentInstance = this;
}

ImageDraw::~ImageDraw() {
  gif.close();
}

void ImageDraw::begin() {
  gif.begin(LITTLE_ENDIAN_PIXELS);
}

void* ImageDraw::GIFOpenFileImpl(const char* fname, int32_t* pSize) {
  f = LittleFS.open(fname, "r");
  if (f) {
    *pSize = f.size();
    return (void*)&f;
  }
  return NULL;
}

void ImageDraw::GIFCloseFileImpl(void* pHandle) {
  File* f = static_cast<File*>(pHandle);
  if (f != NULL)
    f->close();
}

int32_t ImageDraw::GIFReadFileImpl(GIFFILE* pFile, uint8_t* pBuf,
                                   int32_t iLen) {
  int32_t iBytesRead;
  iBytesRead = iLen;
  File* f = static_cast<File*>(pFile->fHandle);
  if ((pFile->iSize - pFile->iPos) < iLen)
    iBytesRead = pFile->iSize - pFile->iPos - 1;
  if (iBytesRead <= 0)
    return 0;
  iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
  pFile->iPos = f->position();
  return iBytesRead;
}

int32_t ImageDraw::GIFSeekFileImpl(GIFFILE* pFile, int32_t iPosition) {
  File* f = static_cast<File*>(pFile->fHandle);
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  return pFile->iPos;
}

void ImageDraw::GIFDrawImpl(GIFDRAW* pDraw) {
  uint8_t* s;
  uint16_t *d, *usPalette, usTemp[320];
  int x, y, iWidth;

  iWidth = pDraw->iWidth;
  if (iWidth > matrix->getXResolution())
    iWidth = matrix->getXResolution();

  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y;

  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) {
    for (x = 0; x < iWidth; x++) {
      if (s[x] == pDraw->ucTransparent)
        s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }

  if (pDraw->ucHasTransparency) {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    int x, iCount;
    pEnd = s + pDraw->iWidth;
    x = 0;
    iCount = 0;
    while (x < pDraw->iWidth) {
      c = ucTransparent - 1;
      d = usTemp;
      while (c != ucTransparent && s < pEnd) {
        c = *s++;
        if (c == ucTransparent) {
          s--;
        } else {
          *d++ = usPalette[c];
          iCount++;
        }
      }
      if (iCount) {
        for (int xOffset = 0; xOffset < iCount; xOffset++) {
          matrix->background->drawPixel(x + xOffset, y, usTemp[xOffset]);
        }
        x += iCount;
        iCount = 0;
      }
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd) {
        c = *s++;
        if (c == ucTransparent)
          iCount++;
        else
          s--;
      }
      if (iCount) {
        x += iCount;
        iCount = 0;
      }
    }
  } else {
    s = pDraw->pPixels;
    for (x = 0; x < pDraw->iWidth; x++) {
      matrix->background->drawPixel(x, y, usPalette[*s++]);
    }
  }
}

bool ImageDraw::openGIF(const char* name) {
  char fullPath[64];
  snprintf(fullPath, sizeof(fullPath), "/img/%s", name);
  
  if (gif.open(fullPath, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw)) {
    x_offset = (matrix->getXResolution() - gif.getCanvasWidth()) / 2;
    if (x_offset < 0)
      x_offset = 0;
    y_offset = (matrix->getYResolution() - gif.getCanvasHeight()) / 2;
    if (y_offset < 0)
      y_offset = 0;
    return true;
  } else {
    log_e("Failed to open GIF: %s. Error code: %d", fullPath, gif.getLastError());
    return false;
  }
}

void ImageDraw::showGIF() {
  if (!gif.playFrame(true, NULL)) {
    gif.reset();
  }
}

void ImageDraw::closeGIF() {
  gif.close();
}