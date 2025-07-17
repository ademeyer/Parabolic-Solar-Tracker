#include "WMMLib.h"

/**
 *
 * @name: WMMHR grid program.
 * @brief: The Geomagnetism Library is used to make a command prompt program. The program prompts
 *          the user to enter a location, performs the computations and prints the results to the
 *          standard output. The program expects the files GeomagnetismLibrary.c, GeomagnetismHeader.h,
 *          EWMM.COF and EGM9615.h to be in the same directory.

* @authors: Manoj.C.Nair@Noaa.Gov, liyin.young@noaa.gov, adebayotimileyin@gmail.com
* @date: 13.07.2025
 */

static int MAG_Grid(MAGtype_CoordGeodetic CoordData,
                    MAGtype_MagneticModel *MagneticModel,
                    MAGtype_Geoid *Geoid,
                    MAGtype_Ellipsoid Ellip,
                    MAGtype_Date DateTime,
                    struct DecData *RecValue);

DecData getDeclinition(const InData *input)
{
  DecData decvalue;
  decvalue.errCode = NOERROR;

  // Check for Null input data
  if (!input)
  {
    decvalue.errCode = NULLERROR;
    return decvalue;
  }

  MAGtype_MagneticModel *MagneticModels[1];
  MAGtype_Ellipsoid Ellip;
  MAGtype_Geoid Geoid;
  int epochs = 1, i;

  char filename[] = "WMM.COF";

  if (!MAG_robustReadMagModels(filename, &MagneticModels, 1))
  {
    decvalue.errCode = FILEERROR;
    return decvalue;
  }

  MAG_SetDefaults(&Ellip, &Geoid);

  /* Set EGM96 Geoid parameters */
  Geoid.GeoidHeightBuffer = GeoidHeightBuffer;
  Geoid.Geoid_Initialized = 1;
  /* Set EGM96 Geoid parameters END */

  /* Use the Default Lat/Long, Altitude */
  MAGtype_CoordGeodetic cood;
  cood.phi = input->pos.Latitude;
  cood.lambda = input->pos.Longitude;
  cood.HeightAboveGeoid = input->pos.Altitude;

  // Use Above Mean Sea Level (MSL)
  Geoid.UseGeoid = 1;

  // Year
  MAGtype_Date startdate;
  startdate.DecimalYear = input->decimalYear;

  MAG_Grid(cood, MagneticModels[0], &Geoid, Ellip, startdate, &decvalue);

  for (i = 0; i < epochs; i++)
    MAG_FreeMagneticModelMemory(MagneticModels[i]);

  return decvalue;
}

