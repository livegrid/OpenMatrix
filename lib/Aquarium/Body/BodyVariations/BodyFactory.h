#ifndef BODYFACTORY_H
#define BODYFACTORY_H

#include <Arduino.h>
#include <Body/Body.h>
#include <Body/Fin.h>
#include <Body/Head.h>
#include <Body/Tail.h>
#include <Fish.h>
#include <Matrix.h>

#include <chrono>
#include <functional>
#include <random>
#include <vector>

#include "FishBody.h"
#include "SnakeBody.h"
#include "StarBody.h"
#include "TurtleBody.h"
#include "OctopusBody.h"

class BodyFactory {
 private:
  Matrix* matrix;
  std::vector<std::function<Fin*(Matrix*)>> finConstructors;
  std::vector<std::function<Head*(Matrix*)>> headConstructors;
  std::vector<std::function<Tail*(Matrix*)>> tailConstructors;
  std::vector<std::function<Body*(Matrix*, Head*, Tail*, Fin*)>>
      bodyConstructors;
  std::mt19937 gen;

  String bodyType;
  String motionType;
  String headType;
  String finType;
  String tailType;

 public:
  BodyFactory(Matrix* m)
      : matrix(m),
        gen(std::chrono::system_clock::now().time_since_epoch().count()) {
    // Register fin types
    // registerFinType([](Matrix* m) -> Fin* { return new noFin(m); });
    registerFinType([](Matrix* m) -> Fin* { return new TriangleFin(m); });
    registerFinType([](Matrix* m) -> Fin* { return new EllipseFin(m); });
    registerFinType([](Matrix* m) -> Fin* { return new LegFin(m); });
    registerFinType([](Matrix* m) -> Fin* { return new RoundFin(m); });
    // Register head types
    registerHeadType([](Matrix* m) -> Head* { return new TriangleHead(m); });
    registerHeadType([](Matrix* m) -> Head* { return new FrogHead(m); });
    registerHeadType([](Matrix* m) -> Head* { return new NeedleHead(m); });
    // Register tail types
    // registerTailType([](Matrix* m) -> Tail* { return new noTail(m); });
    registerTailType([](Matrix* m) -> Tail* { return new TriangleTail(m); });
    registerTailType([](Matrix* m) -> Tail* { return new CurvyTail(m); });
    registerTailType([](Matrix* m) -> Tail* { return new WavyTail(m); });
    
    // Register body types
    registerBodyType([](Matrix* m, Head* h, Tail* t, Fin* f) -> Body* {
      return new FishBody(m, h, t, f);
    });
    registerBodyType([](Matrix* m, Head* h, Tail* t, Fin* f) -> Body* {
      return new TurtleBody(m, h, t, f);
    });
    registerBodyType([](Matrix* m, Head* h, Tail* t, Fin* f) -> Body* {
      return new StarBody(m, h, t, f);
    });
    registerBodyType([](Matrix* m, Head* h, Tail* t, Fin* f) -> Body* {
      return new SnakeBody(m, h, t, f);
    });
    registerBodyType([](Matrix* m, Head* h, Tail* t, Fin* f) -> Body* {
      return new OctopusBody(m, h, t, f);
    });
  }

  void registerFinType(const std::function<Fin*(Matrix*)>& constructor) {
    finConstructors.push_back(constructor);
  }

  void registerHeadType(const std::function<Head*(Matrix*)>& constructor) {
    headConstructors.push_back(constructor);
  }

  void registerTailType(const std::function<Tail*(Matrix*)>& constructor) {
    tailConstructors.push_back(constructor);
  }

  void registerBodyType(
      const std::function<Body*(Matrix*, Head*, Tail*, Fin*)>& constructor) {
    bodyConstructors.push_back(constructor);
  }

  Fin* createRandomFin() {
    return createRandomComponent(finConstructors);
  }

  Head* createRandomHead() {
    return createRandomComponent(headConstructors);
  }

