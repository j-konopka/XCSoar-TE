/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

// TODO code: units stuff
// - check buffer size in LongitudeToString and LattiditudeToString
// - convertion function
// - fill up UnitDescriptors with convertion factors
// - registry re-store
// - unit dialog support


//default       EU   UK   US   AUS
//altitude      m    ft   ft   m
//verticalspeed m/s  kts  kts  kts
//wind speed    km/  kts  mp   kts
//IAS           km/  kts  mp   kts
//distance      km   nm   ml   nm

#include "Units.hpp"

#include <stdio.h>
//#include <assert.h>

#include "resource.h"
#include "SettingsUser.hpp"
#include "Math/Units.h"

#include <stdlib.h>
#include <math.h>
#include <tchar.h>

CoordinateFormats_t Units::CoordinateFormat;

//SI to Local Units
double SPEEDMODIFY = TOKNOTS;
double LIFTMODIFY = TOKNOTS;
double DISTANCEMODIFY = TONAUTICALMILES;
double ALTITUDEMODIFY = TOFEET;
double TASKSPEEDMODIFY = TOKPH;

UnitDescriptor_t Units::UnitDescriptors[] = {
  { NULL, 1, 0 },
  { _T("km"), 0.001, 0 },
  { _T("nm"), 0.00053996, 0 },
  { _T("sm"), 0.0006214, 0},
  { _T("km/h"), 0.0036, 0},
  { _T("kt"), 0.001944, 0},
  { _T("mph"), 0.002237, 0},
  { _T("m/s"), 1.0, 0},
  { _T("fpm"), 3.281 / 60.0, 0},
  { _T("m"), 1.0, 0},
  { _T("ft"), 3.281, 0},
  { _T("K"), 1, 0},
  { _T(DEG)_T("C"), 1.0, -273.15},
  { _T(DEG)_T("F"), 1.8, -459.67}
};

Units_t Units::UserDistanceUnit = unKiloMeter;
Units_t Units::UserAltitudeUnit = unMeter;
Units_t Units::UserHorizontalSpeedUnit = unKiloMeterPerHour;
Units_t Units::UserVerticalSpeedUnit = unMeterPerSecond;
Units_t Units::UserWindSpeedUnit = unKiloMeterPerHour;
Units_t Units::UserTaskSpeedUnit = unKiloMeterPerHour;

void
Units::LongitudeToDMS(double Longitude, int *dd, int *mm, int *ss, bool *east)
{
  *east = (Longitude < 0 ? false : true);

  Longitude = fabs(Longitude);
  *dd = (int)Longitude;
  Longitude = (Longitude - (*dd)) * 60.0;
  *mm = (int)(Longitude);
  Longitude = (Longitude - (*mm)) * 60.0;
  *ss = (int)(Longitude + 0.5);
  if (*ss >= 60) {
    (*mm)++;
    (*ss) -= 60;
  }
  if ((*mm) >= 60) {
    (*dd)++;
    (*mm) -= 60;
  }
}

void
Units::LatitudeToDMS(double Latitude, int *dd, int *mm, int *ss, bool *north)
{
  *north = (Latitude < 0 ? false : true);

  Latitude = fabs(Latitude);
  *dd = (int)Latitude;
  Latitude = (Latitude - (*dd)) * 60.0;
  *mm = (int)(Latitude);
  Latitude = (Latitude - (*mm)) * 60.0;
  *ss = (int)(Latitude + 0.5);
  if (*ss >= 60) {
    (*mm)++;
    (*ss) -= 60;
  }
  if ((*mm) >= 60) {
    (*dd)++;
    (*mm) -= 60;
  }
}

