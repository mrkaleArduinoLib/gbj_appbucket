<a id="library"></a>

# gbj\_appbucket
This is an application library, which is used usually as a project library for particular PlatformIO project. It encapsulates the functionality of a `Rainfall Tipping Bucket`. The encapsulation provides following advantages:

* Functionality is hidden from the main sketch.
* The library follows the principle `separation of concerns`.
* The library is reusable for various projects without need to code the bucket management.
* Update in library is valid for all involved projects.
* It specifies (inherits from) the parent application library `gbj_appcore`.
* It utilizes funcionality and error handling from the parent class.


## Fundamental functionality
* The functionality is represented within this library by a class method called by an <abbr title='Interuption Service Routine'>ISR</abbr> usually in the main sketch at each tip of a GPIO pin of a microcontroller sensing a bucket's reed switch.
* The library processes (counts) bucket tips for detecting and evaluating a rainfall.
* A rainfall starts at the very first bucket tip after eventual previous rainfall, since recent boot of a microcontroller.
* A rainfall finishes at the end of the particular time period since recent bucket tip, which is considered as `Rainfall detection period`. This time period is defined in the project configuration file `platformio.ini`.
* Rain parameters are evaluated for a time period from the first bucket tip to the last one in a rainfall.


<a id="internals"></a>

## Internal parameters
Internal parameters are hard-coded in the library usually as enumerations and have neither setters nor getters associated. Some of them serve as default values.

* **Bucket factor** (`0.2794 mm/m2`): The measurement feature of the rainfall tipping bucket, which determines the extrapolation of rain volume measured in millimeters of the water amount dropped upon one square meter of ground.
* **Rainfall detection period** (`30 minutes`): It is a usual period determing the finish of a rainfall.
* **Time period for debouncing** (`500 milliseconds`): This parameter determines time period within which subsequent tips are ignored. It should overcome (ignore) the bucket's reed switch contacts bouncing.
The highest rain rate is expected 72 mm/m2/hr, which is 257.7 bucket tips per hour, and mean time between tips is cca. 14 seconds. It is the maximal reasonable debouncing period. The default debouncing period should be reached at rain rate of 2012 mm/m2/hr, i.e., 7200 tips per hour, which is unreal.


<a id="dependency"></a>

## Dependency
* **gbj\_appcore**: Parent library loaded from the file `gbj_appcore.h`.
* **gbj\_appstatistics**: Library for definition of various statistical measures in form of structures with methods loaded from the file `gbj_appstatistics.h`.
* **gbj\_serial\_debug**: Auxilliary library for debug serial output loaded from the file `gbj_serial_debug.h`. It enables to exclude serial outputs from final (production) compilation.

#### Arduino platform
* **Arduino.h**: Main include file for the Arduino SDK.
* **inttypes.h**: Integer type conversions. This header file includes the exact-width integer definitions and extends them with additional facilities provided by the implementation.

#### Espressif ESP8266 platform
* **Arduino.h**: Main include file for the Arduino platform.

#### Espressif ESP32 platform
* **Arduino.h**: Main include file for the Arduino platform.


<a id="interface"></a>

