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

#include <Arduino.h>
#if defined(__AVR__)
  #include <inttypes.h>
#endif
#include "gbj_appcore.h"
#include "gbj_serial_debug.h"

#undef SERIAL_PREFIX
#define SERIAL_PREFIX "gbj_appbucket"
//******************************************************************************
// Class definition
//******************************************************************************
class gbj_appbucket : gbj_appcore
{
public:
  typedef void Handler();

  struct Handlers
  {
    Handler *onRainfallStart;
    Handler *onRainfallStop;
    Handler *onEvaluate;
  };
  /*
    Constructor

    DESCRIPTION:
    Constructor creates the class instance object and sets operational
    parameters.

    PARAMETERS:
    rainfallOffset - Time in minutes from recent tip to determine end of a
    rainfall.
       - Data type: constant string
       - Default value: none
       - Limited range: none
    handlers - A structure with pointers to various callback handler functions.
      - Data type: Handlers
      - Default value: structure with zeroed all handlers
      - Limited range: system address range


    RETURN: object
  */
  inline gbj_appbucket(word rainfallOffset, Handlers handlers = Handlers())
  {
    rain_.offset = rainfallOffset;
    handlers_ = handlers;
  }

  /*
    Interruption Service Routing.

    DESCRIPTION:
    The method collects random tips from a rain tip bucket.
    - The method should be called in a main sketch ISR attached to the bucket
    pin.

    PARAMETERS: None

    RETURN: none
  */
  inline void isr()
  {
    if (millis() - isr_.tsStop < Timing::PERIOD_DEBOUNCE)
    {
      return;
    }
    SERIAL_TITLE("ISR")
    isr_.tips++;
    isr_.tsStop = millis();
    if (isr_.tsStart == 0)
    {
      isr_.tsStart = isr_.tsStop;
    }
  }

  /*
    Processing.

    DESCRIPTION:
    The method should be called frequently either in an application sketch loop
    or in a timer handler.

    PARAMETERS: None

    RETURN: None
  */
  inline void run()
  {
    if (isr_.tips > 0)
    {
      rainEvaluate();
    }
    rainfallEnd();
  }

  // Getters
  inline bool isRain() { return rain_.flPending; }
  inline word getRainDuration() { return rain_.duration; }
  inline float getRainVolume() { return rain_.volume; }
  inline float getRainRate() { return rain_.rate; }
  inline unsigned long getRainStart() { return isr_.tsStart; }
  inline unsigned long getRainStop() { return isr_.tsStop; }

private:
  enum Timing : word
  {
    // Debouncing delay in milliseconds
    PERIOD_DEBOUNCE = 1000,
  };
  struct Isr
  {
    volatile word tips;
    volatile unsigned long tsStart;
    volatile unsigned long tsStop;
    void reset() { tips = tsStart = tsStop = 0; }
  } isr_;
  struct Rain
  {
    // Number of collected bucket tips
    word tips;
    // Delay in minutes from recent tip determining rainfall end
    word offset;
    // Overall rain time in seconds
    unsigned long duration;
    // Millimeters
    float volume;
    // Rain intensity in millimeters per hour
    float rate;
    // Flag about pending rainfall
    bool flPending;
    void reset() { tips = duration = volume = rate = 0; }
  } rain_;
  // Rain millimeters per bucket tick
  const float BUCKET_FACTOR = 0.2794;
  Handlers handlers_;
  /*
    Rain evaluation.

    DESCRIPTION:
    The method evaluates the rain tips collected so far.

    PARAMETERS: None

    RETURN: none
  */
  void rainEvaluate()
  {
    // Allow concurrent interruptions
    word tips = isr_.tips;
    // Leave new tips in meanwhile intact
    isr_.tips -= tips;
    // Determine rain
    if (!rain_.flPending)
    {
      rain_.flPending = true;
      SERIAL_VALUE("Rainfall", "START")
      if (handlers_.onRainfallStart != nullptr)
      {
        handlers_.onRainfallStart();
      }
    }
    // Evaluate
    rain_.tips += tips;
    unsigned long duration = isr_.tsStop - isr_.tsStart;
    // Convert from milliseconds to seconds and round
    duration += 500;
    duration /= 1000;
    // Evaluate
    rain_.volume = float(rain_.tips * BUCKET_FACTOR);
    rain_.duration = duration;
    rain_.rate = 0;
    if (rain_.duration > 0)
    {
      float tipRate = float(rain_.tips - 1) / float(rain_.duration);
      rain_.rate = tipRate * BUCKET_FACTOR * 3600;
    }
    SERIAL_VALUE("buketTips", rain_.tips)
    SERIAL_VALUE("rainVolume", String(rain_.volume, 4))
    SERIAL_VALUE("rainDuration", rain_.duration)
    SERIAL_VALUE("rainRate", rain_.rate)
    if (handlers_.onEvaluate != nullptr)
    {
      handlers_.onEvaluate();
    }
  }
  /*
    Rain end detection.

    DESCRIPTION:
    The method detects a rainfall end by elapsed predefined minutes after recent
    bucket tip.

    PARAMETERS: None

    RETURN: none
  */
  void rainfallEnd()
  {
    word offset = (millis() - isr_.tsStop) / 60000;
    if (rain_.flPending && (offset >= rain_.offset))
    {
      SERIAL_VALUE("Rainfall", "STOP")
      rain_.flPending = false;
      if (handlers_.onRainfallStop != nullptr)
      {
        handlers_.onRainfallStop();
      }
      isr_.reset();
      rain_.reset();
    }
  }
};

#endif
