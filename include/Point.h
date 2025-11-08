#pragma once

struct Point2f
{
  double X;
  double Y;
  bool IsValid() const { return !(X == 0.0 && X == Y); }
  Point2f() : X(0.0), Y(0.0) {}
  Point2f(const double &x, const double &y) : X(x), Y(y) {}
};

struct Point3f
{
  double X;
  double Y;
  double Z;
  bool IsValid() const { return !(X == 0.0 && X == Y && Y == Z); }
  Point3f() : X(0.0), Y(0.0), Z(0.0) {}
  Point3f(const double &x, const double &y, const double &z) : X(x), Y(y), Z(z) {}
};
