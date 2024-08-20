#pragma once

#include "Effect.h"

class TemplateEffect : public Effect {
private:
  float speed = 0.02;
// Declare private variables here

public:
  TemplateEffect(Matrix* m);

  // Declare public methods here
  void update() override;

  void reset() override;
  
  const char* getName() const override;
};