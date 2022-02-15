/*
  NAME:
  gbj_appbucket

  DESCRIPTION:
  Application library for processing tips of Rainfall Tipping Bucket.
  - The library processes tips caught with external interrupts.

  LICENSE:
  This program is free software; you can redistribute it and/or modify
  it under the terms of the license GNU GPL v3
  http://www.gnu.org/licenses/gpl-3.0.html (related to original code) and MIT
  License (MIT) for added code.

  CREDENTIALS:
  Author: Libor Gabaj
 */
#ifndef GBJ_APPBUCKET_H
#define GBJ_APPBUCKET_H

#if defined(__AVR__)
  #include <Arduino.h>
  #include <inttypes.h>
#elif defined(ESP8266)
  #include <Arduino.h>
#elif defined(ESP32)
  #include <Arduino.h>
#elif defined(PARTICLE)
  #include <Particle.h>
#endif
#include "gbj_appcore.h"
#include "gbj_serial_debug.h"

#undef SERIAL_PREFIX
#define SERIAL_PREFIX "gbj_appbucket"

class gbj_appbucket : gbj_appcore
{
public:
  static const String VERSION;

  typedef void Handler();

  struct Handlers
  {
    Handler *onRainfallStart;
    Handler *onRainfallEnd;
  };

  /*
    Constructor

    DESCRIPTION:
    Constructor creates the class instance object and sets operational
    parameters.

    PARAMETERS:
    handlers - A structure with pointers to various callback handler functions.
      - Data type: Handlers
      - Default value: structure with zeroed all handlers
      - Limited range: system address range

    RETURN: object
  */
  inline gbj_appbucket(Handlers handlers = Handlers())
  {
    handlers_ = handlers;
    setDelay();
  }

  /*
    Processing.

    DESCRIPTION:
    The method should be called in an application sketch loop.
    It processes main functionality and is control by the internal timer.

    PARAMETERS: None

    RETURN: none
  */
  inline void run()
  {
    // Measure at new tips
    if (tips_)
    {
      rainProcessTips();
    }
    rainDetectEnd();
  }

  /*
    Interruption Service Routing.

    DESCRIPTION:
    The method should be called in a main sketch ISR attached to the bucket pin.

    PARAMETERS: None

    RETURN: none
  */
  inline void isr()
  {
    if (millis() - rainStop_ < Timing::PERIOD_DEBOUNCE)
    {
      return;
    }
    tips_++;
    rainStop_ = millis();
    if (rainStart_ == 0)
    {
      rainStart_ = rainStop_;
    }
  }

  // Setters
  inline void setDelay(byte rainDelay = Timing::PERIOD_RAINFALL_END)
  {
    rainDelay_ = rainDelay * 60;
  }
  inline void setRainfalls(byte rainfalls = 0) { rainfalls_ = rainfalls; }

  // Getters
  inline bool isRain() { return isRain_; }
  inline byte getDelay() { return rainDelay_ / 60; }
  inline byte getRainfalls() { return rainfalls_; };
  inline byte getIntensity() { return (byte)rainLevel_; };
  inline unsigned long getOffset() { return rainOffset_; }
  inline unsigned int getDuration() { return rainDuration_; }
  inline unsigned int getTips() { return rainTips_; }
  inline float getVolume() { return rainVolume_; }
  inline float getRate() { return rainRate_; }
  inline float getRateTips() { return rainRateTips_; }

private:
  enum Timing : byte
  {
    PERIOD_DEBOUNCE = 250, // Debouncing delay in milliseconds
    PERIOD_RAINFALL_END = 20, // Delay between rainfalls in minutes
  };
  enum RainIntensity : byte
  {
    RAIN_NONE,
    RAIN_LIGHT,
    RAIN_SHOWER,
    RAIN_MODERATE,
    RAIN_STRONG,
    RAIN_HEAVY,
    RAIN_INTENSE,
    RAIN_TORRENTIAL,
    RAIN_UNKNOWN,
  };
  Handlers handlers_;
  const float BUCKET_FACTOR = 0.2794; // Rain millimeters per bucket tick
  volatile unsigned int tips_; // Tips since recent main processing
  volatile unsigned long rainStart_; // Timestamp of the first tip in a rain
  volatile unsigned long rainStop_; // Timestamp of the last tip in a rain
  unsigned long rainOffset_; // Time in seconds from recent tip
  unsigned int rainDuration_; // Time in seconds between first and last tip
  unsigned int rainTips_; // Tips in a rain
  unsigned int rainDelay_; // Delay between rainfalls in seconds (EEPROM)
  byte rainfalls_; // Number of rains detected (EEPROM)
  float rainVolume_; // Rain millimeters in a rain
  float rainRateTips_; // Rain speed in tips per hour
  float rainRate_; // Rain speed in millimeters per hour
  bool isRain_; // Flag about pending rainfall
  RainIntensity rainLevel_; // Level of a rain intensity

  // Rain rate level thresholds for particular hour of rain duration
  const float rainThreshold_[3][7] = {
    { 58, 23, 15, 10, 5, 1, 0 }, // 1st hour
    { 64, 30.5, 21, 14, 7.5, 1.5, 0 }, // 2-nd hour
    { 72, 33, 23.5, 11.5, 9, 2, 0 }, // 3rd or higher hour
  };

  void rainProcessTips(); // Process bucket tips
  void rainDetectEnd(); // Detect end of current rainfall
  void rainLevel(); // Calculate the rain level
};

#endif
