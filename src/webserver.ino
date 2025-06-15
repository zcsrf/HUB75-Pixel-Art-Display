/*
// parses and processes webpages
// if the webpage has %SOMETHING% or %SOMETHINGELSE% it will replace those strings with the ones defined
String processor(const String &var)
{
  if (var == "FIRMWARE")
  {
    return FIRMWARE_VERSION;
  }

  if (var == "FREEFLASH")
  {
    return humanReadableSize((LittleFS.totalBytes() - LittleFS.usedBytes()));
  }

  if (var == "USEDFLASH")
  {
    return humanReadableSize(LittleFS.usedBytes());
  }

  if (var == "TOTALFLASH")
  {
    return humanReadableSize(LittleFS.totalBytes());
  }

  if (var == "SLIDERVALUE")
  {
    return sliderValue;
  }
  return String();
}

*/