  Tail* createRandomTail() {
    return createRandomComponent(tailConstructors);
  }

  Head* createHead(const String& type) {
    if (type == "TriangleHead")
      return new TriangleHead(matrix);
    else if (type == "FrogHead")
      return new FrogHead(matrix);
    else if (type == "NeedleHead")
      return new NeedleHead(matrix);
    else
      return nullptr;
  }

  Tail* createTail(const String& type) {
    if (type == "noTail") {
      return new noTail(matrix);
    } else if (type == "TriangleTail") {
      return new TriangleTail(matrix);
    } else if (type == "CurvyTail") {
      return new CurvyTail(matrix);
    } else if (type == "WavyTail") {
      return new WavyTail(matrix);
    } else {
      return nullptr; 
    }
  }

  Fin* createFin(const String& type) {
    if (type == "TriangleFin") {
      return new TriangleFin(matrix);
    } else if (type == "EllipseFin") {
      return new EllipseFin(matrix);
    } else if (type == "LegFin") {
      return new LegFin(matrix);
    } else if (type == "RoundFin") {
      return new RoundFin(matrix);
    } else {
      return nullptr;
    }
  }

  Body* createBody(Head* head, Tail* tail, Fin* fin) {
    return new FishBody(matrix, head, tail, fin);
  }

  Body* createRandomBody() {
    if (bodyConstructors.empty())
      return nullptr;
    std::uniform_int_distribution<> dis(0, bodyConstructors.size() - 1);
    int index = dis(gen);
    return bodyConstructors[index](matrix, createRandomHead(),
                                   createRandomTail(), createRandomFin());
  }

  Body* createBody(const String& type, const String& headType,
                   const String& tailType, const String& finType) {
    Head* head = createHead(headType);
    Tail* tail = createTail(tailType);
    Fin* fin = createFin(finType);

    if (type == "Fish") {
      return new FishBody(matrix, head, tail, fin);
    } else if (type == "Turtle") {
      return new TurtleBody(matrix, head, tail, fin);
    } else if (type == "Star") {
      return new StarBody(matrix, head, tail, fin);
    } else if (type == "Snake") {
      return new SnakeBody(matrix, head, tail, fin);
    } else if (type == "Octopus") {
      return new OctopusBody(matrix, head, tail, fin);
    } else {
      // If the type is not recognized, return nullptr or a default body type
      delete head;
      delete tail;
      delete fin;
      return nullptr;
    }
  }

  Body* createBody(const String& type) {
    if (type == "Fish") {
      return new FishBody(matrix, createRandomHead(), createRandomTail(),
                          createRandomFin());
    } else if (type == "Turtle") {
      return new TurtleBody(matrix, createRandomHead(), createRandomTail(),
                            createRandomFin());
    } else if (type == "Star") {
      return new StarBody(matrix, createRandomHead(), createRandomTail(),
                          createRandomFin());
    } else if (type == "Snake") {
      return new SnakeBody(matrix, createRandomHead(), createRandomTail(),
                           createRandomFin());
    } else if (type == "Octopus") {
      return new OctopusBody(matrix, createRandomHead(), createRandomTail(),
                             createRandomFin());
    }
    else {
      return nullptr;
    }
  }

  Body* testBody() {
    return new TurtleBody(matrix, new FrogHead(matrix), new noTail(matrix),
                          new LegFin(matrix));
  }

  Fin* createFin() {
    return new TriangleFin(matrix);
  }

  Head* createHead() {
    return new TriangleHead(matrix);
  }

  Tail* createTail() {
    return new TriangleTail(matrix);
  }

 private:
  template <typename T>
  T* createRandomComponent(
      std::vector<std::function<T*(Matrix*)>>& constructors) {
    if (constructors.empty())
      return nullptr;
    std::uniform_int_distribution<> dis(0, constructors.size() - 1);
    int index = dis(gen);
    return constructors[index](matrix);
  }
};

#endif  // BODYFACTORY_H
