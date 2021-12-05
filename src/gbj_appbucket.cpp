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
  }
  isRain_ = true;
  rainVolume_ = float(rainTips_ * BUCKET_FACTOR);
  rainSpan_ = (rainStop_ - rainStart_) / 1000;
  SERIAL_VALUE("tips", tips);
  SERIAL_VALUE("rainTips", rainTips_);
  SERIAL_VALUE("rainVolume", rainVolume_);
  SERIAL_VALUE("rainSpan", rainSpan_);
  rainCalculate();
}

void gbj_appbucket::rainCalculate()
{
  if (isRain_)
  {
    rainDuration_ = (millis() - rainStart_) / 1000;
    SERIAL_VALUE("rainDuration", rainDuration_);
    if (rainDuration_)
    {
      rainRateTips_ = float((rainTips_ - 1) * 3600) / float(rainDuration_);
      rainRate_ = rainRateTips_ * BUCKET_FACTOR;
      SERIAL_VALUE("rainRate", rainRate_);
      SERIAL_VALUE("rainRateTips", rainRateTips_);
    }
  }
}

void gbj_appbucket::rainDetectEnd()
{
  if (isRain_ && millis() - rainStop_ > rainDelay_)
  {
    isRain_ = false;
    rainStart_ = rainStop_ = rainTips_ = rainVolume_ = rainDuration_ =
      rainRateTips_ = rainRate_ = 0;
    SERIAL_VALUE("Rainfall", "STOP");
  }
}

gbj_appbucket::RainIntensity gbj_appbucket::rainIntensity(float rainRate)
{
  if (rainRate > 30.0)
  {
    return RainIntensity::RAIN_TORRENTIAL;
  }
  else if (rainRate > 15.0)
  {
    return RainIntensity::RAIN_INTENSE;
  }
  else if (rainRate > 7.5)
  {
    return RainIntensity::RAIN_HEAVY;
  }
  else if (rainRate > 2.5)
  {
    return RainIntensity::RAIN_MODERATE;
  }
  else if (rainRate > 0.0)
  {
    return RainIntensity::RAIN_LIGHT;
  }
  else
  {
    return RainIntensity::RAIN_NONE;
  }
}
