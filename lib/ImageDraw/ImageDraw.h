#pragma once

#include <AnimatedGIF.h>
#include <LittleFS.h>
#include <Matrix.h>

class ImageDraw {
 private:
  Matrix* matrix;
  AnimatedGIF gif;
  File f;
  int x_offset, y_offset;
  static ImageDraw* currentInstance;

  // Private static callback functions
    static void* GIFOpenFile(const char* fname, int32_t* pSize) {
        return currentInstance->GIFOpenFileImpl(fname, pSize);
    }

    static void GIFCloseFile(void* pHandle) {
        currentInstance->GIFCloseFileImpl(pHandle);
    }

    static int32_t GIFReadFile(GIFFILE* pFile, uint8_t* pBuf, int32_t iLen) {
        return currentInstance->GIFReadFileImpl(pFile, pBuf, iLen);
    }

    static int32_t GIFSeekFile(GIFFILE* pFile, int32_t iPosition) {
        return currentInstance->GIFSeekFileImpl(pFile, iPosition);
    }

    static void GIFDraw(GIFDRAW* pDraw) {
        currentInstance->GIFDrawImpl(pDraw);
    }

    // Actual implementation functions
    void* GIFOpenFileImpl(const char* fname, int32_t* pSize);
    void GIFCloseFileImpl(void* pHandle);
    int32_t GIFReadFileImpl(GIFFILE* pFile, uint8_t* pBuf, int32_t iLen);
    int32_t GIFSeekFileImpl(GIFFILE* pFile, int32_t iPosition);
    void GIFDrawImpl(GIFDRAW* pDraw);

 public:
  ImageDraw(Matrix* matrix);
  ~ImageDraw();


  void begin();
  bool openGIF(const char* name);
  void showGIF();
  void closeGIF();
};
