#include "gbj_appbucket.h"
const String gbj_appbucket::VERSION = "GBJ_APPBUCKET 1.0.0";

void gbj_appbucket::measure()
{
  // No new tips since recent processing
  SERIAL_TITLE("measure");
  if (tips_)
  {
    unsigned int tips = tips_; // Allow concurrent interruptions
    rainTips_ += tips;
    tips_ -= tips; // Leave new tips in meanwhile intact
    SERIAL_VALUE("tips", tips);
    SERIAL_VALUE("rainTips", rainTips_);
    // Detect start of a rainfall
    if (!isRain_ && rainTips_ >= RAIN_TIPS)
    {
      isRain_ = true;
      SERIAL_VALUE("Rainfall", "START");
    }
    // Evaluate pending rainfall
    if (isRain_)
    {
      rainVolume_ = float(rainTips_ * BUCKET_FACTOR);
      rainDuration_ = (rainStop_ - rainStart_) / 1000;
      SERIAL_VALUE("rainVolume", rainVolume_);
      SERIAL_VALUE("rainDuration", rainDuration_);
      if (rainDuration_)
      {
        rainSpeedVolume_ = float(rainVolume_ * 3600) / float(rainDuration_);
        SERIAL_VALUE("rainSpeedVolume", rainSpeedVolume_);
      }
    }
  }
  // Detect end of a rainfall or false one
  if ((isRain_ || rainTips_ > 0) && millis() - rainStop_ > rainDelay_)
  {
    isRain_ = false;
    rainStart_ = rainStop_ = rainTips_ = rainVolume_ = rainDuration_ =
      rainSpeedTips_ = rainSpeedVolume_ = 0;
    SERIAL_VALUE("Rainfall", "STOP");
  }
}
