#ifndef EFFECT_H
#define EFFECT_H

#include "../Matrix.h"

class Effect {
public:
    Effect(Matrix* matrix) : m_matrix(matrix) {}
    virtual ~Effect() {}

    virtual void update() = 0;
    virtual const char* getName() const = 0;

protected:
    Matrix* m_matrix;
};

#endif // EFFECT_H