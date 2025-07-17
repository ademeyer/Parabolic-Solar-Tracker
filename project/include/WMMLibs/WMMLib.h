#pragma once
#include "IDateTime.h"
#include "IMUSensor.h"
#include "IGPSSensor.h"

/*************************** USER INPUT DATA ***************************************/
struct InData
{
  double decimalYear;
  struct Position pos;
};
/*************************** END USER INPUT DATA ***********************************/

/*************************** USER OUTPUT DATA **************************************/
enum ERROCODE
{
  NOERROR = 0,
  INPUTERROR,
  FILEERROR,
  MEMERROR,
  NULLERROR,
};
struct MagComponents
{
  /**
   * @brief: Usage not yet defined for application
   *
  double F; // Total Intensity of the geomagnetic field
  double H; // Horizontal Intensity of the geomagnetic field
  double X; // North Component of the geomagnetic field
  double Y; // East Component of the geomagnetic field
  double Z; // Vertical Component of the geomagnetic field
  double I; // Geomagnetic Inclination*/
  double D; // Geomagnetic Declination (Magnetic Variation)
};

struct DecData
{
  int errCode;
  MagComponents magData;
  MagComponents magDataErr;
  double sv; /* Secular variable / Annual Changes (in nT) */
};

/*************************** END USER OUTPUT DATA ***********************************/

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
  DecData getDeclinition(const InData *input);

#ifdef __cplusplus
} // extern "C"
#endif