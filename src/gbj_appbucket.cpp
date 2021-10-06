#include "gbj_appbucket.h"
const String gbj_appbucket::VERSION = "GBJ_APPBUCKET 1.0.0";

void gbj_appbucket::measure()
{
  // No new tips since recent processing
  SERIAL_TITLE("measure");
  if (_tips)
  {
    unsigned int tips = _tips; // Allow concurrent interruptions
    _rainTips += tips;
    _tips -= tips; // Leave new tips in meanwhile intact
    SERIAL_VALUE("tips", tips);
    SERIAL_VALUE("rainTips", _rainTips);
    // Detect start of a rainfall
    if (!_isRain && _rainTips >= RAIN_TIPS)
    {
      _isRain = true;
      SERIAL_VALUE("Rainfall", "START");
    }
    // Evaluate pending rainfall
    if (_isRain)
    {
      _rainVolume = float(_rainTips * BUCKET_FACTOR);
      _rainDuration = (_rainStop - _rainStart) / 1000;
      SERIAL_VALUE("rainVolume", _rainVolume);
      SERIAL_VALUE("rainDuration", _rainDuration);
      if (_rainDuration)
      {
        _rainSpeedVolume = float(_rainVolume * 3600) / float(_rainDuration);
        SERIAL_VALUE("rainSpeedVolume", _rainSpeedVolume);
      }
    }
  }
  // Detect end of a rainfall or false one
  if ((_isRain || _rainTips > 0) && millis() - _rainStop > _rainDelay)
  {
    _isRain = false;
    _rainStart = _rainStop = _rainTips = _rainVolume = _rainDuration =
      _rainSpeedTips = _rainSpeedVolume = 0;
    SERIAL_VALUE("Rainfall", "STOP");
  }
}