bool
Units::LongitudeToString(double Longitude, TCHAR *Buffer, size_t size)
{
  (void)size;
  TCHAR EW[] = _T("WE");
  int dd, mm, ss;

  int sign = Longitude < 0 ? 0 : 1;
  Longitude = fabs(Longitude);

  switch (CoordinateFormat) {
  case cfDDMMSS:
    dd = (int)Longitude;
    Longitude = (Longitude - dd) * 60.0;
    mm = (int)(Longitude);
    Longitude = (Longitude - mm) * 60.0;
    ss = (int)(Longitude + 0.5);
    if (ss >= 60) {
      mm++;
      ss -= 60;
    }
    if (mm >= 60) {
      dd++;
      mm -= 60;
    }
    _stprintf(Buffer, _T("%c%03d")_T(DEG)_T("%02d'%02d\""), EW[sign],
        dd, mm, ss);
    break;

  case cfDDMMSSss:
    dd = (int)Longitude;
    Longitude = (Longitude - dd) * 60.0;
    mm = (int)(Longitude);
    Longitude = (Longitude - mm) * 60.0;
    _stprintf(Buffer, _T("%c%03d")_T(DEG)_T("%02d'%05.2f\""), EW[sign],
        dd, mm, Longitude);
    break;

  case cfDDMMmmm:
    dd = (int)Longitude;
    Longitude = (Longitude - dd) * 60.0;
    _stprintf(Buffer, _T("%c%03d")_T(DEG)_T("%06.3f'"), EW[sign], dd, Longitude);
    break;

  case cfDDdddd:
    _stprintf(Buffer, _T("%c%08.4f")_T(DEG), EW[sign], Longitude);
    break;

  default:
    //      assert(false /* undefined coordinateformat */);
    break;
  }

  return true;
}

bool
Units::LatitudeToString(double Latitude, TCHAR *Buffer, size_t size)
{
  (void)size;
  TCHAR EW[] = _T("SN");
  int dd, mm, ss;

  int sign = Latitude < 0 ? 0 : 1;
  Latitude = fabs(Latitude);

  switch (CoordinateFormat) {
  case cfDDMMSS:
    dd = (int)Latitude;
    Latitude = (Latitude - dd) * 60.0;
    mm = (int)(Latitude);
    Latitude = (Latitude - mm) * 60.0;
    ss = (int)(Latitude + 0.5);
    if (ss >= 60) {
      mm++;
      ss -= 60;
    }
    if (mm >= 60) {
      dd++;
      mm -= 60;
    }
    _stprintf(Buffer, _T("%c%02d")_T(DEG)_T("%02d'%02d\""), EW[sign],
        dd, mm, ss);
    break;

  case cfDDMMSSss:
    dd = (int)Latitude;
    Latitude = (Latitude - dd) * 60.0;
    mm = (int)(Latitude);
    Latitude = (Latitude - mm) * 60.0;
    _stprintf(Buffer, _T("%c%02d")_T(DEG)_T("%02d'%05.2f\""), EW[sign],
        dd, mm, Latitude);
    break;

  case cfDDMMmmm:
    dd = (int)Latitude;
    Latitude = (Latitude - dd) * 60.0;
    _stprintf(Buffer, _T("%c%02d")_T(DEG)_T("%06.3f'"), EW[sign], dd, Latitude);
    break;

  case cfDDdddd:
    _stprintf(Buffer, _T("%c%07.4f")_T(DEG), EW[sign], Latitude);
    break;

  default:
    // assert(false /* undefined coordinateformat */);
    break;
  }

  return true;
}

const TCHAR *
Units::GetUnitName(Units_t Unit)
{
  //  return(gettext(UnitDescriptors[Unit].Name));
  // JMW adjusted this because units are pretty standard internationally
  // so don't need different names in different languages.
  return UnitDescriptors[Unit].Name;
}

Units_t
Units::GetUserDistanceUnit(void)
{
  return UserDistanceUnit;
}

Units_t
Units::SetUserDistanceUnit(Units_t NewUnit)
{
  Units_t last = UserDistanceUnit;
  if (UserDistanceUnit != NewUnit) {
    UserDistanceUnit = NewUnit;
    NotifyUnitChanged();
  }
  return last;
}

Units_t
Units::GetUserAltitudeUnit(void)
{
  return UserAltitudeUnit;
}

