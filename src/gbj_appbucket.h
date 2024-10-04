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
#include "config_params_stats.h"
#include "gbj_appcore.h"
#include "gbj_appstatistics.h"
#include "gbj_serial_debug.h"

#undef SERIAL_PREFIX
#define SERIAL_PREFIX "gbj_appbucket"
//******************************************************************************
// Class definition
//******************************************************************************
class gbj_appbucket
  : gbj_appcore
  , gbj_appstatistics
{
public:
  typedef void Handler();
  StatisticTime statTime = StatisticTime(lblStatsIntervalTime);

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
    if (millis() - statTime.getStop() < Timing::PERIOD_DEBOUNCE)
    {
      return;
    }
    rain_.flTips = true;
    statTime.set(millis());
    SERIAL_TITLE("ISR")
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
    if (rain_.flTips)
    {
      rainEvaluate();
      rain_.flTips = false;
    }
    rainfallEnd();
  }

  // Getters
  inline bool isRain() { return rain_.flPending; }
  inline word getRainDuration() { return rain_.duration; }
  inline float getRainVolume() { return rain_.volume; }
  inline float getRainRate() { return rain_.rate; }
  inline unsigned long getRainStart() { return statTime.getStart(); }
  inline unsigned long getRainStop() { return statTime.getStop(); }
  inline unsigned long getTips() { return statTime.getCnt(); }
  inline unsigned long getTipsGapMin() { return statTime.getMin(); }
  inline unsigned long getTipsGapMax() { return statTime.getMax(); }
  inline unsigned long getTipsGapAvg() { return statTime.getAvg(); }

private:
  enum Timing : word
  {
    // Debouncing delay in milliseconds
    PERIOD_DEBOUNCE = 1000,
  };
  struct Rain
  {
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
    // Flag about new tips
    bool flTips;
    void reset() { duration = volume = rate = 0; }
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
    // Determine rain
    if (!rain_.flPending)
    {
      SERIAL_VALUE("Rainfall", "START")
      rain_.flPending = true;
      if (handlers_.onRainfallStart != nullptr)
      {
        handlers_.onRainfallStart();
      }
    }
    // Evaluate
    unsigned long duration = statTime.get();
    // Convert from milliseconds to seconds and round
    duration += 500;
    duration /= 1000;
    // Evaluate
    rain_.volume = float(statTime.getCnt() * BUCKET_FACTOR);
    rain_.duration = duration;
    rain_.rate = 0;
    if (rain_.duration > 0)
    {
      float tipRate = float(statTime.getCnt() - 1) / float(rain_.duration);
      rain_.rate = tipRate * BUCKET_FACTOR * 3600;
    }
    SERIAL_VALUE("rainVolume", String(getRainVolume(), 4))
    SERIAL_VALUE("rainDuration", getRainDuration())
    SERIAL_VALUE("rainRate", getRainRate())
    SERIAL_VALUE("tipCnt", getTips())
    SERIAL_VALUE("tipGabMin", getTipsGapMin())
    SERIAL_VALUE("tipGabMax", getTipsGapMax())
    SERIAL_VALUE("tipGabAvg", getTipsGapAvg())
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
    word offset = (millis() - statTime.getStop()) / 60000;
    if (rain_.flPending && (offset >= rain_.offset))
    {
      SERIAL_VALUE("Rainfall", "STOP")
      rain_.flPending = false;
      if (handlers_.onRainfallStop != nullptr)
      {
        handlers_.onRainfallStop();
      }
      statTime.reset();
      rain_.reset();
    }
  }
};

#endif
