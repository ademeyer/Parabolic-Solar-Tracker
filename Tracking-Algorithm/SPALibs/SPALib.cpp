#include "SPALib.h"

SunPositionData getSunPosition(const SPA_Input *input)
{
  SunPositionData outdata;
  if (!input)
  {
    outdata.errCode = 1;
    return outdata;
  }
  spa_data spa;
  float min, sec;

  // enter required input values into SPA structure
  spa.year = input->dateTime.dt.date.year;
  spa.month = input->dateTime.dt.date.month;
  spa.day = input->dateTime.dt.date.day;
  spa.hour = input->dateTime.dt.time.hour;
  spa.minute = input->dateTime.dt.time.minute;
  spa.second = input->dateTime.dt.time.second;
  spa.timezone = static_cast<double>(input->dateTime.dt.time.timezone);
  spa.delta_ut1 = input->dateTime.GetDelta_UT1();
  spa.delta_t = input->dateTime.GetDelta_T();
  spa.longitude = input->pos.Longitude;
  spa.latitude = input->pos.Latitude;
  spa.elevation = input->pos.Altitude;
  spa.pressure = input->weather.pressure;
  spa.temperature = input->weather.temp;
  /*
  Left this as default until I learn
  what it is and how to get it for diffrent location
  */
  spa.slope = 30;
  spa.azm_rotation = -10;
  spa.atmos_refract = 0.5667;
  spa.function = SPA_ALL;
  int err = 0;
  if ((err = spa_calculate(&spa)) != 0) // check for SPA errors
  {
    outdata.errCode = err;
    return outdata;
  }

  outdata.errCode = 0;
  outdata.elevation = 90.0 - spa.zenith;
  outdata.azimuth = spa.azimuth;
  outdata.incidence = spa.incidence;

  min = 60.0 * (spa.sunrise - (int)(spa.sunrise));
  sec = 60.0 * (min - (int)min);
  outdata.sunrise = Time(spa.sunrise, min, sec, input->dateTime.dt.time.timezone);

  min = 60.0 * (spa.sunset - (int)(spa.sunset));
  sec = 60.0 * (min - (int)min);
  outdata.sunset = Time(spa.sunset, min, sec, input->dateTime.dt.time.timezone);

  return outdata;
}