Units_t
Units::SetUserAltitudeUnit(Units_t NewUnit)
{
  Units_t last = UserAltitudeUnit;
  if (UserAltitudeUnit != NewUnit) {
    UserAltitudeUnit = NewUnit;
    NotifyUnitChanged();
  }
  return last;
}

Units_t
Units::GetUserHorizontalSpeedUnit(void)
{
  return UserHorizontalSpeedUnit;
}

Units_t
Units::SetUserHorizontalSpeedUnit(Units_t NewUnit)
{
  Units_t last = UserHorizontalSpeedUnit;
  if (UserHorizontalSpeedUnit != NewUnit) {
    UserHorizontalSpeedUnit = NewUnit;
    NotifyUnitChanged();
  }
  return last;
}

Units_t
Units::GetUserTaskSpeedUnit(void)
{
  return UserTaskSpeedUnit;
}

Units_t
Units::SetUserTaskSpeedUnit(Units_t NewUnit)
{
  Units_t last = UserTaskSpeedUnit;
  if (UserTaskSpeedUnit != NewUnit) {
    UserTaskSpeedUnit = NewUnit;
    NotifyUnitChanged();
  }
  return last;
}

Units_t
Units::GetUserVerticalSpeedUnit(void)
{
  return UserVerticalSpeedUnit;
}

Units_t
Units::SetUserVerticalSpeedUnit(Units_t NewUnit)
{
  Units_t last = UserVerticalSpeedUnit;
  if (UserVerticalSpeedUnit != NewUnit) {
    UserVerticalSpeedUnit = NewUnit;
    NotifyUnitChanged();
  }
  return last;
}

Units_t
Units::GetUserWindSpeedUnit(void)
{
  return UserWindSpeedUnit;
}

Units_t
Units::SetUserWindSpeedUnit(Units_t NewUnit)
{
  Units_t last = UserWindSpeedUnit;
  if (UserWindSpeedUnit != NewUnit) {
    UserWindSpeedUnit = NewUnit;
    NotifyUnitChanged();
  }
  return last;
}

Units_t
Units::GetUserUnitByGroup(UnitGroup_t UnitGroup)
{
  switch (UnitGroup) {
  case ugNone:
    return (unUndef);
  case ugDistance:
    return (GetUserDistanceUnit());
  case ugAltitude:
    return (GetUserAltitudeUnit());
  case ugHorizontalSpeed:
    return (GetUserHorizontalSpeedUnit());
  case ugVerticalSpeed:
    return (GetUserVerticalSpeedUnit());
  case ugWindSpeed:
    return (GetUserWindSpeedUnit());
  case ugTaskSpeed:
    return (GetUserTaskSpeedUnit());
  default:
    return (unUndef);
  }
}

void
Units::NotifyUnitChanged(void)
{
  // todo

  if (SPEEDMODIFY == TOMPH) {
    SetUserHorizontalSpeedUnit(unStatuteMilesPerHour);
    SetUserWindSpeedUnit(unStatuteMilesPerHour);
  } else if (SPEEDMODIFY == TOKNOTS) {
    SetUserHorizontalSpeedUnit(unKnots);
    SetUserWindSpeedUnit(unKnots);
  } else if (SPEEDMODIFY == TOKPH) {
    SetUserHorizontalSpeedUnit(unKiloMeterPerHour);
    SetUserWindSpeedUnit(unKiloMeterPerHour);
  }

  if (DISTANCEMODIFY == TOMILES) {
    SetUserDistanceUnit(unStatuteMiles);
  } else if (DISTANCEMODIFY == TONAUTICALMILES) {
    SetUserDistanceUnit(unNauticalMiles);
  } else if (DISTANCEMODIFY == TOKILOMETER) {
    SetUserDistanceUnit(unKiloMeter);
  }

  if (ALTITUDEMODIFY == TOFEET) {
    SetUserAltitudeUnit(unFeet);
  } else if (ALTITUDEMODIFY == TOMETER) {
    SetUserAltitudeUnit(unMeter);
  }

  if (LIFTMODIFY == TOKNOTS) {
    SetUserVerticalSpeedUnit(unKnots);
  } else if (LIFTMODIFY==TOMETER) {
    SetUserVerticalSpeedUnit(unMeterPerSecond);
  }

  if (TASKSPEEDMODIFY == TOMPH) {
    SetUserTaskSpeedUnit(unStatuteMilesPerHour);
  } else if (TASKSPEEDMODIFY == TOKNOTS) {
    SetUserTaskSpeedUnit(unKnots);
  } else if (TASKSPEEDMODIFY == TOKPH) {
    SetUserTaskSpeedUnit(unKiloMeterPerHour);
  }
}

