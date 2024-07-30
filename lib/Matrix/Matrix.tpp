#pragma once

#include "Matrix.h"

template <typename T>
void Matrix::drawText(const T& text) {
  if constexpr (std::is_same_v<T, std::string>) {
    matrix->print(text.c_str());
  } else {
    matrix->print(text);
  }
}

template <typename... Args>
void Matrix::drawTextf(const char* format, Args... args) {
  matrix->printf(format, args...);
}

template <typename T>
void Matrix::drawText(const T& text, uint8_t r, uint8_t g, uint8_t b) {
  matrix->setTextColor(matrix->color565(r, g, b));
  if constexpr (std::is_same_v<T, std::string>) {
    matrix->print(text.c_str());
  } else {
    matrix->print(text);
  }
}
