#pragma once
#include <cmath>
struct Point2f
{
  double X;
  double Y;
  auto operator<=>(const Point2f &) const = default;
  constexpr operator bool() const { return !((X == -MAXFLOAT || Y == -MAXFLOAT) ||
                                             (std::isnan(X) || std::isnan(Y)) ||
                                             (std::isinf(X) || std::isinf(Y))); }
  Point2f() : X(-MAXFLOAT), Y(-MAXFLOAT) {}
  Point2f(const double &x, const double &y) : X(x), Y(y) {}
};

struct Point3f
{
  double X;
  double Y;
  double Z;
  auto operator<=>(const Point3f &) const = default;
  constexpr operator bool() const { return !((X == -MAXFLOAT || Y == -MAXFLOAT || Z == -MAXFLOAT) ||
                                             (std::isnan(X) || std::isnan(Y) || std::isnan(Z)) ||
                                             (std::isinf(X) || std::isinf(Y) || std::isinf(Z))); }
  Point3f() : X(-MAXFLOAT), Y(-MAXFLOAT), Z(-MAXFLOAT) {}
  Point3f(const double &x, const double &y, const double &z) : X(x), Y(y), Z(z) {}
};