const TCHAR *
Units::GetHorizontalSpeedName()
{
  return GetUnitName(GetUserHorizontalSpeedUnit());
}

const TCHAR *
Units::GetVerticalSpeedName()
{
  return GetUnitName(GetUserVerticalSpeedUnit());
}

const TCHAR *
Units::GetDistanceName()
{
  return GetUnitName(GetUserDistanceUnit());
}

const TCHAR *
Units::GetAltitudeName()
{
  return GetUnitName(GetUserAltitudeUnit());
}

const TCHAR *
Units::GetTaskSpeedName()
{
  return GetUnitName(GetUserTaskSpeedUnit());
}

bool
Units::FormatUserAltitude(double Altitude, TCHAR *Buffer, size_t size)
{
  int prec;
  TCHAR sTmp[32];
  UnitDescriptor_t *pU = &UnitDescriptors[UserAltitudeUnit];

  Altitude = Altitude * pU->ToUserFact; // + pU->ToUserOffset;

  // prec = 4-log10(Altitude);
  // prec = max(prec, 0);
  prec = 0;

  _stprintf(sTmp, _T("%.*f%s"), prec, Altitude, pU->Name);

  if (_tcslen(sTmp) < size - 1) {
    _tcscpy(Buffer, sTmp);
    return true;
  } else {
    _tcsncpy(Buffer, sTmp, size);
    Buffer[size - 1] = '\0';
    return false;
  }
}

bool
Units::FormatAlternateUserAltitude(double Altitude, TCHAR *Buffer, size_t size)
{
  Units_t saveUnit = UserAltitudeUnit;
  bool res;

  if (saveUnit == unMeter)
    UserAltitudeUnit = unFeet;
  if (saveUnit == unFeet)
    UserAltitudeUnit = unMeter;

  res = FormatUserAltitude(Altitude, Buffer, size);

  UserAltitudeUnit = saveUnit;

  return res;
}

// JMW, what does this do?
bool
Units::FormatUserArrival(double Altitude, TCHAR *Buffer, size_t size)
{
  int prec;
  TCHAR sTmp[32];
  UnitDescriptor_t *pU = &UnitDescriptors[UserAltitudeUnit];

  Altitude = Altitude * pU->ToUserFact; // + pU->ToUserOffset;

  // prec = 4-log10(Altitude);
  // prec = max(prec, 0);
  prec = 0;

  _stprintf(sTmp, _T("%+.*f%s"), prec, Altitude, pU->Name);

  if (_tcslen(sTmp) < size - 1) {
    _tcscpy(Buffer, sTmp);
    return true;
  } else {
    _tcsncpy(Buffer, sTmp, size);
    Buffer[size - 1] = '\0';
    return false;
  }
}

