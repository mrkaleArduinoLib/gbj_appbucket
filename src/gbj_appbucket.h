/**
 * @name gbj_appbucket
 *
 * @brief Application library for processing tips of Rainfall Tipping Bucket.
 *
 * @note The library processes tips caught with external interrupts.
 *
 * @copyright This program is free software; you can redistribute it and/or
 * modify it under the terms of the license GNU GPL v3
 * http://www.gnu.org/licenses/gpl-3.0.html (related to original code) and MIT
 * License (MIT) for added code.
 *
 * @author Libor Gabaj
 *
 * @see https://github.com/ayushsharma82/EasyDDNS
 */
#ifndef GBJ_APPBUCKET_H
#define GBJ_APPBUCKET_H

#if defined(__AVR__)
  #include <inttypes.h>
#endif
#include "config_params_stats.h"
#include "gbj_appcore.h"
#include "gbj_apphelpers.h"
#include "gbj_appstatistics.h"
#include "gbj_serial_debug.h"
#include "gbj_timer.h"
#include <Arduino.h>

#undef SERIAL_PREFIX
#define SERIAL_PREFIX "gbj_appbucket"

// Class definition for updating DDNS service
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

  /**
   * @brief Constructor
   *
   * @param rainfallOffset Time in minutes from last tip to determine end of a
   * rainfall.
   * @param handlers A structure with pointers to various callback handler
   * functions.
   */
  inline gbj_appbucket(byte rainfallOffset, Handlers handlers = Handlers())
  {
    rain_.offsetMax = rainfallOffset * 60;
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
  /**
   * @brief Interruption Service Routing.
   *
   * @note The method collects random tips from a rain tip bucket.
   * @warning The method should be called in a main sketch ISR attached to the
   * bucket pin.
   */
  inline void isr()
  {
    // Debouncing
    if (millis() - rain_.timeLastTip < Timing::PERIOD_DEBOUNCE)
    {
      return;
    }
    rain_.timeLastTip = millis();

    // Register bucket tip
    rain_.flTips = true;
    statTime_.set(rain_.timeBoot +
                  gbj_apphelpers::convertMs2Sec(rain_.timeLastTip));
    SERIAL_TITLE("ISR")
  }

  /**
   * @brief Processing
   * @note The method should be called frequently either in an application
   * sketch loop or in a timer handler.
   * @note Until NTP boot the method does not evaluate rainfall, just collects
   * tips.
   */
  inline void run()
  {
    // Ignore rainfall evaluation before NTP boot
    if (rain_.timeBoot == 0)
    {
      return;
    }

    // Evaluate rainfall
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

  // Set epoch time of MCU boot
  inline void setTimeBoot(unsigned long timeBoot)
  {
    // Set epoch time of MCU boot only once
    if (rain_.timeBoot == 0 && timeBoot > 0)
    {
      rain_.timeBoot = timeBoot;

      // Some bucket tips were collected before the NTP boot
      if (statTime_.timeStart > 0)
      {
        statTime_.timeStart += rain_.timeBoot;
        statTime_.timeStop += rain_.timeBoot;
      }
    }
  }

  // Return flag about pending rainfall
  inline bool isRain() { return rain_.flPending; }

  // Set flag about pending rainfall
  inline void setRain(bool flRain) { rain_.flPending = flRain; }

  // Return rainfall duration in seconds
  inline word getRainDuration() { return rain_.duration; }

  // Return rainfall volume in millimeters
  inline float getRainVolume() { return rain_.volume; }

  // Return rainfall rate in millimeters per hour
  inline float getRainRate() { return rain_.rate; }

  // Return rainfall beginning in epoch time in seconds
  inline unsigned long getRainStart() { return statTime_.getTimeStart(); }

  // Return rainfall finishing in epoch time in seconds
  inline unsigned long getRainStop() { return statTime_.getTimeStop(); }

  // Return number of bucket tips
  inline unsigned long getTips() { return statTime_.getCnt(); }

  // Return minimal gap between tips in seconds
  inline unsigned long getTipsGapMin() { return statTime_.getMin(); }

  // Return maximal gap between tips in seconds
  inline unsigned long getTipsGapMax() { return statTime_.getMax(); }

  // Return average gap between tips in seconds
  inline unsigned long getTipsGapAvg() { return statTime_.getAvg(); }

  // Return serialized JSON representation of statistical time data
  inline String getJsonStatisticTime()
  {
    return gbj_appstatistics::exportStatisticTime(statTime_);
  }

  // Update statistical time data from serialized JSON representation
  inline bool importStatisticTime(String jsonStr)
  {
    return gbj_appstatistics::importStatisticTime(statTime_, jsonStr);
  }

private:
  // Timing constants
  enum Timing : unsigned long
  {
    // Debouncing delay in milliseconds
    PERIOD_DEBOUNCE = 1200,

    // Period in milliseconds for detecting end of rainfall
    PERIOD_END = 5000,
  };

  // Parameters
  enum Params : byte
  {
    // Multiplicator to determine an observation period of rainfall phase
    PARAM_ACTIVE_END_COEF = 5,
  };

  // Rainfall data
  struct Rain
  {
    // Epoch time of the MCU boot in seconds
    unsigned long timeBoot;

    // Timestamp of the last tip in milliseconds
    unsigned long timeLastTip;

    // Maximal delay from recent tip determining rainfall end in seconds
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

    // Reset all values
    void reset()
    {
      duration = volume = rate = 0;
      flPending = flTips = false;
    }
  } rain_;

  // Rain millimeters per bucket tick
  const float BUCKET_FACTOR = 0.2794;

  // Callback handlers
  Handlers handlers_;

  // Statistical time data object
  StatisticTime statTime_ = StatisticTime();

  // Internal timer actuator
  gbj_timer *timer_;

  /**
   * @brief Rain evaluation
   * @note  The method evaluates the rain tips collected so far.
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

    // Evaluate rainfall
    rain_.volume = float(statTime_.getCnt() * BUCKET_FACTOR);
    rain_.duration = statTime_.get();
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

    // Process pending rainfall
    if (handlers_.onRainfallRun != nullptr)
    {
      handlers_.onRainfallRun();
    }
  }

  /**
   * @brief Rain end detection
   * @note  The method detects a rainfall end by elapsed predefined time since
   * recent bucket tip.
   */
  void rainfallEnd()
  {
    // Wait for NTP boot
    if (rain_.timeBoot == 0)
    {
      return;
    }

    unsigned long epochTime =
      rain_.timeBoot + gbj_apphelpers::convertMs2Sec(millis());

    // Determine pending rainfall end
    if (rain_.flPending)
    {
      unsigned long offsetLimit = min(
        statTime_.getMax() * Params::PARAM_ACTIVE_END_COEF, rain_.offsetMax);
      if (offsetLimit == 0)
      {
        offsetLimit = rain_.offsetMax;
      }
      if (epochTime - statTime_.getTimeStop() >= offsetLimit)
      {
        SERIAL_VALUE("Rainfall", "STOP")
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
          epochTime - statTime_.getTimeStop() > rain_.offsetMax)
      {
        statTime_.reset();
      }
    }
  }
};

#endif
