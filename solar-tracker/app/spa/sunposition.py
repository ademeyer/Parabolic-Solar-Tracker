# The MIT License (MIT)
# 
# Copyright (c) 2025 Samuel Bear Powell
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import os
import sys
import argparse
import re
import numpy as np
import time
import datetime
import warnings

try:
    #scipy is required for numba's linear algebra routines to work
    import numba
    import scipy
except:
    numba = None

VERSION = '1.2.1'

_arg_parser = argparse.ArgumentParser(prog='sunposition',description='Compute sun position parameters given the time and location')
_arg_parser.add_argument('--version',action='version',version=f'%(prog)s {VERSION}')
_arg_parser.add_argument('--citation',action='store_true',help='Print citation information')
_arg_parser.add_argument('-t','--time',type=str,default='now',help='"now" or date and time in ISO8601 format or a (UTC) POSIX timestamp')
_arg_parser.add_argument('-lat','--latitude',type=float,default=51.48,help='observer latitude, in decimal degrees, positive for north')
_arg_parser.add_argument('-lon','--longitude',type=float,default=0.0,help='observer longitude, in decimal degrees, positive for east')
_arg_parser.add_argument('-e','--elevation',type=float,default=0,help='observer elevation, in meters')
_arg_parser.add_argument('-T','--temperature',type=float,default=14.6,help='temperature, in degrees celcius')
_arg_parser.add_argument('-p','--pressure',type=float,default=1013.0,help='atmospheric pressure, in millibar')
_arg_parser.add_argument('-a','--atmos_refract',type=float,default=None,help='Atmospheric refraction at sunrise and sunset, in degrees. Omit to compute automatically, spa.c uses 0.5667')
_arg_parser.add_argument('-dt',type=float,default=0.0,help='difference between earth\'s rotation time (TT) and universal time (UT1)')
_arg_parser.add_argument('-r','--radians',action='store_true',help='Output in radians instead of degrees')
_arg_parser.add_argument('--csv',action='store_true',help='Comma separated values (time,dt,lat,lon,elev,temp,pressure,az,zen,RA,dec,H)')
_arg_parser.add_argument('--jit',action='store_true',help='Enable Numba acceleration (likely to cause slowdown for a single computation!)')

def main(args=None, **kwargs):
    """Run sunposition command-line tool.

    If run without arguments, uses sys.argv, otherwise arguments may be
    specified by a list of strings to be parsed, e.g.:
        main(['--time','now'])
    or as keyword arguments:
        main(time='now')
    or as an argparse.Namespace object (as produced by argparse.ArgumentParser)

    Parameters
    ----------
    args : list of str or argparse.Namespace, optional
        Command-line arguments. sys.argv is used if not provided.
    version : bool
        If true, print the version information and quit
    citation : bool
        If true, print citation information and quit
    time : str
        "now" or date and time in ISO8601 format or a UTC POSIX timestamp
    latitude : float
        observer latitude in decimal degrees, positive for north
    longitude : float
        observer longitude in decimal degrees, positive for east
    elevation : float
        observer elevation in meters
    temperature : float
        temperature, in degrees celcius
    pressure : float
        atmospheric pressure, in millibar
    atmos_refract : float
        atmospheric refraction at sunrise and sunset, in degrees
    dt : float
        difference between Earth's rotation time (TT) and universal time (UT1)
    radians : bool
        If True, output in radians instead of degrees
    csv : bool
        If True, output as comma separated values (time, dt, lat, lon, elev, temp, pressure, az, zen, RA, dec, H)
    """
    if args is None and not kwargs:
        args = _arg_parser.parse_args()
    elif isinstance(args,(list,tuple)):
        args = _arg_parser.parse_args(args)
    
    for kw in kwargs:
        setattr(args,kw,kwargs[kw])
    '''    
    if args.citation:
        print("Algorithm:")
        print("  Ibrahim Reda, Afshin Andreas, \"Solar position algorithm for solar radiation applications\",")
        print("  Solar Energy, Volume 76, Issue 5, 2004, Pages 577-589, ISSN 0038-092X,")
        print("  doi:10.1016/j.solener.2003.12.003")
        print("Implemented by Samuel Powell, 2016-2025, https://github.com/s-bear/sun-position")
        return 0
    '''
    enable_jit(args.jit)
    
    t = time_to_datetime64(args.time)
    lat, lon, elev = args.latitude, args.longitude, args.elevation
    temp, p, ar, dt = args.temperature, args.pressure, args.atmos_refract, args.dt 
    rad = args.radians

    az, zen, ra, dec, h = sunposition(t, lat, lon, elev, temp, p, ar, dt, radians=rad)

    if args.csv:
        pass
        #machine readable
        # print(f'{t}, {dt}, {lat}, {lon}, {elev}, {temp}, {p}, {az:0.6f}, {zen:0.6f}, {ra:0.6f}, {dec:0.6f}, {h:0.6f}')
    else:
        dr = 'rad' if args.radians else 'deg'
        ts = time_to_iso8601(t)
        # print(f"Computing sun position at T = {ts} + {dt} s")
        # print(f"Lat, Lon, Elev = {lat} deg, {lon} deg, {elev} m")
        # print(f"T, P = {temp} C, {p} mbar")
        # print("Results:")
        # print(f"Azimuth, zenith = {az:0.6f} {dr}, {zen:0.6f} {dr}")
        # print(f"RA, dec, H = {ra:0.6f} {dr}, {dec:0.6f} {dr}, {h:0.6f} {dr}")\
        print(f"{az:0.0f},{zen:0.0f}")

    return 0

def enable_jit(en = True):
    global _ENABLE_JIT
    if en and numba is None:
        warnings.warn('JIT unavailable (requires numba and scipy)',stacklevel=2)
    #We set the _ENABLE_JIT flag regardless of whether numba is available, just to test that code path!
    _ENABLE_JIT = en

def disable_jit():
    enable_jit(False)

def jit_enabled():
    if _ENABLE_JIT:
        return _jit_test()
    return False

def arcdist(p0, p1, *, radians=False, jit=None):
    """Angular distance between azimuth,zenith pairs
    
    Parameters
    ----------
    p0 : array_like, shape (..., 2)
    p1 : array_like, shape (..., 2)
        p[...,0] = azimuth angles, p[...,1] = zenith angles
    radians : boolean (default False)
        If False, angles are in degrees, otherwise in radians
    jit : bool, optional
        override module jit settings. True to enable Numba acceleration (default if Numba is available), False to disable.

    Returns
    -------
    ad :  array_like, shape is broadcast(p0,p1).shape
        Arcdistances between corresponding pairs in p0,p1
        In degrees by default, in radians if radians=True
    """
    #formula comes from translating points into cartesian coordinates
    #taking the dot product to get the cosine between the two vectors
    #then arccos to return to angle, and simplify everything assuming real inputs
    p0,p1 = np.broadcast_arrays(p0, p1)
    if radians:
        if jit: return _arcdist_jit(p0,p1)
        else: return _arcdist(p0,p1)
    else:
        if jit: return _arcdist_deg_jit(p0,p1)
        else: return _arcdist_deg(p0,p1)

_np_microseconds = np.dtype('datetime64[us]')

def time_to_datetime64(t):
    '''Convert various date/time formats to microsecond `numpy.datetime64`.
    
    When parameter `t` is a numeric type, it is assumed to be a POSIX-style 
    timestamp, in seconds since the 1970-01-01 epoch.
    
    When `t` contains `datetime.datetime`s they're converted to UTC first using 
    `datetime.datetime.astimezone(datetime.timezone.utc)`, which assumes the
    datetime is in local time if it's not timezone aware.

    When `t` contains `str`s they're parsed per ISO 8601, with some variations
    accepted. Notable, "now" uses `time.time_ns()` to obtain the current time.
    The parser uses approximately the following grammar:
        <DATETIME> := <DATE> ('T'|' ') <TIME> [<TIMEZONE>]
        <DATE> := ['+'|'-'] <YEAR> ['-'|'/'] <MONTH> ['-'|'/'] <DAY>
        <TIME> := <HOUR> [':'] <MINUTE> [[':'] <SECOND>]
        <TIMEZONE> := 'Z' | ('+'|'-') <HOUR> [[':'] <MINUTE>]
    Note that <YEAR> is a 0-padded 4-digit number, <MONTH>, <DAY>, <HOUR>, and
    <MINUTE> are similarly 0-padded 2-digit numbers. <SECOND> is a 0-padded
    2-digit number with an optional fractional part: '01' and '01.234' are both
    valid <SECOND>s. <YEAR>, <MONTH>, and <DAY> may be fewer digits when the
    separators (- or /) them are included, which eliminates any ambiguity.

    Parameters
    ----------
    t : array_like of various date/time formats
    
    Returns
    -------
    t_microseconds : array of datetime64[us]
    '''
    t = np.asarray(t)
    # [()] unwrap scalar values out of np.array
    if np.issubdtype(t.dtype, np.datetime64):
        #NB: issubdtype can't differentiate between different units of datetime64
        #  ie. it says datetime64[s] is the same as datetime64[us]
        return t.astype(_np_microseconds)[()]
    elif np.issubdtype(t.dtype, str):
        return _time_string_to_i64_vec(t).astype(_np_microseconds)[()]
    elif np.issubdtype(t.dtype, datetime.datetime):
        return _time_datetime_to_datetime64(t).astype(_np_microseconds)[()]
    else:
        #assume posix timestamp (seconds)
        return np.round(t*1e6).astype(_np_microseconds)[()]

