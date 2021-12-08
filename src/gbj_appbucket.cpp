#include "gbj_appbucket.h"
const String gbj_appbucket::VERSION = "GBJ_APPBUCKET 1.0.0";

void gbj_appbucket::rainProcessTips()
{
  unsigned int tips = tips_; // Allow concurrent interruptions
  tips_ -= tips; // Leave new tips in meanwhile intact
  rainTips_ += tips;
  if (!isRain_)
  {
    SERIAL_VALUE("Rainfall", "START");
    rainfalls_++;
  }
  isRain_ = true;
  rainVolume_ = float(rainTips_ * BUCKET_FACTOR);
  rainDuration_ = (rainStop_ - rainStart_) / 1000;
  rainRateTips_ = -1;
  rainRate_ = -1;
  if (rainStop_ > rainStart_)
  {
    rainRateTips_ =
      float((rainTips_ - 1) * 3600000L) / float(rainStop_ - rainStart_);
    rainRate_ = rainRateTips_ * BUCKET_FACTOR;
  }
  SERIAL_VALUE("tips", tips);
  SERIAL_VALUE("rainTips", rainTips_);
  SERIAL_VALUE("rainVolume", rainVolume_);
  SERIAL_VALUE("rainDuration", rainDuration_);
  SERIAL_VALUE("rainRate", rainRate_);
  SERIAL_VALUE("rainRateTips", rainRateTips_);
}

void gbj_appbucket::rainDetectEnd()
{
  rainOffset_ = (millis() - rainStop_) / 1000;
  if (isRain_ && rainOffset_ > rainDelay_)
  {
    isRain_ = false;
    rainStart_ = rainStop_ = rainTips_ = rainVolume_ = rainDuration_ =
      rainRateTips_ = rainRate_ = 0;
    SERIAL_VALUE("rainfalls_", rainfalls_);
    SERIAL_VALUE("Rainfall", "STOP");
  }
}

byte gbj_appbucket::getIntens()
{
  byte thr = sizeof(rainThreshold_) / sizeof(rainThreshold_[0]) - 1;
  byte levels = sizeof(rainThreshold_[0]) / sizeof(rainThreshold_[0][0]);
  byte hour = min((byte)floor((float)rainDuration_ / 3600.0), thr);
  RainIntensity rainLevel =
    isRain_ ? RainIntensity::RAIN_UNKNOWN : RainIntensity::RAIN_NONE;
  if (rainRate_ > 0)
  {
    rainLevel = RainIntensity::RAIN_LIGHT;
    for (byte i = 0; i < levels; i++)
    {
      if (rainRate_ > rainThreshold_[hour][i])
      {
        rainLevel = (RainIntensity)(levels - i);
        break;
      }
    }
  }
  SERIAL_VALUE("rainLevel", rainLevel);
  return (byte)rainLevel;
}
