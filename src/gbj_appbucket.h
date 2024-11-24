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
#include "gbj_apphelpers.h"
#include "gbj_appstatistics.h"
#include "gbj_serial_debug.h"
#include "gbj_timer.h"

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

  struct Handlers
  {
    Handler *onRainfallStart;
    Handler *onRainfallStop;
    Handler *onRainfallRun;
  };
  /*
    Constructor

    DESCRIPTION:
    Constructor creates the class instance object and sets operational
    parameters.

    PARAMETERS:
    rainfallOffset - Time in minutes from single tip to determine end of a
    rainfall.
       - Data type: unsigned integer
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
    rain_.offsetMax = rainfallOffset * 60000;
    handlers_ = handlers;
    timer_ = new gbj_timer(Timing::PERIOD_END);
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
    if (millis() - statTime_.getTimeStop() < Timing::PERIOD_DEBOUNCE)
    {
      return;
    }
    rain_.flTips = true;
    statTime_.set(millis());
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
      timer_->reset();
    }
    if (timer_->run())
    {
      rainfallEnd();
    }
  }

  // Getters
  inline bool isRain() { return rain_.flPending; }
  inline word getRainDuration() { return rain_.duration; }
  inline float getRainVolume() { return rain_.volume; }
  inline float getRainRate() { return rain_.rate; }
  inline unsigned long getRainStart() { return statTime_.getTimeStart(); }
  inline unsigned long getRainStop() { return statTime_.getTimeStop(); }
  inline unsigned long getTips() { return statTime_.getCnt(); }
  inline unsigned long getTipsGapMin() { return statTime_.getMin(); }
  inline unsigned long getTipsGapMax() { return statTime_.getMax(); }
  inline unsigned long getTipsGapAvg() { return statTime_.getAvg(); }

private:
  enum Timing : word
  {
    // Debouncing delay in milliseconds
    PERIOD_DEBOUNCE = 500,
    // Period for detecting end of rainfall
    PERIOD_END = 5000,
  };
  enum Params : byte
  {
    // Multiplicator to determine an observation period of rainfall phase
    PARAM_ACTIVE_END_COEF = 5,
  };
  struct Rain
  {
    // Maximal delay from recent tip determining rainfall end
    unsigned long offsetMax;
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
    void reset()
    {
      duration = volume = rate = 0;
      flPending = flTips = false;
    }
  } rain_;
  // Rain millimeters per bucket tick
  const float BUCKET_FACTOR = 0.2794;
  Handlers handlers_;
  StatisticTime statTime_ = StatisticTime(lblStatsIntervalTime);
  gbj_timer *timer_;
  /*
    Rain evaluation.

    DESCRIPTION:
    The method evaluates the rain tips collected so far.

    PARAMETERS: None

    RETURN: none
  */
  void rainEvaluate()
  {
    // Ignore the very first tip
    if (statTime_.getCnt() < 2)
    {
      return;
    }
    // Register rainfall start
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
    rain_.volume = float(statTime_.getCnt() * BUCKET_FACTOR);
    rain_.duration = gbj_apphelpers::convertMs2Sec(statTime_.get());
    rain_.rate = 0;
    if (rain_.duration > 0)
    {
      rain_.rate = rain_.volume * 3600 / float(rain_.duration);
    }
    SERIAL_VALUE("rainVolume", String(getRainVolume(), 4))
    SERIAL_VALUE("rainDuration", getRainDuration())
    SERIAL_VALUE("rainRate", getRainRate())
    SERIAL_VALUE("tipCnt", getTips())
    SERIAL_VALUE("tipGapMin", getTipsGapMin())
    SERIAL_VALUE("tipGapMax", getTipsGapMax())
    SERIAL_VALUE("tipGapAvg", getTipsGapAvg())
    if (handlers_.onRainfallRun != nullptr)
    {
      handlers_.onRainfallRun();
    }
  }
  /*
    Rain end detection.

    DESCRIPTION:
    The method detects a rainfall end by elapsed predefined time since recent
    bucket tip.

    PARAMETERS: None

    RETURN: none
  */
  void rainfallEnd()
  {
    // Determine pending rainfall end
    if (rain_.flPending)
    {
      unsigned long offsetLimit =
        min(statTime_.getMax() * Params::PARAM_ACTIVE_END_COEF, rain_.offsetMax);
      if (millis() - statTime_.getTimeStop() >= offsetLimit)
      {
        SERIAL_VALUE("Rainfall", "STOP")
        SERIAL_VALUE("offsetLimit", offsetLimit)
        rain_.flPending = false;
        if (handlers_.onRainfallStop != nullptr)
        {
          handlers_.onRainfallStop();
        }
        statTime_.reset();
        rain_.reset();
      }
    }
    // No rainfall
    else
    {
      // Close rainfall at single tip and after expiry time
      if (statTime_.getCnt() == 1 &&
          millis() - statTime_.getTimeStop() > rain_.offsetMax)
      {
        statTime_.reset();
      }
      return;
    }
  }
};

#endif
