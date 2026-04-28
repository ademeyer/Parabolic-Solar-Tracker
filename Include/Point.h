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
  Point3f operator+(const Point3f &rhs) const
  {
    Point3f ret = *this;
    ret.X += rhs.X;
    ret.Y += rhs.Y;
    ret.Z += rhs.Z;
    return ret;
  }

  Point3f operator-(const Point3f &rhs) const
  {
    Point3f ret = *this;
    ret.X -= rhs.X;
    ret.Y -= rhs.Y;
    ret.Z -= rhs.Z;
    return ret;
  }

  Point3f operator*(const double &scalar) const
  {
    Point3f ret = *this;
    ret.X *= scalar;
    ret.Y *= scalar;
    ret.Z *= scalar;
    return ret;
  }

  Point3f &operator/=(const double &scalar)
  {
    this->X /= scalar;
    this->Y /= scalar;
    this->Z /= scalar;
    return *this;
  }

  Point3f &operator+=(const Point3f &rhs)
  {
    this->X += rhs.X;
    this->Y += rhs.Y;
    this->Z += rhs.Z;
    return *this;
  }

  Point3f &operator-=(const Point3f &rhs)
  {
    this->X -= rhs.X;
    this->Y -= rhs.Y;
    this->Z -= rhs.Z;
    return *this;
  }

  double Dot(const Point3f &rhs) const
  {
    return this->X * rhs.X + this->Y * rhs.Y + this->Z * rhs.Z;
  }

  Point3f Cross(const Point3f &rhs) const
  {
    return Point3f(this->Y * rhs.Z - this->Z * rhs.Y,
                   this->Z * rhs.X - this->X * rhs.Z,
                   this->X * rhs.Y - this->Y * rhs.X);
  }

  double Norm() const
  {
    return std::sqrt(X * X + Y * Y + Z * Z);
  }

  Point3f Normalized() const
  {
    double norm = this->Norm();
    if (norm == 0)
      return Point3f(0, 0, 0);
    return Point3f(X / norm, Y / norm, Z / norm);
  }

  Point3f() : X(-MAXFLOAT), Y(-MAXFLOAT), Z(-MAXFLOAT) {}
  Point3f(const double &x, const double &y, const double &z) : X(x), Y(y), Z(z) {}
  Point3f(const Point2f &xy, const double &z) : X(xy.X), Y(xy.Y), Z(z) {}
};
