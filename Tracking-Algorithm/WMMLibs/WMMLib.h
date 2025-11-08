#pragma once
#include "ISensor.h"
#include "ICompute.h"

enum ERROCODE
{
  NOERROR = 0,
  INPUTERROR,
  FILEERROR,
  MEMERROR,
  NULLERROR,
};

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "GeomagnetismHeader.h"
#include "EGM9615.h"
#include "version.h"
#include "GeomagInterativeLib.h"

  // Forward Declaration
  GeoMagneticData getDeclinition(const InData *input);

#ifdef __cplusplus
} // extern "C"
#endif