static int MAG_Grid(MAGtype_CoordGeodetic CoordData, MAGtype_MagneticModel *MagneticModel, MAGtype_Geoid *Geoid, MAGtype_Ellipsoid Ellip, MAGtype_Date DateTime, struct DecData *RecValue)
{
  // Check DateTime is within Model Validity
  if (DateTime.DecimalYear < MagneticModel->min_year || DateTime.DecimalYear > MagneticModel->CoefficientFileEndDate)
  {
    RecValue->errCode = INPUTERROR;
    return FALSE;
  }

  int NumTerms;

  MAGtype_MagneticModel *TimedMagneticModel;
  MAGtype_CoordSpherical CoordSpherical;
  MAGtype_MagneticResults MagneticResultsSph, MagneticResultsGeo, MagneticResultsSphVar, MagneticResultsGeoVar;
  MAGtype_SphericalHarmonicVariables *SphVariables;
  MAGtype_GeoMagneticElements GeoMagneticElements, Errors;
  MAGtype_LegendreFunction *LegendreFunction;

  double min_wgsalt = -1;
  double max_wgsalt = 1900;

  NumTerms = ((MagneticModel->nMax + 1) * (MagneticModel->nMax + 2) / 2);
  TimedMagneticModel = MAG_AllocateModelMemory(NumTerms);
  LegendreFunction = MAG_AllocateLegendreFunctionMemory(NumTerms); /* For storing the ALF functions */
  SphVariables = MAG_AllocateSphVarMemory(MagneticModel->nMax);

  if (Geoid->UseGeoid == 1)
    MAG_ConvertGeoidToEllipsoidHeight(&CoordData, Geoid); /* This converts the height above mean sea level to height above the WGS-84 ellipsoid */
  else
    CoordData.HeightAboveEllipsoid = CoordData.HeightAboveGeoid;
#ifndef WMMHR
  if (CoordData.HeightAboveEllipsoid < min_wgsalt || CoordData.HeightAboveEllipsoid > max_wgsalt)
  {
    return FALSE;
  }
#endif
  MAG_GeodeticToSpherical(Ellip, CoordData, &CoordSpherical);
  MAG_ComputeSphericalHarmonicVariables(Ellip, CoordSpherical, MagneticModel->nMax, SphVariables); /* Compute Spherical Harmonic variables  */
  MAG_AssociatedLegendreFunction(CoordSpherical, MagneticModel->nMax, LegendreFunction);           /* Compute ALF  Equations 5-6, WMM Technical report*/

  MAG_TimelyModifyMagneticModel(DateTime, MagneticModel, TimedMagneticModel);                                       /*This modifies the Magnetic coefficients to the correct date. */
  MAG_Summation(LegendreFunction, TimedMagneticModel, *SphVariables, CoordSpherical, &MagneticResultsSph);          /* Accumulate the spherical harmonic coefficients Equations 10:12 , WMM Technical report*/
  MAG_SecVarSummation(LegendreFunction, TimedMagneticModel, *SphVariables, CoordSpherical, &MagneticResultsSphVar); /*Sum the Secular Variation Coefficients, Equations 13:15 , WMM Technical report  */
  MAG_RotateMagneticVector(CoordSpherical, CoordData, MagneticResultsSph, &MagneticResultsGeo);                     /* Map the computed Magnetic fields to Geodetic coordinates Equation 16 , WMM Technical report */
  MAG_RotateMagneticVector(CoordSpherical, CoordData, MagneticResultsSphVar, &MagneticResultsGeoVar);               /* Map the secular variation field components to Geodetic coordinates, Equation 17 , WMM Technical report*/
  MAG_CalculateGeoMagneticElements(&MagneticResultsGeo, &GeoMagneticElements);                                      /* Calculate the Geomagnetic elements, Equation 18 , WMM Technical report */
  MAG_CalculateGridVariation(CoordData, &GeoMagneticElements);
  MAG_CalculateSecularVariationElements(MagneticResultsGeoVar, &GeoMagneticElements); /*Calculate the secular variation of each of the Geomagnetic elements, Equation 19, WMM Technical report*/
#if WMMHR
  MAG_WMMHRErrorCalc(GeoMagneticElements.H, &Errors);
#else
  MAG_WMMErrorCalc(GeoMagneticElements.H, &Errors);
#endif

  MagComponents res, er;
  // Pass Value Result
  /**
   * @brief: Usage not yet defined for application
   *
  res.F = GeoMagneticElements.F;
  res.H = GeoMagneticElements.H;
  res.X = GeoMagneticElements.X;
  res.Y = GeoMagneticElements.Y;
  res.Z = GeoMagneticElements.Z;
  res.I = GeoMagneticElements.Incl;*/
  res.D = GeoMagneticElements.Decl;

  // Pass Error Result
  /**
   * @brief: Usage not yet defined for application
   *
  er.F = Errors.F;
  er.H = Errors.H;
  er.X = Errors.X;
  er.Y = Errors.Y;
  er.Z = Errors.Z;
  er.I = Errors.Incl;*/
  er.D = Errors.Decl;

  RecValue->magData = res;
  RecValue->magDataErr = er;

  /* Deallocate Memory */
  MAG_FreeMagneticModelMemory(TimedMagneticModel);
  MAG_FreeLegendreMemory(LegendreFunction);
  MAG_FreeSphVarMemory(SphVariables);

  RecValue->errCode = NOERROR;

  return TRUE;
} /*MAG_Grid*/