bool
Units::FormatUserDistance(double Distance, TCHAR *Buffer, size_t size)
{
  int prec;
  double value;
  TCHAR sTmp[32];

  UnitDescriptor_t *pU = &UnitDescriptors[UserDistanceUnit];

  value = Distance * pU->ToUserFact; // + pU->ToUserOffset;

  if (value >= 100)
    prec = 0;
  else if (value > 10)
    prec = 1;
  else if (value > 1)
    prec = 2;
  else {
    prec = 3;
    if (UserDistanceUnit == unKiloMeter) {
      prec = 0;
      pU = &UnitDescriptors[unMeter];
      value = Distance * pU->ToUserFact;
    }
    if (UserDistanceUnit == unNauticalMiles ||
        UserDistanceUnit == unStatuteMiles) {
      pU = &UnitDescriptors[unFeet];
      value = Distance * pU->ToUserFact;
      if (value < 1000) {
        prec = 0;
      } else {
        prec = 1;
        pU = &UnitDescriptors[UserDistanceUnit];
        value = Distance * pU->ToUserFact;
      }
    }
  }

  _stprintf(sTmp, _T("%.*f%s"), prec, value, pU->Name);

  if (_tcslen(sTmp) < size - 1) {
    _tcscpy(Buffer, sTmp);
    return true;
  } else {
    _tcsncpy(Buffer, sTmp, size);
    Buffer[size - 1] = '\0';
    return false;
  }
}

bool
Units::FormatUserMapScale(Units_t *Unit, double Distance, TCHAR *Buffer,
                          size_t size)
{
  int prec;
  double value;
  TCHAR sTmp[32];

  UnitDescriptor_t *pU = &UnitDescriptors[UserDistanceUnit];

  if (Unit != NULL)
    *Unit = UserDistanceUnit;

  value = Distance * pU->ToUserFact; // + pU->ToUserOffset;

  if (value >= 9.999)
    prec = 0;
  else if ((UserDistanceUnit == unKiloMeter && value >= 0.999) ||
           (UserDistanceUnit != unKiloMeter && value >= 0.160))
    prec = 1;
  else {
    prec = 2;
    if (UserDistanceUnit == unKiloMeter) {
      prec = 0;
      if (Unit != NULL)
        *Unit = unMeter;
      pU = &UnitDescriptors[unMeter];
      value = Distance * pU->ToUserFact;
    }
    if (UserDistanceUnit == unNauticalMiles ||
        UserDistanceUnit == unStatuteMiles) {
      prec = 0;
      if (Unit != NULL)
        *Unit = unFeet;
      pU = &UnitDescriptors[unFeet];
      value = Distance * pU->ToUserFact;
    }
  }

  // _stprintf(sTmp, _T("%.*f%s"), prec, value, pU->Name);
  _stprintf(sTmp, _T("%.*f"), prec, value);

  if (_tcslen(sTmp) < size - 1) {
    _tcscpy(Buffer, sTmp);
    return true;
  } else {
    _tcsncpy(Buffer, sTmp, size);
    Buffer[size - 1] = '\0';
    return false;
  }
}

double
Units::ToUserAltitude(double Altitude)
{
  UnitDescriptor_t *pU = &UnitDescriptors[UserAltitudeUnit];
  Altitude = Altitude * pU->ToUserFact; // + pU->ToUserOffset;
  return Altitude;
}

double
Units::ToSysAltitude(double Altitude)
{
  UnitDescriptor_t *pU = &UnitDescriptors[UserAltitudeUnit];
  Altitude = Altitude / pU->ToUserFact; // + pU->ToUserOffset;
  return Altitude;
}

double
Units::ToUserDistance(double Distance)
{
  UnitDescriptor_t *pU = &UnitDescriptors[UserDistanceUnit];
  Distance = Distance * pU->ToUserFact; // + pU->ToUserOffset;
  return Distance;
}

double
Units::ToSysDistance(double Distance)
{
  UnitDescriptor_t *pU = &UnitDescriptors[UserDistanceUnit];
  Distance = Distance / pU->ToUserFact; // + pU->ToUserOffset;
  return Distance;
}

void
Units::TimeToText(TCHAR* text, int d)
{
  int hours, mins;
  bool negative = (d < 0);
  int dd = abs(d) % (3600 * 24);
  hours = (dd / 3600);
  mins = (dd / 60 - hours * 60);
  hours = hours % 24;
  if (negative) {
    _stprintf(text, _T("-%02d:%02d"), hours, mins);
  } else {
    _stprintf(text, _T("%02d:%02d"), hours, mins);
  }
}
