#pragma once

#include <Arduino.h>

String extractEndpointFromURL(const String &url)
{
  // 1. If it's already just a path, normalize and return it
  if (url.startsWith("/"))
    return url;

  // 2. Strip protocol if present
  int protoPos = url.indexOf("://");
  int start = 0;
  if (protoPos >= 0)
    start = protoPos + 3; // skip "http://" or "https://"

  // 3. Find first slash after domain
  int slashPos = url.indexOf('/', start);

  if (slashPos < 0)
  {
    // URL has no path → endpoint is root "/"
    return "/";
  }

  // 4. Extract everything from the slash onward (already a path)
  return url.substring(slashPos);
}

// Time Helpers
String formatTimeISO8601(time_t t)
{
  char buffer[25];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d.000Z",
           year(t), month(t), day(t), hour(t), minute(t), second(t));
  return String(buffer);
}

time_t formatTimeFromISO8601(String timestamp)
{
  int Year, Month, Day, Hour, Minute, Second;
  sscanf(timestamp.c_str(), "%04d-%02d-%02dT%02d:%02d:%02d.000Z",
         &Year, &Month, &Day, &Hour, &Minute, &Second);
  tmElements_t tm;
  tm.Year = Year - 1970;
  tm.Month = Month;
  tm.Day = Day;
  tm.Hour = Hour;
  tm.Minute = Minute;
  tm.Second = Second;
  return makeTime(tm);
}