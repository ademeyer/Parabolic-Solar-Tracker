#include "SPALib.h"

SPA_Output getSunPosition(const SPA_Input *input)
{
  SPA_Output outdata;
  if (!input)
  {
    outdata.errCode = 1;
    return outdata;
  }
  spa_data spa;
  float min, sec;

  // enter required input values into SPA structure
  spa.year = input->dateTime.dt.year;
  spa.month = input->dateTime.dt.month;
  spa.day = input->dateTime.dt.day;
  spa.hour = input->dateTime.tt.hour;
  spa.minute = input->dateTime.tt.minute;
  spa.second = input->dateTime.tt.second;
  spa.timezone = input->dateTime.tt.timezone;
  spa.delta_ut1 = input->dateTime.GetDelta_UT1();
  spa.delta_t = input->dateTime.GetDelta_T();
  spa.longitude = input->pos.Longitude;
  spa.latitude = input->pos.Latitude;
  spa.elevation = input->pos.Altitude;
  spa.pressure = input->weather.presure;
  spa.temperature = input->weather.temp;
  /*
  Left this as default until I learn
  what it is and how to get it for diffrent location
  */
  spa.slope = 30;
  spa.azm_rotation = -10;
  spa.atmos_refract = 0.5667;
  spa.function = SPA_ALL;

  if (spa_calculate(&spa) != 0) // check for SPA errors
  {
    outdata.errCode = 2;
    return outdata;
  }

  outdata.errCode = 0;
  outdata.elevation = 90.0 - spa.zenith;
  outdata.azimuth = spa.azimuth;
  outdata.incidence = spa.incidence;

  min = 60.0 * (spa.sunrise - (int)(spa.sunrise));
  sec = 60.0 * (min - (int)min);
  outdata.sunrise = Time(spa.sunrise, min, sec, input->dateTime.tt.timezone);

  min = 60.0 * (spa.sunset - (int)(spa.sunset));
  sec = 60.0 * (min - (int)min);
  outdata.sunset = Time(spa.sunset, min, sec, input->dateTime.tt.timezone);

  return outdata;
}