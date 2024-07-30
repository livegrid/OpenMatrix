/*
 * Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// This is a modified version of the original class

#ifndef Vector_H
#define Vector_H

template <class T>
class Vector2 {
public:
    T x, y;

    Vector2() :x(0), y(0) {}
    Vector2(T x, T y) : x(x), y(y) {}
    Vector2(const Vector2& v) : x(v.x), y(v.y) {}

    Vector2& operator=(const Vector2& v) {
        x = v.x;
        y = v.y;
        return *this;
    }
    
    bool isEmpty() {
        return x == 0 && y == 0;
    }

    bool operator==(const Vector2& v) const {
        return x == v.x && y == v.y;
    }

    bool operator!=(const Vector2& v) const {
        return x != v.x || y != v.y;
    }

    Vector2 operator+(const Vector2& v) const {
        return Vector2(x + v.x, y + v.y);
    }

    Vector2 operator-(const Vector2& v) const {
        return Vector2(x - v.x, y - v.y);
    }

    Vector2& operator+=(const Vector2& v) {
        x += v.x;
        y += v.y;
        return *this;
    }

    Vector2& operator-=(const Vector2& v) {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    Vector2 operator+(double s) const {
        return Vector2(x + s, y + s);
    }
    Vector2 operator-(double s) const {
        return Vector2(x - s, y - s);
    }
    Vector2 operator*(double s) const {
        return Vector2(x * s, y * s);
    }

    Vector2 operator/(double s) const {
        return Vector2(x / s, y / s);
    }
    
    Vector2& operator+=(double s) {
        x += s;
        y += s;
        return *this;
    }
    Vector2& operator-=(double s) {
        x -= s;
        y -= s;
        return *this;
    }
    Vector2& operator*=(double s) {
        x *= s;
        y *= s;
        return *this;
    }
    Vector2& operator/=(double s) {
        x /= s;
        y /= s;
        return *this;
    }

    void set(T x, T y) {
        this->x = x;
        this->y = y;
    }

    void rotate(double deg) {
        double c = cos(deg);
        double s = sin(deg);
        double tx = x * c - y * s;
        double ty = x * s + y * c;
        x = tx;
        y = ty;
    }

    Vector2& normalize() {
        if (length() == 0) return *this;
        *this *= (1.0 / length());
        return *this;
    }

    float dist(Vector2 v) const {
        Vector2 d(v.x - x, v.y - y);
        return d.length();
    }
    float length() const {
        return sqrt(x * x + y * y);
    }

    float mag() const {
        return length();
    }

    float magSq() {
        return (x * x + y * y);
    }

    void truncate(double length) {
        double angle = atan2f(y, x);
        x = length * cos(angle);
        y = length * sin(angle);
    }

    Vector2 ortho() const {
        return Vector2(y, -x);
    }

    float heading() {
      return atan2f(y, x);
    }

    static float dot(Vector2 v1, Vector2 v2) {
        return v1.x * v2.x + v1.y * v2.y;
    }
    static float cross(Vector2 v1, Vector2 v2) {
        return (v1.x * v2.y) - (v1.y * v2.x);
    }

    static Vector2 fromAngle(float angle) {
        return Vector2(cos(angle), sin(angle));
    }

    void limit(float max) {
        if (magSq() > max*max) {
            normalize();
            *this *= max;
        }
    }

    void setMag(float magnitude) {
        normalize();
        *this *= magnitude;
    }

    void setAngle(float angle) {
        float diff = angle - heading();
        rotate(diff);
    }

    Vector2 lerp(const Vector2& start, const Vector2& end, float t) const {
        float x = (1 - t) * start.x + t * end.x;
        float y = (1 - t) * start.y + t * end.y;
        return Vector2(x, y);
    }
};

typedef Vector2<float> PVector;

#endif
