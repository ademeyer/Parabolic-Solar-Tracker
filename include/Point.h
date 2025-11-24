#pragma once
#include <iostream>
struct Point2f
{
  double X;
  double Y;
  constexpr operator bool() const { return !((X == -MAXFLOAT || Y == -MAXFLOAT) ||
                                             (std::isnan(X) || std::isnan(Y)) ||
                                             (std::isinf(X) || std::isinf(Y))); }
  Point2f() : X(0.0), Y(0.0) {}
  Point2f(const double &x, const double &y) : X(x), Y(y) {}
};

struct Point3f
{
  double X;
  double Y;
  double Z;
  constexpr operator bool() const { return !((X == -MAXFLOAT || Y == -MAXFLOAT || Z == -MAXFLOAT) ||
                                             (std::isnan(X) || std::isnan(Y) || std::isnan(Z)) ||
                                             (std::isinf(X) || std::isinf(Y) || std::isinf(Z))); }
  Point3f() : X(-MAXFLOAT), Y(-MAXFLOAT), Z(-MAXFLOAT) {}
  Point3f(const double &x, const double &y, const double &z) : X(x), Y(y), Z(z) {}
  auto operator<=>(const Point3f &) const = default;
};
