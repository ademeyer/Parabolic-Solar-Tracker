#pragma once

#include "ISensor.h"
#include "ICompute.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include "spa.h"

  SunPositionData getSunPosition(const SPA_Input *input);
#ifdef __cplusplus
} // extern "C"
#endif
