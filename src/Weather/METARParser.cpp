/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#include "METARParser.hpp"
#include "METAR.hpp"
#include "ParsedMETAR.hpp"
#include "Units/Units.hpp"

#include <tchar.h>

class METARLine {
protected:
  TCHAR *start, *data, *end;

public:
  METARLine(const TCHAR *line)
    :start(_tcsdup(line)), data(start), end(start + _tcslen(line)) {}

  ~METARLine() {
    free(start);
  }

  const TCHAR *Next() {
    if (data >= end)
      return NULL;

    const TCHAR *start = data;

    TCHAR *seperator = _tcschr(data, _T(' '));
    if (seperator != NULL && seperator < end) {
      *seperator = _T('\0');
      data = seperator + 1;
    } else {
      data = end;
    }

    return start;
  }
};

static bool
DetectICAOCodeToken(const TCHAR *token)
{
  if (_tcslen(token) != 4)
    return false;

  for (unsigned i = 0; i < 4; i++) {
    if ((token[i] >= 'A' && token[i] <= 'Z') ||
        (token[i] >= 'a' && token[i] <= 'z')) {
      // okay
    } else
      return false;
  }

  return true;
}

static bool
DetectTimeCodeToken(const TCHAR *token)
{
  if (_tcslen(token) != 7)
    return false;

  return token[6] == _T('Z') || token[6] == _T('z');
}

static bool
ParseTimeCode(const TCHAR *token, ParsedMETAR &parsed)
{
  assert(DetectTimeCodeToken(token));

  TCHAR *endptr;
  unsigned time_code = _tcstod(token, &endptr);
  if (endptr == NULL || endptr == token)
    return false;

  parsed.day_of_month = (int)(time_code / 10000);
  time_code -= parsed.day_of_month * 10000;
  parsed.hour = (int)(time_code / 100);
  time_code -= parsed.hour * 100;
  parsed.minute = time_code;

  return true;
}

static bool
DetectWindToken(const TCHAR *token)
{
  unsigned length = _tcslen(token);

  if (length == 8)
    return _tcsicmp(token + 5, _T("MPS")) == 0;

  if (length == 7)
    return _tcsicmp(token + 5, _T("KT")) == 0;

  return false;
}

static bool
ParseWind(const TCHAR *token, ParsedMETAR &parsed)
{
  assert(DetectWindToken(token));

  TCHAR *endptr;
  unsigned wind_code = _tcstod(token, &endptr);
  if (endptr == NULL || endptr == token)
    return false;

  unsigned bearing = (int)(wind_code / 100);
  wind_code -= bearing * 100;

  if (_tcsicmp(endptr, _T("MPS")) == 0)
    parsed.wind.norm = fixed(wind_code);
  else if (_tcsicmp(endptr, _T("KT")) == 0)
    parsed.wind.norm = Units::ToSysUnit(fixed(wind_code), unKnots);
  else
    return false;

  parsed.wind.bearing = Angle::degrees(fixed(bearing));
  parsed.wind_available = true;
  return true;
}

static bool
DetectQNHToken(const TCHAR *token)
{
  unsigned length = _tcslen(token);

  // International style
  if (token[0] == _T('Q') || token[0] == _T('q'))
    return length <= 5 && length >= 4;

  // American style
  if (token[0] == _T('A') || token[0] == _T('a'))
    return length == 5;

  return false;
}

static bool
ParseQNH(const TCHAR *token, ParsedMETAR &parsed)
{
  assert(DetectQNHToken(token));

  // International style (hPa)
  if (token[0] == _T('Q') || token[0] == _T('q')) {
    token++;

    TCHAR *endptr;
    unsigned hpa = _tcstod(token, &endptr);
    if (endptr == NULL || endptr == token)
      return false;

    parsed.qnh.set_QNH(fixed(hpa));
    parsed.qnh_available = true;
    return true;
  }

  // American style (inHg)
  if (token[0] == _T('A') || token[0] == _T('a')) {
    token++;

    TCHAR *endptr;
    unsigned inch_hg = _tcstod(token, &endptr);
    if (endptr == NULL || endptr == token)
      return false;

    parsed.qnh.set_QNH(Units::ToSysUnit(fixed(inch_hg), unInchMercurial));
    parsed.qnh_available = true;
    return true;
  }

  return false;
}

bool
METARParser::Parse(const METAR &metar, ParsedMETAR &parsed)
{
  // Examples:
  // EDDL 231050Z 31007KT 9999 FEW020 SCT130 23/18 Q1013 NOSIG
  // METAR ETOU 231055Z AUTO 15004KT 9999 FEW130 27/19 A2993 RMK AO2 RAB1038E1048DZB1006E1011 SLP128 P0000 T02710189=
  // METAR KTTN 051853Z 04011KT 1/2SM VCTS SN FZFG BKN003 OVC010 M02/M02 A3006 RMK AO2 TSB40 SLP176 P0002 T10171017=

  parsed.Reset();

  METARLine line(metar.content.begin());
  const TCHAR *token;

  // Parse four-letter ICAO code
  while ((token = line.Next()) != NULL) {
    if (DetectICAOCodeToken(token)) {
      parsed.icao_code = token;
      break;
    }
  }

  // Parse day of month, hour and minute
  if ((token = line.Next()) == NULL)
    return false;

  if (!DetectTimeCodeToken(token))
    return false;

  ParseTimeCode(token, parsed);

  // Parse Wind
  while ((token = line.Next()) != NULL) {
    if (DetectWindToken(token)) {
      if (!ParseWind(token, parsed))
        return false;

      break;
    }
  }

  // Parse QNH
  while ((token = line.Next()) != NULL) {
    if (DetectQNHToken(token)) {
      if (!ParseQNH(token, parsed))
        return false;

      break;
    }
  }

  return true;
}
