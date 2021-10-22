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
#include "gbj_appbase.h"
#include "gbj_serial_debug.h"
#include "gbj_timer.h"

#undef SERIAL_PREFIX
#define SERIAL_PREFIX "gbj_appbucket"

class gbj_appbucket : gbj_appbase
{
public:
  static const String VERSION;

  inline gbj_appbucket()
  {
    setDelay();
    timer_ = new gbj_timer(Timing::PERIOD_CHECK);
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
    if (timer_->run())
    {
      measure();
    }
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
  inline void setPeriod(unsigned long period) { timer_->setPeriod(period); }
  inline void setDelay(byte delay = Timing::PERIOD_RAINFALL)
  {
    rainDelay_ = delay * 60000;
  }

  // Getters
  inline unsigned long getPeriod() { return timer_->getPeriod(); }
  inline bool isRain() { return isRain_; }
  inline unsigned int getDuration() { return rainDuration_; }
  inline float getVolume() { return rainVolume_; }
  inline float getSpeed() { return rainSpeedVolume_; }
  inline byte getDelay() { return rainDelay_ / 60000; }

private:
  enum Timing : unsigned int
  {
    PERIOD_CHECK = 5147, // Prime number - Avoid useless collisions
    PERIOD_DEBOUNCE = 50, // Debouncing delay in milliseconds
    PERIOD_RAINFALL = 5, // Delay between rainfalls in minutes
  };
  gbj_timer *timer_;
  const float BUCKET_FACTOR = 0.2794; // Rain millimeters per bucket tick
  const byte RAIN_TIPS = 3; // Minimal number of tips for rainfall recognition
  volatile unsigned int tips_; // Tips since recent main processing
  volatile unsigned long rainStart_; // Timestamp of the first tip in a rain
  volatile unsigned long rainStop_; // Timestamp of the last tip in a rain
  unsigned int rainTips_; // Tips in a rain
  unsigned int rainDuration_; // Duration of a rain in seconds
  unsigned long rainDelay_; // Delay between rainfalls in milliseconds
  float rainVolume_; // Rain millimeters in a rain
  float rainSpeedTips_; // Rain speed in tips per hour
  float rainSpeedVolume_; // Rain speed in millimeters per hour
  bool isRain_; // Flag about pending rainfall

  void measure();
};

#endif