## Custom data types
* [Handler](#handler)
* [Handlers](#handlers)

## Interface
* [gbj_appbucket()](#gbj_appbucket)
* [isr()](#isr)
* [run()](#run)

### Getters
* [isRain()](#isRain)
* [getRainDuration()](#getRainDuration)
* [getRainVolume()](#getRainVolume)
* [getRainRate()](#getRainRate)
* [getRainStart()](#getRainTime)
* [getRainStop()](#getRainTime)
* [getTips()](#getTips)
* [getTipsGapMin](#getTipsGapStats)
* [getTipsGapMax](#getTipsGapStats)
* [getTipsGapAvg](#getTipsGapStats)


<a id="handler"></a>

## Handler

#### Description
The template or the signature of a callback function, which is called at particular event in the processing. It can be utilized for instant communicating with other modules of the application (project).
* A handler is just a function with no arguments and returning nothing.
* A handler can be declared just as `void` type in the main sketch.

#### Syntax
    typedef void Handler()

#### Parameters
None

#### Returns
None

#### See also
[Handlers](#handlers)

[Back to interface](#interface)


<a id="handlers"></a>

## Handlers

#### Description
Structure of pointers to handlers each for particular event in processing.
* Individual or all handlers do not need to be defined in the main sketch, just those that are useful.

#### Syntax
    struct Handlers
    {
      Handler *onRainfallStart;
      Handler *onRainfallStop;
      Handler *onEvaluate;
    }

#### Parameters

* **onRainfallStart**: Pointer to a callback function, which is called at detecting the start of a rainfall, i.e., at the first tip of a rain bucket.
* **onRainfallStop**: Pointer to a callback function, which is called at detecting the end of a rainfall, i.e., after the delay period since recent tip of a rain bucket. There are all parameters or recent rainfall available for the handler by corresponding getters, right before their reset.
* **onEvaluate**: Pointer to a callback function, which is called at the end of a rainfall evaluation when some bucket tips have been detecged. The rain evaluation is executed in the method [run()](#run), usually in the sketch loop. The handler is useful for dynamic show of a rain parameters, e.g., in a built-in web server.

#### Example
```cpp
void onRainfallStart()
{
  ...
}
void onRainfallStop()
{
  ...
}
void onRainfallEval()
{
  ...
}
gbj_appbucket::Handlers handlersBucket = {
    .onRainfallStart = onRainfallStart,
    .onRainfallStop = onRainfallStop,
    .onEvaluate = onRainfallEval,
};
gbj_appbucket bucket = gbj_appbucket(PERIOD_MIN_RAIN_END, handlersBucket);
```

#### See also
[Handler](#handler)

[gbj_appbucket](#gbj_appbucket)

[Back to interface](#interface)


<a id="gbj_appbucket"></a>

## gbj_appbucket()

#### Description
Constructor creates the class instance object and initiates internal resources and parameters.

#### Syntax
    gbj_appbucket(word rainfallOffset, Handlers handlers)

#### Parameters
* **rainfallOffset**: Time in minutes from recent bucket tip to determine end of a rainfall.
  * *Data type*: unsigned integer
  * *Default value*: none

* **handlers**: Pointer to a structure of callback functions. This structure as well as handlers should be defined in the main sketch.
  * *Data type*: Handlers
  * *Default value*: empty structure

#### Returns
Object performing rainfall tipping bucket processing.

#### See also
[Handler](#handler)

[Handlers](#handlers)

[Back to interface](#interface)


<a id="isr"></a>

## isr()

#### Description
The execution method that should be called by IST in the main project sketch.
* The method registers the very first and the recent bucket tip for rainfall processing.

#### Syntax
    void isr()

#### Parameters
None

#### Returns
None

#### Example
```cpp
gbj_appbucket bucket = gbj_appbucket(...);
IRAM_ATTR void isrRainBucket()
{
  bucket.isr();
}
void setup()
{
  pinMode(PIN_BUCKET, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_BUCKET), isrRainBucket, FALLING);
}
```

[Back to interface](#interface)


<a id="run"></a>

## run()

#### Description
The execution method, which should be called frequently, usually in the loop function of a sketch.
* The method detects a rainfall start, the rainfall end, and calculates rain parameters.

#### Syntax
	void run()

#### Parameters
None

#### Returns
None

[Back to interface](#interface)


<a id="isRain"></a>

## isRain()

#### Description
The method returns a flag about a pending rainfall.
* A rainfall starts at the very first bucket tip after previous rainfall has finnished.
* A rainfall finishes after [Rainfall detection period](#internals) since recent bucket tip.

#### Syntax
    bool isRain()

#### Parameters
None

#### Returns
Flag about current (pending) rainfall.

[Back to interface](#interface)


<a id="getRainDuration"></a>

## getRainDuration()

#### Description
The method returns duration of the current rainfall in seconds.
* The duration is calculated as a time period between the very first and very last bucket tip of a rainfall regardless the time, when a rainfall finish has been detected.
* After a rainfall finish the rain duration is reset to zero.

#### Syntax
    word getRainDuration()

#### Parameters
None

#### Returns
Duration of the pending rainfall in seconds. The maximal rain duration is 65,535 seconds, which is 18 hours and 12.25 minutes. For a rainfall longer than that period the duration overflows and is reset to zero.

[Back to interface](#interface)


<a id="getRainVolume"></a>

## getRainVolume()

#### Description
The method returns rain water volume in millimeters per square meter of the current rainfall.
* The volume is calculated from the very first bucket tip until the recent tip.
* After a rainfall finish the rain volume is reset to zero.

#### Syntax
    float getRainVolume()

#### Parameters
None

#### Returns
Rain volume of the pending rainfall in millimeters per square meter.

[Back to interface](#interface)


<a id="getRainRate"></a>

## getRainRate()

#### Description
The method returns rain intensity in millimeters per square meter for an hour of the current rainfall.
* The rain rate is calculated from rain volume and rain duration, which is extrapolated or recalculated to one hour.
* The rate is calculated from minimum of 2 bucket tips.
* With just one bucket tip at a rainfall start the rain rate is zero.

#### Syntax
    float getRainRate()

#### Parameters
None

#### Returns
Rain rate of the pending rainfall in millimeters per square meter for an hour. At the start of a rainfall it is 0.

#### See also
[getRainDuration()](#getRainDuration)

[getRainVolume()](#getRainVolume)

[Back to interface](#interface)


<a id="getRainTime"></a>

## getRainStart(), getRainStop()

#### Description
The methods return timestamp of the very first and very last bucket tip of a rainfall.
* The timestamps are expressed in milliseconds since <abbr title='Micro Controller Unit'>MCU</abbr> boot.

#### Syntax
    unsigned long getRainStart()
    unsigned long getRainStop()

#### Parameters
None

#### Returns
Rainfall start and end timestamps in milliseconds since MCU start. While a timestamp is 32-bit usigned integer, the maximum timestamp is `4 294 967 295 ms`, which is `49 days 17 hours 2 minutes 47 seconds`. If a rainfall is pending around that uptime of a MCU, i.e., starts before and end after that time point, the timestamp counter resets and rain duration might be incorrect.

#### See also
[getRainDuration()](#getRainDuration)

[getRainVolume()](#getRainVolume)

[Back to interface](#interface)


<a id="getTips"></a>

## getTips()

#### Description
The method returns number of bucket tips of the current rainfall.
* After a rainfall finish the number of bucket tips is reset to zero.

#### Syntax
    unsigned long getTips()

#### Parameters
None

#### Returns
Number of bucket tips of the pending rainfall.

#### See also
[getRainVolume()](#getRainVolume)

[Back to interface](#interface)


<a id="getTipsGapStats"></a>

## getTipsGapMin(), getTipsGapMax(), getTipsGapAvg()

#### Description
The method returns statistical values of gaps between bucket tips of the current rainfall in seconds.
* The bucket tip gaps statistics are minimum, maximum, and average.
* After a rainfall finish the statistics of tip gaps are reset to zero.

#### Syntax
    unsigned long getTipsGapMin()
    unsigned long getTipsGapMax()
    unsigned long getTipsGapAvg()

#### Parameters
None

#### Returns
Minimum, maximum, and average of the bucket tips gaps of the pending rainfall.

#### See also
[getTips()](#getTips)

[Back to interface](#interface)