def time_to_iso8601(t):
    '''Convert various date/time formats to ISO8601 timestamp strings'''
    t = time_to_datetime64(t)
    s = _time_i64_to_string_vec(t.astype(np.int64))
    if s.shape == (): return str(s)
    return s

def julian_day(t, *, jit=None):
    """Convert timestamps from various formats to Julian days

    Parameters
    ----------
    t : array_like
        datetime.datetime, numpy.datetime64, ISO8601 strings, or POSIX timestamps (str, float, int)
    jit : bool or None
        override module jit settings, to True/False to enable/disable numba acceleration
    
    Returns
    -------
    jd : ndarray
        datetimes converted to fractional Julian days
    """

    t = time_to_datetime64(t).astype(np.int64)
    if jit is None: jit = _ENABLE_JIT
    if jit:
        _jit_check()
        jd = _julian_day_vec_jit(t)
    else:
        jd = _julian_day_vec(t)[()]
    return jd

def sunposition(t, latitude, longitude, elevation, temperature=None, pressure=None, atmos_refract=None, delta_t=0, *, radians=False, jit=None):
    """Compute the observed and topocentric coordinates of the sun as viewed at the given time and location.

    Parameters
    ----------
    t : array_like of datetime, datetime64, str, or float
        datetime.datetime, numpy.datetime64, ISO8601 strings, or POSIX timestamps (float or int)
    latitude, longitude : array_like of float
        decimal degrees, positive for north of the equator and east of Greenwich
    elevation : array_like of float
        meters, relative to the WGS-84 ellipsoid
    temperature : None or array_like of float, optional
        celcius, default is 14.6 (global average in 2013)
    pressure : None or array_like of float, optional
        millibar, default is 1013 (global average in ??)
    atmos_refract : None or array_like of float, optional
        Atmospheric refraction at sunrise and sunset, in degrees. None to compute automatically, spa.c default is 0.5667.
    delta_t : array_like of float, optional
        seconds, default is 0, difference between the earth's rotation time (TT) and universal time (UT)
    radians : bool, optional
        return results in radians if True, degrees if False (default)
    jit : bool, optional
        override module jit settings. True to enable Numba acceleration (default if Numba is available), False to disable.
    
    Returns
    -------
    azimuth_angle : ndarray, measured eastward from north
    zenith_angle : ndarray, measured down from vertical
    right_ascension : ndarray, topocentric
    declination : ndarray, topocentric
    hour_angle : ndarray, topocentric
    """

    if temperature is None:
        temperature = 14.6
    if pressure is None:
        pressure = 1013
    if jit is None:
        jit = _ENABLE_JIT

    t = time_to_datetime64(t).astype(np.int64)

    if jit:
        _jit_check()
        args = np.broadcast_arrays(t, latitude, longitude, elevation, temperature, pressure, atmos_refract, delta_t)
        for a in args: a.flags.writeable = False
        sp = _sunpos_vec_jit(*args)
    else:
        sp = _sunpos_vec(t, latitude, longitude, elevation, temperature, pressure, atmos_refract, delta_t)
    if radians:
        sp = tuple(np.deg2rad(a) for a in sp)
    sp = tuple(a[()] for a in sp) #unwrap np.array() from scalars
    return sp

def topocentric_sunposition(t, latitude, longitude, elevation, delta_t=0, *, radians=False, jit=None):
    """Compute the topocentric coordinates of the sun as viewed at the given time and location.

    Parameters
    ----------
    t : array_like of datetime, datetime64, str, or float
        datetime.datetime, numpy.datetime64, ISO8601 strings, or POSIX timestamps (float or int)
    latitude, longitude : array_like of float
        decimal degrees, positive for north of the equator and east of Greenwich
    elevation : array_like of float
        meters, relative to the WGS-84 ellipsoid
    delta_t : array_like of float, optional
        seconds, default is 0, difference between the earth's rotation time (TT) and universal time (UT)
    radians : bool, optional
        return results in radians if True, degrees if False (default)
    jit : bool, optional
        override module jit settings. True to enable Numba acceleration (default if Numba is available), False to disable.

    Returns
    -------
    right_ascension : ndarray, topocentric
    declination : ndarray, topocentric
    hour_angle : ndarray, topocentric
    """
    if jit is None:
        jit = _ENABLE_JIT
    t = time_to_datetime64(t).astype(np.int64)
    if jit:
        _jit_check()
        args = np.broadcast_arrays(t, latitude, longitude, elevation, delta_t)
        for a in args: a.flags.writeable = False
        sp = _topo_sunpos_vec_jit(*args)
    else:
        sp = _topo_sunpos_vec(t, latitude, longitude, elevation, delta_t)
    if radians:
        sp = tuple(np.deg2rad(a) for a in sp)
    sp = tuple(a[()] for a in sp) #unwrap np.array() from scalars
    return sp

def observed_sunposition(t, latitude, longitude, elevation, temperature=None, pressure=None, atmos_refract=None, delta_t=0, *, radians=False, jit=None):
    """Compute the observed coordinates of the sun as viewed at the given time and location.

    Parameters
    ----------
    t : array_like of datetime, datetime64, str, or float
        datetime.datetime, numpy.datetime64, ISO8601 strings, or POSIX timestamps (float or int)
    latitude, longitude : array_like of float
        decimal degrees, positive for north of the equator and east of Greenwich
    elevation : array_like of float
        meters, relative to the WGS-84 ellipsoid
    temperature : None or array_like of float, optional
        celcius, default is 14.6 (global average in 2013)
    pressure : None or array_like of float, optional
        millibar, default is 1013 (global average in ??)
    atmos_refract : None or array_like of float, optional
        Atmospheric refraction at sunrise and sunset, in degrees. None to compute automatically, spa.c default is 0.5667.
    delta_t : array_like of float, optional
        seconds, default is 0, difference between the earth's rotation time (TT) and universal time (UT)
    radians : bool, optional
        return results in radians if True, degrees if False (default)
    jit : bool, optional
        override module jit settings. True to enable Numba acceleration (default if Numba is available), False to disable.

    Returns
    -------
    azimuth_angle : ndarray, measured eastward from north
    zenith_angle : ndarray, measured down from vertical
    """
    return sunpos(t, latitude, longitude, elevation, temperature, pressure, atmos_refract, delta_t, radians=radians, jit=jit)[:2]

sunpos = sunposition
topocentrict_sunpos = topocentric_sunposition
observed_sunpos = observed_sunposition

## Numba decorators ##

def _empty_decorator(f = None, *args, **kw):
    if callable(f):
        return f
    return _empty_decorator

if numba is not None:
    # register_jitable informs numba that a function may be compiled when
    # called from jit'ed code, but doesn't jit it by default
    register_jitable = numba.extending.register_jitable

    # overload informs numba of an *alternate implementation* of a function to
    # use within jit'ed code -- we use it to provide a jit-able version of polyval
    overload = numba.extending.overload
    
    #njit compiles code -- we use this for our top-level functions
    njit = numba.njit

    _ENABLE_JIT = not numba.config.DISABLE_JIT and not os.environ.get('NUMBA_DISABLE_JIT',False)
else:
    #if numba is not available, use _empty_decorator instead
    njit = _empty_decorator
    overload = lambda *a,**k: _empty_decorator
    register_jitable = _empty_decorator
    _ENABLE_JIT = False

## machinery for jit_enabled()

def _jit_test_impl():
    #return False if within Python
    return False

@overload(_jit_test_impl)
def _jit_test_impl_jit():
    def _jit_test_impl_jit():
        #return True if within JIT'ed code
        return True
    return _jit_test_impl_jit

@njit
def _jit_test():
    return _jit_test_impl()

def _jit_check():
    if not _jit_test():
        warnings.warn('JIT requested, but numba is not available!')

## arcdist ##

def _arcdist(p0,p1):
    a0,z0 = p0[...,0], p0[...,1]
    a1,z1 = p1[...,0], p1[...,1]
    return np.arccos(np.cos(z0)*np.cos(z1)+np.cos(a0-a1)*np.sin(z0)*np.sin(z1))

def _arcdist_deg(p0,p1):
    return np.rad2deg(_arcdist(np.deg2rad(p0),np.deg2rad(p1)))

_arcdist_jit = njit(_arcdist)
_arcdist_deg_jit = njit(_arcdist_deg)

## Dates and times ##

# this application has unusual date/time requirements that are not supported by
# Python's time or datetime libraries. Specifically:
# 1. The subroutines use Julian days to represent time
# 2. The algorithm supports dates from year -2000 to 6000 (datetime supports years 1-9999)

