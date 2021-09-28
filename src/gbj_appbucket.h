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

  inline gbj_appbucket() { _timer = new gbj_timer(Timing::PERIOD_CHECK); }

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
    if (_timer->run())
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
    if (millis() - _rainStop < Timing::PERIOD_DEBOUNCE)
    {
      return;
    }
    _tips++;
    _rainStop = millis();
    if (_rainStart == 0)
    {
      _rainStart = _rainStop;
    }
  }

  // Setters
  inline void setPeriod(unsigned long period) { _timer->setPeriod(period); }
  inline void setDelay(byte delay) { _rainDelay = delay * 60000; }

  // Getters
  inline unsigned long getPeriod() { return _timer->getPeriod(); }
  inline bool isRain() { return _isRain; }
  inline unsigned int getDuration() { return _rainDuration; }
  inline float getVolume() { return _rainVolume; }
  inline float getSpeed() { return _rainSpeedVolume; }
  inline byte getDelay() { return _rainDelay / 60000; }

private:
  enum Timing : unsigned long
  {
    PERIOD_CHECK = 5147, // Prime number - Avoid useless collisions
    PERIOD_DEBOUNCE = 50, // Debouncing delay
    PERIOD_RAINFALL = 300000L, // 5 minutes - Delay between rainfalls
  };
  gbj_timer *_timer;
  const float BUCKET_FACTOR = 0.2794; // Rain millimeters per bucket tick
  const byte RAIN_TIPS = 3; // Minimal number of tips for rainfall recognition
  volatile unsigned int _tips; // Tips since recent main processing
  volatile unsigned long _rainStart; // Timestamp of the first tip in a rain
  volatile unsigned long _rainStop; // Timestamp of the last tip in a rain
  unsigned int _rainTips; // Tips in a rain
  unsigned int _rainDuration; // Duration of a rain in seconds
  unsigned long _rainDelay = Timing::PERIOD_RAINFALL; // Delay between rainfalls
  float _rainVolume; // Rain millimeters in a rain
  float _rainSpeedTips; // Rain speed in tips per hour
  float _rainSpeedVolume; // Rain speed in millimeters per hour
  bool _isRain; // Flag about pending rainfall

  void measure();
};

#endif