# For that reason, we will use our own date & time codes
# The Julian Day conversion in Reda & Andreas's paper required the Gregorian
#  (year, month, day), with the time of day specified as a fractional day, all in UTC.
# We can use the algorithms in "Euclidean affine functions and their application to calendar algorithms" C. Neri, L. Schneider (2022) https://doi.org/10.1002/spe.3172
#  to convert rata die timestamps (day number since epoch) to Gregorian (year, month, day)
# We need to be able to convert the following to Julian days:
#  datetime.datetime
#  numpy.datetime64
#  int/float POSIX timestamp
#  ISO8601 string

@register_jitable
def _time_day_to_date(day_number):
    '''convert integer day number to Gregorian (year, month, day)
    year,month,day are int32, uint8, uint8
    
    "Euclidean affine functions and their application to calendar algorithms" C. Neri, L. Schneider (2022) https://doi.org/10.1002/spe.3172
    '''
    #  date32_t to_date(int32_t N_U) {
    N_U = np.int32(day_number)

    s = np.uint32(82) #static uint32_t constexpr s = 82;
    K = np.uint32(719468 + 146097 * s) #static uint32_t constexpr K = 719468 + 146097 * s;
    L = np.uint32(400*s) #static uint32_t constexpr L = 400 * s;
    
    # Rata die shift.
    N = np.uint32(N_U + K) # uint32_t const N = N_U + K;

    # Century.
    N_1 = np.uint32(4 * N + 3) # uint32_t const N_1 = 4 * N + 3;
    # uint32_t const C   = N_1 / 146097;
    # uint32_t const N_C = N_1 % 146097 / 4;
    C, N_C = divmod(N_1, 146097)
    C, N_C = np.uint32(C), np.uint32(N_C // 4)

    # Year.
    N_2 = np.uint32(4 * N_C + 3) # uint32_t const N_2 = 4 * N_C + 3;
    P_2 = 2939745 * np.uint64(N_2) # uint64_t const P_2 = uint64_t(2939745) * N_2;
    # uint32_t const Z   = uint32_t(P_2 / 4294967296);
    # uint32_t const N_Y = uint32_t(P_2 % 4294967296) / 2939745 / 4;
    Z, N_Y = divmod(P_2, 4294967296)
    Z, N_Y = np.uint32(Z), np.uint32(N_Y)
    N_Y = np.uint32((N_Y // 2939745) // 4)
    Y   = np.uint32(100 * C + Z) # uint32_t const Y   = 100 * C + Z;

    # Month and day. 
    N_3 = np.uint32(2141 * N_Y + 197913) # uint32_t const N_3 = 2141 * N_Y + 197913;
    # uint32_t const M   = N_3 / 65536;
    # uint32_t const D   = N_3 % 65536 / 2141;
    M, D = divmod(N_3, 65536)
    M, D = np.uint32(M), np.uint32(D // 2141)

    # Map. (Notice the year correction, including type change.)
    J   = (N_Y >= 306) # uint32_t const J   = N_Y >= 306;
    Y_G = np.int32(Y) - np.int32(L) + np.int32(J) # int32_t  const Y_G = (Y - L) + J;
    # uint32_t const M_G = J ? M - 12 : M;
    if J:
        M_G = np.uint8(M - 12) #we use uint8 instead of uint32
    else:
        M_G = np.uint8(M)

    D_G = np.uint8(D + 1) # uint32_t const D_G = D + 1;
    # return { Y_G, M_G, D_G };
    return (Y_G, M_G, D_G)

@register_jitable
def _time_i64_to_datetime(t_microseconds):
    '''Convert int64 timestamp to (year,month,day,hour,minute,second,micros)
    t_microseconds : microseconds since 1970-1-1
    '''
    # divide by number of microseconds in a day
    day_number, micros = divmod(t_microseconds,86400000000)
    # use Neri-Schneider calendar algorithm to get Gregorian date
    year,month,day = _time_day_to_date(day_number)
    # divide micros to get hour,minute,second
    hour, micros = divmod(micros, 3600000000)
    minute, micros = divmod(micros, 60000000)
    second, micros = divmod(micros, 1000000)
    hour, minute, second = np.uint8(hour), np.uint8(minute), np.uint8(second)
    micros = np.uint32(micros)
    return year,month,day,hour,minute,second,micros

@register_jitable
def _time_date_to_day(date_tuple):
    '''convert Gregorian (year, month, day) to day number
    date_tuple : (int32, uint8, uint8) : year,month,day
    returns day_number : int32
    
    "Euclidean affine functions and their application to calendar algorithms" C. Neri, L. Schneider (2022) https://doi.org/10.1002/spe.3172
    '''
    year,month,day = date_tuple[:3]
    Y_G = np.int32(year)
    M_G = np.uint32(month)
    D_G = np.uint32(day)

    s = np.uint32(82) #static uint32_t constexpr s = 82;
    K = np.uint32(719468 + 146097 * s) #static uint32_t constexpr K = 719468 + 146097 * s;
    L = np.uint32(400*s) #static uint32_t constexpr L = 400 * s;
    #  int32_t to_rata_die(int32_t Y_G, uint32_t M_G, uint32_t D_G) {
    # Map. (Notice the year correction, including type change.)
    J = (M_G <= 2) # uint32_t const J = M_G <= 2;
    Y = np.uint32(Y_G + L - J) # uint32_t const Y = (uint32_t(Y_G) + L) - J;
    # uint32_t const M = J ? M_G + 12 : M_G;
    if J: 
        M = np.uint32(M_G + 12)
    else:
        M = M_G
    
    D = np.uint32(D_G - 1) # uint32_t const D = D_G - 1;
    C = np.uint32(Y // 100) # uint32_t const C = Y / 100;

    # Rata die.
    y_star = np.uint32(1461 * Y // 4 - C + C // 4) # uint32_t const y_star = 1461 * Y / 4 - C + C / 4;
    m_star = np.uint32((979 * M - 2919) // 32) # uint32_t const m_star = (979 * M - 2919) / 32;
    N      = np.uint32(y_star + m_star + D) # uint32_t const N      = y_star + m_star + D;
    
    # Rata die shift.
    N_U = np.int32(N) - np.int32(K) # uint32_t const N_U = N - K; -- the original casts to int32 at return, we do it here
    return N_U

@register_jitable
def _time_datetime_to_i64(datetime_tuple):
    '''Convert datetime_tuple to int64 timestamp
    datetime_tuple : (int32, uint8, uint8, uint8, uint8, uint8, uint32)
        year,month,day,hour,minute,second,microsecond
    returns t : int64, microseconds since epoch 1970-01-01
    
    Does not check for valid dates
    '''
    year,month,day,hour,minute,second,microsecond = datetime_tuple[:7]
    # accumulate parts
    t = np.int64(microsecond)
    t += np.int64(second)*1000000
    t += np.int64(minute)*60000000
    t += np.int64(hour)*3600000000
    t += np.int64(_time_date_to_day((year,month,day)))*86400000000
    return t

@register_jitable
def _time_datetime_to_i64_checked(datetime_tuple):
    '''Convert year,month,day,hour,minute second,microsecond to int64 timestamp, with validity check
    year,month,day : int32, uint8, uint8
    hour,minute,second,microsecond: uint8, uint8, uint8, uint32

    Throws a ValueError if the date & time are not valid in the Gregorian calendar

    return t : int64, microseconds since the epoch 1970-01-01
    '''
    t = _time_datetime_to_i64(datetime_tuple)
    datetime_tuple_valid = _time_i64_to_datetime(t)
    if datetime_tuple != datetime_tuple_valid:
        raise ValueError('Invalid date')
    return t

#                          (  (DATE, NO SEPARATORS            )|(DATE, WITH SEPARATORS                      ))(TIME                                                (TIMEZONE                         ) )
#                                (1:YEAR    )(2:MONTH)(3:DAY )     (4: YEAR     )(5)   (6:MONTH)   (7:  DAY)         (8: HOUR)  (9: MIN )     (10: SECOND       )          (11:TZHOUR)     (12:TZMIN)
_iso8601_re = re.compile(r'(?:(?:([+-]?\d{4})([01]\d)([0-3]\d))|(?:([+-]?\d{1,4})([-/])([01]?\d)\5([0-3]?\d)))(?:[T ]([012]\d):?([0-5]\d)(?::?([0-6]\d(?:\.\d*)?))?(?:Z|(?:([+-]\d{2})(?::?(\d{2}))?))?)?')
def _time_string_to_i64(s):
    '''parse timestamp string to integer microsecond timestamp, assumes UTC if timezone is not specified
    strings may be:
     - "now" -- which gets the current time
     - POSIX timestamp string (seconds since epoch, including fractional part)
     - ISO 8601 formatted string (including negative years, fractional seconds)
    '''
    if s == 'now':
        return time.time_ns()//1000
    try:
        t = float(s)
        #if np.abs(t - np.nextafter(t,0)) > 1e-6: warnings.warn('Timestamp resolution greater than 1 microsecond.')
        return round(float(s)*1e6)
    except ValueError: #
        pass
    match = _iso8601_re.fullmatch(s)
    if not match:
        raise ValueError('Could not parse timestamp string (must be "now" or float or ISO8601)')
    year,month,day,year2,sep,month2,day2,hour,minute,second,tz_hour,tz_minute = match.groups()
    if year is None: year,month,day = year2,month2,day2
    if second is None: second = 0
    if tz_hour is None: tz_hour = 0
    if tz_minute is None: tz_minute = 0
    if year is None or month is None or day is None or hour is None or minute is None:
        raise ValueError('Could not parse timestamp string (must be "now" or float or ISO8601)')
    year, month, day = int(year),int(month),int(day)
    hour,minute,micros = int(hour), int(minute), round(float(second)*1e6)
    second,micros = divmod(micros,1000000)
    tz_hour, tz_minute = int(tz_hour), int(tz_minute)
    if tz_minute < 0 or tz_minute >= 60:
        raise ValueError(f'Invalid timezone: "{tz_hour:02}:{tz_minute:02}"')
    if tz_hour < 0: tz_minute = -tz_minute
    tz_micros = (tz_hour*3600 + tz_minute*60)*1000000 # timezone offset, in microseconds
    # UTC offsets vary from -12:00 (US Minor Outlying Islands) to +14:00 (Kiribati)
    tz_min, tz_max = -12*3600*1000000, 14*3600*1000000
    if tz_micros < tz_min or tz_micros > tz_max:
        raise ValueError(f'Invalid timezone: "{tz_hour:02}:{tz_minute:02}"')
    #convert to timestamp
    t = _time_datetime_to_i64_checked((year,month,day,hour,minute,second,micros))
    #apply timezone offset and return
    return t - tz_micros

_time_string_to_i64_vec = np.vectorize(_time_string_to_i64)

def _time_i64_to_string(t):
    '''Format a microsecond timestamp as an ISO8601 string'''
    year, month, day, hour, minute, sec, micros = _time_i64_to_datetime(t)
    millis, micros = divmod(micros,1000)
    if millis:
        if micros: fsec = f'.{millis:03}{micros:03}'
        else: fsec = f'.{millis:03}'
    else:
        if micros: fsec = f'.000{micros:03}'
        else: fsec = ''
    
    return f'{year:04}-{month:02}-{day:02}T{hour:02}:{minute:02}:{sec:02}{fsec}Z'

_time_i64_to_string_vec = np.vectorize(_time_i64_to_string)

#we can't use numba to accelerate datetime.datetime ops, so use np.vectorize here
@np.vectorize
def _time_datetime_to_datetime64(t : datetime.datetime):
    '''get the i64 timestamp from a datetime.datetime object.
    Converts the datetime to UTC first, datetime objects without tzinfo are treated as local time.
    '''
    return np.datetime64(t.astimezone(datetime.timezone.utc).replace(tzinfo=None))

## Procedure from Reda, Andreas (2004) ##

## 3.1. Calculate the Julian and Julian Ephemeris Day (JDE), centure, and millenium

@register_jitable
def _julian_day(t_microseconds):
    """Calculate the Julian Day from posix timestamp (seconds since epoch)"""
    #TODO: check julian-gregorian calendar changeover for posix timestamps
    # divide by number of microseconds in a day
    day_number, micros = divmod(t_microseconds,86400000000)
    # use Neri-Schneider calendar algorithm to get Gregorian date
    year,month,day = _time_day_to_date(day_number)
    day += np.float64(micros)/86400000000 #add back in fractional day
    
    # eq 4
    # 3.1.1) if M = 1 or 2, then Y = Y - 1 and M = M + 12
    if month <= 2:  
        month += 12
        year -= 1
    jd = int(365.25 * (year + 4716)) + int(30.6001 * (month + 1)) + day - 1524.5
    if jd > 2299160:
        # 3.1.1) B is equal to 0 for the Julian calendar and is equal to 
        #    (2-A+INT(A/4)), A = INT(Y/100), for the Gregorian calendar.
        a = int(year / 100)
        b = 2 - a + int(a / 4)
        jd += b
    return jd

_julian_day_vec = np.vectorize(_julian_day)

@njit
def _julian_day_vec_jit(t):
    ts = np.asarray(t)
    ts_flat = ts.flat
    n = len(ts_flat)
    jds = np.empty(n, dtype=np.float64)
    for i, t in enumerate(ts_flat):
        jds[i] = _julian_day(t)
    jds = jds.reshape(ts.shape)
    return jds[()]

@register_jitable
def _julian_ephemeris_day(jd, Delta_t):
    """Calculate the Julian Ephemeris Day from the Julian Day and delta-time = (terrestrial time - universal time) in seconds"""
    # eq 5
    return jd + Delta_t / 86400.0

@register_jitable
def _julian_century(jd):
    """Caluclate the Julian Century from Julian Day or Julian Ephemeris Day"""
    # eq 6
    # eq 7
    return (jd - 2451545.0) / 36525.0

@register_jitable
def _julian_millennium(jc):
    """Calculate the Julian Millennium from Julian Ephemeris Century"""
    # eq 8
    return jc / 10.0

## 3.2. Calculate the Earth heliocentric longitude, latitude, and radius vector (L, B, and R)

@register_jitable
def _cos_sum(x, coeffs):
    y = np.zeros(len(coeffs))
    for i, abc in enumerate(coeffs):
        for a,b,c in abc:
            # eq 9
            y[i] += a*np.cos(b + c*x)
    return y

#implement np.polyval for numba using overload
@overload(np.polyval)
def _polyval_jit(p, x):
    def _polyval_impl(p, x):
        y = 0.0
        for v in p:
            y = y*x + v
        return y
    return _polyval_impl

# Table 1. Earth periodic terms, Earth Heliocentric Longitude coefficients (L0, L1, L2, L3, L4, and L5)
_EHL = (
    #L5:
    np.array([(1.0, 3.14, 0.0)]),
    #L4:
    np.array([(114.0, 3.142, 0.0), (8.0, 4.13, 6283.08), (1.0, 3.84, 12566.15)]),
    #L3:
    np.array([(289.0, 5.844, 6283.076), (35.0, 0.0, 0.0,), (17.0, 5.49, 12566.15),
    (3.0, 5.2, 155.42), (1.0, 4.72, 3.52), (1.0, 5.3, 18849.23),
    (1.0, 5.97, 242.73)]),
    #L2:
    np.array([(52919.0, 0.0, 0.0), (8720.0, 1.0721, 6283.0758), (309.0, 0.867, 12566.152),
    (27.0, 0.05, 3.52), (16.0, 5.19, 26.3), (16.0, 3.68, 155.42),
    (10.0, 0.76, 18849.23), (9.0, 2.06, 77713.77), (7.0, 0.83, 775.52),
    (5.0, 4.66, 1577.34), (4.0, 1.03, 7.11), (4.0, 3.44, 5573.14),
    (3.0, 5.14, 796.3), (3.0, 6.05, 5507.55), (3.0, 1.19, 242.73),
    (3.0, 6.12, 529.69), (3.0, 0.31, 398.15), (3.0, 2.28, 553.57),
    (2.0, 4.38, 5223.69), (2.0, 3.75, 0.98)]),
    #L1:
    np.array([(628331966747.0, 0.0, 0.0), (206059.0, 2.678235, 6283.07585), (4303.0, 2.6351, 12566.1517),
    (425.0, 1.59, 3.523), (119.0, 5.796, 26.298), (109.0, 2.966, 1577.344),
    (93.0, 2.59, 18849.23), (72.0, 1.14, 529.69), (68.0, 1.87, 398.15),
    (67.0, 4.41, 5507.55), (59.0, 2.89, 5223.69), (56.0, 2.17, 155.42),
    (45.0, 0.4, 796.3), (36.0, 0.47, 775.52), (29.0, 2.65, 7.11),
    (21.0, 5.34, 0.98), (19.0, 1.85, 5486.78), (19.0, 4.97, 213.3),
    (17.0, 2.99, 6275.96), (16.0, 0.03, 2544.31), (16.0, 1.43, 2146.17),
    (15.0, 1.21, 10977.08), (12.0, 2.83, 1748.02), (12.0, 3.26, 5088.63),
    (12.0, 5.27, 1194.45), (12.0, 2.08, 4694), (11.0, 0.77, 553.57),
    (10.0, 1.3, 6286.6), (10.0, 4.24, 1349.87), (9.0, 2.7, 242.73),
    (9.0, 5.64, 951.72), (8.0, 5.3, 2352.87), (6.0, 2.65, 9437.76),
    (6.0, 4.67, 4690.48)]),
    #L0:
    np.array([(175347046.0, 0.0, 0.0), (3341656.0, 4.6692568, 6283.07585), (34894.0, 4.6261, 12566.1517),
    (3497.0, 2.7441, 5753.3849), (3418.0, 2.8289, 3.5231), (3136.0, 3.6277, 77713.7715),
    (2676.0, 4.4181, 7860.4194), (2343.0, 6.1352, 3930.2097), (1324.0, 0.7425, 11506.7698),
    (1273.0, 2.0371, 529.691), (1199.0, 1.1096, 1577.3435), (990.0, 5.233, 5884.927),
    (902.0, 2.045, 26.298), (857.0, 3.508, 398.149), (780.0, 1.179, 5223.694),
    (753.0, 2.533, 5507.553), (505.0, 4.583, 18849.228), (492.0, 4.205, 775.523),
    (357.0, 2.92, 0.067), (317.0, 5.849, 11790.629), (284.0, 1.899, 796.298),
    (271.0, 0.315, 10977.079), (243.0, 0.345, 5486.778), (206.0, 4.806, 2544.314),
    (205.0, 1.869, 5573.143), (202.0, 2.458, 6069.777), (156.0, 0.833, 213.299),
    (132.0, 3.411, 2942.463), (126.0, 1.083, 20.775), (115.0, 0.645, 0.98),
    (103.0, 0.636, 4694.003), (102.0, 0.976, 15720.839), (102.0, 4.267, 7.114),
    (99.0, 6.21, 2146.17), (98.0, 0.68, 155.42), (86.0, 5.98, 161000.69),
    (85.0, 1.3, 6275.96), (85.0, 3.67, 71430.7), (80.0, 1.81, 17260.15),
    (79.0, 3.04, 12036.46), (75.0, 1.76, 5088.63), (74.0, 3.5, 3154.69),
    (74.0, 4.68, 801.82), (70.0, 0.83, 9437.76), (62.0, 3.98, 8827.39),
    (61.0, 1.82, 7084.9), (57.0, 2.78, 6286.6), (56.0, 4.39, 14143.5),
    (56.0, 3.47, 6279.55), (52.0, 0.19, 12139.55), (52.0, 1.33, 1748.02),
    (51.0, 0.28, 5856.48), (49.0, 0.49, 1194.45), (41.0, 5.37, 8429.24),
    (41.0, 2.4, 19651.05), (39.0, 6.17, 10447.39), (37.0, 6.04, 10213.29),
    (37.0, 2.57, 1059.38), (36.0, 1.71, 2352.87), (36.0, 1.78, 6812.77),
    (33.0, 0.59, 17789.85), (30.0, 0.44, 83996.85), (30.0, 2.74, 1349.87),
    (25.0, 3.16, 4690.48)])
)

@register_jitable
def _heliocentric_longitude(jme):
    """Compute the Earth Heliocentric Longitude (L) in degrees given the Julian Ephemeris Millennium"""
    # eq 10
    Li = _cos_sum(jme, _EHL) #L5, ..., L0
    # eq 11
    L = np.polyval(Li, jme) / 1e8
    # eq 12
    L = np.rad2deg(L) % 360
    return L

# Table 1. Earth periodic terms, Earth Heliocentric Latitude coefficients (B0 and B1)
_EHB = ( 
    #B1:
    np.array([(9.0, 3.9, 5507.55), (6.0, 1.73, 5223.69)]),
    #B0:
    np.array([(280.0, 3.199, 84334.662), (102.0, 5.422, 5507.553), (80.0, 3.88, 5223.69),
    (44.0, 3.7, 2352.87), (32.0, 4.0, 1577.34)])
)

@register_jitable
def _heliocentric_latitude(jme):
    """Compute the Earth Heliocentric Latitude (B) in degrees given the Julian Ephemeris Millennium"""
    # section 3.2.7
    Bi = _cos_sum(jme, _EHB)
    B = np.polyval(Bi, jme) / 1e8
    B = np.rad2deg(B) % 360
    return B

#Table 1. Earth periodic terms, Earth Heliocentric Radius coefficients (R0, R1, R2, R3, R4)
_EHR = (
    #R4:
    np.array([(4.0, 2.56, 6283.08)]),
    #R3:
    np.array([(145.0, 4.273, 6283.076), (7.0, 3.92, 12566.15)]),
    #R2:
    np.array([(4359.0, 5.7846, 6283.0758), (124.0, 5.579, 12566.152), (12.0, 3.14, 0.0),
    (9.0, 3.63, 77713.77), (6.0, 1.87, 5573.14), (3.0, 5.47, 18849.23)]),
    #R1:
    np.array([(103019.0, 1.10749, 6283.07585), (1721.0, 1.0644, 12566.1517), (702.0, 3.142, 0.0),
    (32.0, 1.02, 18849.23), (31.0, 2.84, 5507.55), (25.0, 1.32, 5223.69),
    (18.0, 1.42, 1577.34), (10.0, 5.91, 10977.08), (9.0, 1.42, 6275.96),
    (9.0, 0.27, 5486.78)]),
    #R0:
    np.array([(100013989.0, 0.0, 0.0), (1670700.0, 3.0984635, 6283.07585), (13956.0, 3.05525, 12566.1517),
    (3084.0, 5.1985, 77713.7715), (1628.0, 1.1739, 5753.3849), (1576.0, 2.8469, 7860.4194),
    (925.0, 5.453, 11506.77), (542.0, 4.564, 3930.21), (472.0, 3.661, 5884.927),
    (346.0, 0.964, 5507.553), (329.0, 5.9, 5223.694), (307.0, 0.299, 5573.143),
    (243.0, 4.273, 11790.629), (212.0, 5.847, 1577.344), (186.0, 5.022, 10977.079),
    (175.0, 3.012, 18849.228), (110.0, 5.055, 5486.778), (98.0, 0.89, 6069.78),
    (86.0, 5.69, 15720.84), (86.0, 1.27, 161000.69), (65.0, 0.27, 17260.15),
    (63.0, 0.92, 529.69), (57.0, 2.01, 83996.85), (56.0, 5.24, 71430.7),
    (49.0, 3.25, 2544.31), (47.0, 2.58, 775.52), (45.0, 5.54, 9437.76),
    (43.0, 6.01, 6275.96), (39.0, 5.36, 4694), (38.0, 2.39, 8827.39),
    (37.0, 0.83, 19651.05), (37.0, 4.9, 12139.55), (36.0, 1.67, 12036.46),
    (35.0, 1.84, 2942.46), (33.0, 0.24, 7084.9), (32.0, 0.18, 5088.63),
    (32.0, 1.78, 398.15), (28.0, 1.21, 6286.6), (28.0, 1.9, 6279.55),
    (26.0, 4.59, 10447.39)])
)

@register_jitable
def _heliocentric_radius(jme):
    """Compute the Earth Heliocentric Radius (R) in astronimical units given the Julian Ephemeris Millennium"""
    # section 3.2.8
    Ri = _cos_sum(jme, _EHR)
    R = np.polyval(Ri, jme) / 1e8
    return R

@register_jitable
def _heliocentric_position(jme):
    """Compute the Earth Heliocentric Longitude, Latitude, and Radius given the Julian Ephemeris Millennium
        Returns (L, B, R) where L = longitude in degrees, B = latitude in degrees, and R = radius in astronimical units
    """
    return _heliocentric_longitude(jme), _heliocentric_latitude(jme), _heliocentric_radius(jme)

## 3.3. Calculate the geocentric longitude and latitude (Θ [Theta] and β [beta])

@register_jitable
def _geocentric_position(heliocentric_position):
    """Compute the geocentric latitude (Θ [Theta]) and longitude (β [beta]) (in degrees) of the sun given the earth's heliocentric position (L, B, R)"""
    L,B,R = heliocentric_position
    # eq 13
    Theta = L + 180
    # eq 14
    beta = -B
    return (Theta, beta)

## 3.4. Calculate the nutation in longitude and obliquity (Δψ [Delta_psi] and Δε [Delta_epsilon])

#Table 2. Periodic terms for the nutation in longitude and obliquity, Coefficients for sin terms (Yi)
_NLO_Y = np.array([(0.0,   0.0,   0.0,   0.0,   1.0), (-2.0,  0.0,   0.0,   2.0,   2.0), (0.0,   0.0,   0.0,   2.0,   2.0),
        (0.0,   0.0,   0.0,   0.0,   2.0), (0.0,   1.0,   0.0,   0.0,   0.0), (0.0,   0.0,   1.0,   0.0,   0.0),
        (-2.0,  1.0,   0.0,   2.0,   2.0), (0.0,   0.0,   0.0,   2.0,   1.0), (0.0,   0.0,   1.0,   2.0,   2.0),
        (-2.0,  -1.0,  0.0,   2.0,   2.0), (-2.0,  0.0,   1.0,   0.0,   0.0), (-2.0,  0.0,   0.0,   2.0,   1.0),
        (0.0,   0.0,   -1.0,  2.0,   2.0), (2.0,   0.0,   0.0,   0.0,   0.0), (0.0,   0.0,   1.0,   0.0,   1.0),
        (2.0,   0.0,   -1.0,  2.0,   2.0), (0.0,   0.0,   -1.0,  0.0,   1.0), (0.0,   0.0,   1.0,   2.0,   1.0),
        (-2.0,  0.0,   2.0,   0.0,   0.0), (0.0,   0.0,   -2.0,  2.0,   1.0), (2.0,   0.0,   0.0,   2.0,   2.0),
        (0.0,   0.0,   2.0,   2.0,   2.0), (0.0,   0.0,   2.0,   0.0,   0.0), (-2.0,  0.0,   1.0,   2.0,   2.0),
        (0.0,   0.0,   0.0,   2.0,   0.0), (-2.0,  0.0,   0.0,   2.0,   0.0), (0.0,   0.0,   -1.0,  2.0,   1.0),
        (0.0,   2.0,   0.0,   0.0,   0.0), (2.0,   0.0,   -1.0,  0.0,   1.0), (-2.0,  2.0,   0.0,   2.0,   2.0),
        (0.0,   1.0,   0.0,   0.0,   1.0), (-2.0,  0.0,   1.0,   0.0,   1.0), (0.0,   -1.0,  0.0,   0.0,   1.0),
        (0.0,   0.0,   2.0,   -2.0,  0.0), (2.0,   0.0,   -1.0,  2.0,   1.0), (2.0,   0.0,   1.0,   2.0,   2.0),
        (0.0,   1.0,   0.0,   2.0,   2.0), (-2.0,  1.0,   1.0,   0.0,   0.0), (0.0,   -1.0,  0.0,   2.0,   2.0),
        (2.0,   0.0,   0.0,   2.0,   1.0), (2.0,   0.0,   1.0,   0.0,   0.0), (-2.0,  0.0,   2.0,   2.0,   2.0),
        (-2.0,  0.0,   1.0,   2.0,   1.0), (2.0,   0.0,   -2.0,  0.0,   1.0), (2.0,   0.0,   0.0,   0.0,   1.0),
        (0.0,   -1.0,  1.0,   0.0,   0.0), (-2.0,  -1.0,  0.0,   2.0,   1.0), (-2.0,  0.0,   0.0,   0.0,   1.0),
        (0.0,   0.0,   2.0,   2.0,   1.0), (-2.0,  0.0,   2.0,   0.0,   1.0), (-2.0,  1.0,   0.0,   2.0,   1.0),
        (0.0,   0.0,   1.0,   -2.0,  0.0), (-1.0,  0.0,   1.0,   0.0,   0.0), (-2.0,  1.0,   0.0,   0.0,   0.0),
        (1.0,   0.0,   0.0,   0.0,   0.0), (0.0,   0.0,   1.0,   2.0,   0.0), (0.0,   0.0,   -2.0,  2.0,   2.0),
        (-1.0,  -1.0,  1.0,   0.0,   0.0), (0.0,   1.0,   1.0,   0.0,   0.0), (0.0,   -1.0,  1.0,   2.0,   2.0),
        (2.0,   -1.0,  -1.0,  2.0,   2.0), (0.0,   0.0,   3.0,   2.0,   2.0), (2.0,   -1.0,  0.0,   2.0,   2.0)])

#Table 2. Periodic terms for the nutation in longitude and obliquity, Coefficients for Δψ [Delta_psi] (a,b) 
_NLO_AB = np.array([(-171996.0, -174.2), (-13187.0, -1.6), (-2274.0, -0.2), (2062.0, 0.2), (1426.0, -3.4), (712.0, 0.1),
        (-517.0, 1.2), (-386.0, -0.4), (-301.0, 0.0), (217.0, -0.5), (-158.0, 0.0), (129.0, 0.1),
        (123.0, 0.0), (63.0,  0.0), (63.0,  0.1), (-59.0, 0.0), (-58.0, -0.1), (-51.0, 0.0),
        (48.0,  0.0), (46.0,  0.0), (-38.0, 0.0), (-31.0, 0.0), (29.0,  0.0), (29.0,  0.0),
        (26.0,  0.0), (-22.0, 0.0), (21.0,  0.0), (17.0,  -0.1), (16.0,  0.0), (-16.0, 0.1),
        (-15.0, 0.0), (-13.0, 0.0), (-12.0, 0.0), (11.0,  0.0), (-10.0, 0.0), (-8.0,  0.0),
        (7.0,   0.0), (-7.0,  0.0), (-7.0,  0.0), (-7.0,  0.0), (6.0,   0.0), (6.0,   0.0),
        (6.0,   0.0), (-6.0,  0.0), (-6.0,  0.0), (5.0,   0.0), (-5.0,  0.0), (-5.0,  0.0),
        (-5.0,  0.0), (4.0,   0.0), (4.0,   0.0), (4.0,   0.0), (-4.0,  0.0), (-4.0,  0.0),
        (-4.0,  0.0), (3.0,   0.0), (-3.0,  0.0), (-3.0,  0.0), (-3.0,  0.0), (-3.0,  0.0),
        (-3.0,  0.0), (-3.0,  0.0), (-3.0,  0.0)])

#Table 2. Periodic terms for the nutation in longitude and obliquity, Coefficients for Δε [Delta_epsilon] (c,d) 
_NLO_CD = np.array([(92025.0,   8.9), (5736.0,    -3.1), (977.0, -0.5), (-895.0,    0.5),
        (54.0,  -0.1), (-7.0,  0.0), (224.0, -0.6), (200.0, 0.0),
        (129.0, -0.1), (-95.0, 0.3), (0.0,   0.0), (-70.0, 0.0),
        (-53.0, 0.0), (0.0,   0.0), (-33.0, 0.0), (26.0,  0.0),
        (32.0,  0.0), (27.0,  0.0), (0.0,   0.0), (-24.0, 0.0),
        (16.0,  0.0), (13.0,  0.0), (0.0,   0.0), (-12.0, 0.0),
        (0.0,   0.0), (0.0,   0.0), (-10.0, 0.0), (0.0,   0.0),
        (-8.0,  0.0), (7.0,   0.0), (9.0,   0.0), (7.0,   0.0),
        (6.0,   0.0), (0.0,   0.0), (5.0,   0.0), (3.0,   0.0),
        (-3.0,  0.0), (0.0,   0.0), (3.0,   0.0), (3.0,   0.0),
        (0.0,   0.0), (-3.0,  0.0), (-3.0,  0.0), (3.0,   0.0),
        (3.0,   0.0), (0.0,   0.0), (3.0,   0.0), (3.0,   0.0),
        (3.0,   0.0), (0.0, 0.0), (0.0, 0.0), (0.0, 0.0),
        (0.0, 0.0), (0.0, 0.0), (0.0, 0.0), (0.0, 0.0),
        (0.0, 0.0), (0.0, 0.0), (0.0, 0.0), (0.0, 0.0),
        (0.0, 0.0), (0.0, 0.0), (0.0, 0.0)])

@register_jitable
def _eqs15_19(jce):
    '''Compute eqs 15-18, in radians'''
    #mean elongation of the moon from the sun, in radians:
    # eq 15, mean elongation of the moon from the sun, in radians
    eq15_coeffs = np.array([1./189474, -0.0019142, 445267.111480, 297.85036])
    x0 = np.deg2rad(np.polyval(eq15_coeffs,jce))
    # eq 16, mean anomaly of the sun (Earth), in radians:
    eq16_coeffs = np.array([-1/3e5, -0.0001603, 35999.050340, 357.52772])
    x1 = np.deg2rad(np.polyval(eq16_coeffs, jce))
    # eq 17, mean anomaly of the moon, in radians:
    eq17_coeffs = np.array([1./56250, 0.0086972, 477198.867398, 134.96298])
    x2 = np.deg2rad(np.polyval(eq17_coeffs, jce))
    # eq 18, moon's argument of latitude, in radians:
    eq18_coeffs = np.array([1./327270, -0.0036825, 483202.017538, 93.27191])
    x3 = np.deg2rad(np.polyval(eq18_coeffs, jce))
    # eq 19, Longitude of the ascending node of the moon's mean orbit on the
    #   ecliptic measured from the mean equinox of the date, in radians:
    eq19_coeffs = np.array([1./45e4, 0.0020708, -1934.136261, 125.04452])
    x4 = np.deg2rad(np.polyval(eq19_coeffs, jce))

    return np.array([x0, x1, x2, x3, x4])

@register_jitable
def _nutation_obliquity(jce):
    """Compute the nutation in longitude (Δψ, Delta_psi) and the true obliquity of the ecliptic (ε, epsilon) given the Julian Ephemeris Century"""
    
    x = _eqs15_19(jce)

    # eq 20
    # eq 22
    a,b = _NLO_AB.T
    Delta_psi = np.sum((a + b*jce)*np.sin(np.dot(_NLO_Y, x)))/36e6
    
    # eq 21
    # eq 23
    c,d = _NLO_CD.T
    Delta_epsilon = np.sum((c + d*jce)*np.cos(np.dot(_NLO_Y, x)))/36e6
    
    epsilon = _ecliptic_obliquity(_julian_millennium(jce), Delta_epsilon)

    return Delta_psi, epsilon

## 3.5. Calculate the true obliquity of the ecliptic, ε [epsilon] (in degrees)

@register_jitable
def _ecliptic_obliquity(jme, Delta_epsilon):
    """Calculate the true obliquity of the ecliptic (ε [epsilon], in degrees) given the Julian Ephemeris Millennium and the obliquity"""
    # eq 24
    u = jme/10
    eq24_coeffs = np.array([2.45, 5.79, 27.87, 7.12, -39.05, -249.67, -51.38, 1999.25, -1.55, -4680.93, 84381.448])
    epsilon_0 = np.polyval(eq24_coeffs, u)
    # eq 25
    epsilon = epsilon_0/3600.0 + Delta_epsilon
    return epsilon

## 3.6. Calculate the aberration correction, Δτ [Delta_tau] (in degrees)

@register_jitable
def _abberation_correction(R):
    """Calculate the abberation correction (Δτ [Delta_tau], in degrees) given the Earth Heliocentric Radius (in AU)"""
    # eq 26
    Delta_tau = -20.4898/(3600*R)
    return Delta_tau

## 3.7. Calculate the apparent sun longitude, λ [lambda] (in degrees)

@register_jitable
def _sun_longitude(heliocentric_position, Delta_psi):
    """Calculate the apparent sun longitude (λ [lambda], in degrees) and geocentric latitude (β [beta], in degrees) given the earth heliocentric position and Δψ [Delta psi]"""
    R = heliocentric_position[2]
    Theta, beta = _geocentric_position(heliocentric_position)
    # eq 27
    lambda_ = Theta + Delta_psi + _abberation_correction(R)
    return lambda_, beta

## 3.8. Calculate the apparent sidereal time at Greenwich at any given time, ν [nu] (in degrees)

@register_jitable
def _greenwich_sidereal_time(jd, Delta_psi, epsilon):
    """Calculate the apparent Greenwich sidereal time (ν [nu], in degrees) given the Julian Day"""
    jc = _julian_century(jd)
    # eq 28, mean sidereal time at greenwich, in degrees:
    nu0 = (280.46061837 + 360.98564736629*(jd - 2451545) + 0.000387933*(jc**2) - (jc**3)/38710000) % 360
    # eq 29
    nu = nu0 + Delta_psi*np.cos(np.deg2rad(epsilon))
    return nu

## 3.9. Calculate the geocentric sun right ascension, α [alpha] (in degrees)
## 3.10. Calculate the geocentric sun declination, δ [delta] (in degrees)

@register_jitable
def _sun_ra_decl(lambda_, epsilon, beta):
    """Calculate the sun's geocentric right ascension (α [alpha], in degrees) and declination (δ [delta], in degrees)"""
    l = np.deg2rad(lambda_)
    e = np.deg2rad(epsilon)
    b = np.deg2rad(beta)
    # eq 30
    alpha = np.arctan2(np.sin(l)*np.cos(e) - np.tan(b)*np.sin(e), np.cos(l)) #x1 / x2
    alpha = np.rad2deg(alpha) % 360
    # eq 31
    delta = np.arcsin(np.sin(b)*np.cos(e) + np.cos(b)*np.sin(e)*np.sin(l))
    delta = np.rad2deg(delta)
    return alpha, delta

## 3.11. Calculate the observer local hour angle, H (in degrees)
## 3.12. Calculate the topocentric sun right ascension, α' [alpha prime] (in degrees)
## 3.13. Calculate the topocentric local hour angle, H' (in degrees)
@register_jitable
def _sun_topo_ra_decl_hour(latitude, longitude, elevation, jd, delta_t = 0):
    """Calculate the sun's topocentric right ascension (α' [alpha_prime]), declination (δ' [delta_prime]), and hour angle (H' [H_prime])"""
    
    jde = _julian_ephemeris_day(jd, delta_t)
    jce = _julian_century(jde)
    jme = _julian_millennium(jce)

    helio_pos = _heliocentric_position(jme)
    R = helio_pos[2]
    phi, E = np.deg2rad(latitude), elevation
    
    # eq 33. equatorial horizontal parallax of the sun, in radians
    xi = np.deg2rad(8.794/(3600*R))

    # eq 34
    u = np.arctan(0.99664719*np.tan(phi))

    # eq 35
    x = np.cos(u) + E*np.cos(phi)/6378140

    # eq 36
    y = 0.99664719*np.sin(u) + E*np.sin(phi)/6378140

    # Note: x, y = rho*sin(phi_prime), rho*cos(phi_prime)
    # rho = distance from center of earth in units of the equatorial radius
    # phi_prime = geocentric latitude
    # NB: These equations look like they're based on WGS-84, but are rounded slightly
    #   The WGS-84 reference ellipsoid has major axis a = 6378137 m, and flattening factor 1/f = 298.257223563
    #   minor axis b = a*(1-f) = 6356752.3142 = 0.996647189335*a

    Delta_psi, epsilon = _nutation_obliquity(jce)
    lambda_, beta = _sun_longitude(helio_pos, Delta_psi)
    alpha, delta = _sun_ra_decl(lambda_, epsilon, beta)
    nu = _greenwich_sidereal_time(jd, Delta_psi, epsilon)

    # eq 32
    H = nu + longitude - alpha
    
    # eq 37
    H_rad, delta_rad = np.deg2rad(H), np.deg2rad(delta)
    sin_xi = np.sin(xi)
    cos_H, sin_H = np.cos(H_rad), np.sin(H_rad)
    cos_delta, sin_delta = np.cos(delta_rad), np.sin(delta_rad)
    Delta_alpha_rad = np.arctan2(-x*sin_xi*sin_H, cos_delta-x*sin_xi*cos_H)
    Delta_alpha = np.rad2deg(Delta_alpha_rad)
    
    # eq 38
    alpha_prime = alpha + Delta_alpha

    # eq 39
    delta_prime = np.rad2deg(np.arctan2((sin_delta - y*sin_xi)*np.cos(Delta_alpha_rad), cos_delta - x*sin_xi*cos_H))

    # eq 40
    H_prime = H - Delta_alpha

    return alpha_prime, delta_prime, H_prime

## 3.14. Calculate the topocentric zenith angle, theta (in degrees)
## 3.15. Calculate the topocentric azimuth angle, Phi (in degrees)
@register_jitable
def _atmospheric_correction(e0, temperature, pressure, atmos_refract, sun_radius=0.26667):
    '''Apply atmospheric refraction correction
    In the original paper, atmospheric refraction correction is applied in all cases
    In the official C software, atmospheric refraction correction is only applied if the sun is
        above a certain elevation threshold, specified using the atmos_refract parameter, which defaults to 0.5667 deg.
        The documentation states "Set the atmos. refraction correction to zero, when sun is below horizon." and describes
        atmos_refract as "Atmospheric refraction at sunrise and sunset [degrees]".
    We observe that atmos_refract is nominally Delta_e when the sun is at the horizon (one radius below), so we can check
        post-hoc whether the adjusted elevation is below the horizon, and remove the adjustment if not. This removes the
        necessity for the atmos_refract parameter (which would otherwise need to be recomputed for various temperatures/pressures)
    
    To use the manual elevation threshold, (as per spa.c), specify atmos_refract
    To use the automatic elevation threshold, use atmos_refract=None
    '''
    # spa.c code:
    # Delta_e = 0
    # if e0 >= -1*(sun_radius + atmos_refract):
    #     Delta_e = (pressure / 1010.0) * (283.0 / (273.0 + temperature)) * 1.02 / (60.0 * np.tan(np.deg2rad(e0 + 10.3/(e0 + 5.11))))
    # return e0 + Delta_e

    # eq 42
    Delta_e = (pressure / 1010.0) * (283.0 / (273.0 + temperature)) * 1.02 / (60.0 * np.tan(np.deg2rad(e0 + 10.3/(e0 + 5.11))))
    # eq 43
    if atmos_refract is None:
        # our code path, check if the sun is above the horizon after applying the correction
        e = e0 + Delta_e
        if e < -sun_radius:
            # the sun is below the horizon, remove the correction
            e = e0
    else:
        # spa.c logic, estimate whether the sun is above the horizon using atmos_refract
        if e0 < -1*(sun_radius + atmos_refract):
            Delta_e = 0
        e = e0 + Delta_e
    return e

@register_jitable
def _sun_topo_azimuth_zenith(latitude, delta_prime, H_prime, temperature, pressure, atmos_refract):
    """Compute the sun's topocentric azimuth (Φ [Phi]) and zenith angles (θ [theta])
    azimuth is measured eastward from north, zenith from vertical
    temperature = average temperature in C (default is 14.6 = global average in 2013)
    pressure = average pressure in mBar (default 1013 = global average)
    """
    phi = np.deg2rad(latitude)
    delta_prime_rad, H_prime_rad = np.deg2rad(delta_prime), np.deg2rad(H_prime)
    
    # eq 41, topocentric elevation angle, uncorrected
    e0 = np.rad2deg(np.arcsin(np.sin(phi)*np.sin(delta_prime_rad) + np.cos(phi)*np.cos(delta_prime_rad)*np.cos(H_prime_rad)))

    e = _atmospheric_correction(e0, temperature, pressure, atmos_refract)
    Delta_e = e - e0
    theta = 90 - e

    Gamma = np.rad2deg(np.arctan2(np.sin(H_prime_rad), np.cos(H_prime_rad)*np.sin(phi) - np.tan(delta_prime_rad)*np.cos(phi))) % 360
    Phi = (Gamma + 180) % 360 #azimuth from north
    return Phi, theta, e0, Delta_e

@register_jitable
def _norm_lat_lon(lat,lon):
    '''Normalize latitude and longitude into standard ranges'''
    if lat < -90 or lat > 90:
        #convert to cartesian and back
        x = np.cos(np.deg2rad(lon))*np.cos(np.deg2rad(lat))
        y = np.sin(np.deg2rad(lon))*np.cos(np.deg2rad(lat))
        z = np.sin(np.deg2rad(lat))
        r = np.sqrt(x**2 + y**2 + z**2)
        lon = np.rad2deg(np.arctan2(y,x)) % 360
        lat = np.rad2deg(np.arcsin(z/r))
    elif lon < 0 or lon > 360:
        lon = lon % 360
    return lat,lon

@register_jitable
def _topo_sunpos(t, latitude, longitude, elevation, Delta_t):
    """compute sun topocentric right ascension (α' [alpha_prime]), declination (δ' [delta_prime]), hour angle (H' [H_prime]), all in degrees"""
    jd = _julian_day(t)
    latitude,longitude = _norm_lat_lon(latitude,longitude)
    alpha_prime, delta_prime, H_prime = _sun_topo_ra_decl_hour(latitude, longitude, elevation, jd, Delta_t)
    return alpha_prime, delta_prime, H_prime

_topo_sunpos_vec = np.vectorize(_topo_sunpos)

@njit
def _topo_sunpos_vec_jit(t, latitude, longitude, elevation, Delta_t):
    '''Compute sun topocentric coordinates; vectorized for use with Numba
    Arguments must be broadcast before calling: Numba's broadcast does not match Numpy's with scalar arguments
    Return values must be unwrapped after calling because Numba can't handle dynamic return types
    '''
    #broadcast
    out_shape = t.shape
    #flatten
    args_flat = t.flat, latitude.flat, longitude.flat, elevation.flat, Delta_t.flat
    n = len(args_flat[0])
    #allocate outputs as flat arrays
    alpha_prime, delta_prime, H_prime = np.empty(n), np.empty(n), np.empty(n)
    #do calculations over flattened inputs
    for i, arg in enumerate(zip(*args_flat)):
        alpha_prime[i], delta_prime[i], H_prime[i] = _topo_sunpos(*arg)
    #reshape to final dims
    alpha_prime = alpha_prime.reshape(out_shape)
    delta_prime = delta_prime.reshape(out_shape)
    H_prime = H_prime.reshape(out_shape)
    return alpha_prime, delta_prime, H_prime

@register_jitable
def _sunpos(t, latitude, longitude, elevation, temperature, pressure, atmos_refract, Delta_t):
    """Compute the sun's topocentric azimuth (Φ [Phi]), zenith (θ [theta]), right ascension (α' [alpha_prime]), declination (δ' [delta_prime]), and hour angle (H' [H_prime])"""
    jd = _julian_day(t)
    latitude,longitude = _norm_lat_lon(latitude,longitude)
    alpha_prime, delta_prime, H_prime = _sun_topo_ra_decl_hour(latitude, longitude, elevation, jd, Delta_t)
    Phi, theta, e0, Delta_e = _sun_topo_azimuth_zenith(latitude, delta_prime, H_prime, temperature, pressure, atmos_refract)
    return Phi, theta, alpha_prime, delta_prime, H_prime

_sunpos_vec = np.vectorize(_sunpos)

@njit
def _sunpos_vec_jit(t, latitude, longitude, elevation, temperature, pressure, atmos_refract, Delta_t):
    '''Compute azimuth,zenith,RA,ec,H; vectorized for use with Numba
    Note that arguments must be broadcast in advance as numba's broadcast does not match numpy's with scalar arguments
    Return values must be unwrapped after calling because Numba can't handle dynamic return types
    '''
    out_shape = t.shape #final output shape
    
    #flatten inputs
    args_flat = t.flat, latitude.flat, longitude.flat, elevation.flat, temperature.flat, pressure.flat, atmos_refract.flat, Delta_t.flat
    n = len(args_flat[0])
    #allocate outputs as flat arrays
    Phi, theta = np.empty(n), np.empty(n)
    alpha_prime, delta_prime, H_prime = np.empty(n), np.empty(n), np.empty(n)
    #do calculations over flattened inputs
    for i, arg in enumerate(zip(*args_flat)):
        t, lat, lon, elev, temp, press, ar, dt = arg
        Phi[i], theta[i], alpha_prime[i], delta_prime[i], H_prime[i] = _sunpos(t, lat, lon, elev, temp, press, ar, dt)
    #reshape outputs to final dimensions
    Phi, theta = Phi.reshape(out_shape), theta.reshape(out_shape)
    alpha_prime, delta_prime, H_prime = alpha_prime.reshape(out_shape), delta_prime.reshape(out_shape), H_prime.reshape(out_shape)
    return Phi, theta, alpha_prime, delta_prime, H_prime

def _intermediate_values_impl(t, latitude, longitude, elevation, temperature, pressure, atmos_refract, delta_t):
    '''Return all intermediate values for testing purposes'''
    
    #Values is a dict with keys matching the table produced by generate_test_data.py
    # NB: the meaning of atmos_refract in the parameters and atmos_refract in values are different:
    #    parameters: atmos_refract is the atmospheric refraction correction at dawn/dusk
    #    values: atmos_refract is the atmospheric refraction correction at t, the time of measurement
    values = dict(t=t,lat=latitude,lon=longitude,elev=elevation,temp=temperature,pres=pressure,refract=atmos_refract,delta_t=delta_t)
    # julian_day, julian_ephemeris_day, julian_ephemeris_century, julian_ephemeris_millennium
    jd = _julian_day(t) 
    jde = _julian_ephemeris_day(jd, delta_t)
    jce = _julian_century(jde)
    jme = _julian_millennium(jce)
    values.update(julian_day=jd,julian_eph_day=jde,julian_eph_century=jce,julian_eph_millennium=jme)
    # earth_heliocentric_longitude, earth_heliocentric_latitude, earth_radius_vector
    L,B,R = _heliocentric_position(jme) 
    values.update(earth_helio_lon=L,earth_helio_lat=B,earth_rad=R)
    # geocentric_longitude, geocentric_latitude
    Theta, beta = _geocentric_position((L,B,R))
    values.update(geo_lon=Theta,geo_lat=beta)
    # mean_elongation_moon_sun, mean_anomaly_sun, mean_anomaly_moon, argument_latitude_moon, ascending_longitude_moon,
    x0, x1, x2, x3, x4 = np.rad2deg(_eqs15_19(jce))
    values.update(mean_elongation=x0,mean_anomaly_sun=x1,mean_anomaly_moon=x2,arg_lat_moon=x3,asc_lon_moon=x4)
    # nutation_longitude, ecliptic_obliquity
    Delta_psi, epsilon = _nutation_obliquity(jce)
    values.update(nutation_lon=Delta_psi,ecliptic_obliquity=epsilon)
    # aberration_correction, apparent_sun_longitude, geocentric_latitude
    Delta_tau = _abberation_correction(R)
    lambda_, beta_2 = _sun_longitude((L,B,R), Delta_psi)
    assert beta_2 == beta ## beta is already in values, so just make sure it's the same value
    values.update(aberration_correction=Delta_tau, sun_lon=lambda_)
    # greenwich_sidereal_time, geocentric_sun_right_ascension, geocentric_sun_declination
    nu = _greenwich_sidereal_time(jd, Delta_psi, epsilon) 
    alpha, delta = _sun_ra_decl(lambda_, epsilon, beta)
    values.update(greenwich_sidereal_t=nu, geo_right_asc=alpha, geo_decl=delta)
    # topo_sun_right_ascension, topo_sun_declination, topo_local_hour_angle
    alpha_prime, delta_prime, H_prime = _sun_topo_ra_decl_hour(latitude, longitude, elevation, jd, delta_t)
    values.update(topo_right_asc=alpha_prime, topo_decl=delta_prime, topo_hour=H_prime)
    # topo_azimuth, topo_zenith, atmos_refract
    Phi, theta, e0, Delta_e = _sun_topo_azimuth_zenith(latitude,delta_prime,H_prime,temperature,pressure,atmos_refract) 
    values.update(topo_azimuth=Phi,topo_zenith=theta,topo_elevation_uncorrected=e0,atmos_refract=Delta_e)
    #values we don't compute here:
    # ecliptic_mean_obliquity,nutation_obliquity,greenwich_mean_sidereal_t,observer_hour,sun_horizontal_parallax,sun_right_asc_parallax,topo_elevation_uncorrected,topo_elevation,eq_of_t
    return values

_intermediate_values_jit = njit(_intermediate_values_impl)

def _intermediate_values(t, latitude, longitude, elevation, temperature, pressure, atmos_refract, delta_t, *, jit=None):
    t = time_to_datetime64(t).astype(np.int64)
    if jit is None:
        jit = _ENABLE_JIT
    if jit:
        return _intermediate_values_jit(t, latitude, longitude, elevation, temperature, pressure, atmos_refract, delta_t)
    else:
        return _intermediate_values_impl(t, latitude, longitude, elevation, temperature, pressure, atmos_refract, delta_t)

if __name__ == '__main__':
    sys.exit(main())
