<a id="library"></a>

# gbj\_appbucket
This is an application library, which is used usually as a project library for particular PlatformIO project. However; in every project utilizing the rainfall tipping bucket should be copied the same library, so that it is located in central library storage.

- Library specifies (inherits from) the application `gbj_appcore` library.
- Library utilizes error handling from the parent class.
- Library processes rainfall tipping bucket impulses for detecting and evaluating a rainfall.
- A rainfall finish is considered at the end of the particular time period since recent bucket tip, which is usually 15 or 20 minutes.
- A rainfall starts at the very first bucket tip after eventual previous rainfall, since recent boot of a microcontroller.
- Rain parameters are evaluated for a time period from the first bucket tip to the last one in a rainfall.


<a id="intensity"></a>

## Rain intensity levels
The rain intensity level is determined by ranking the rain rate in millimeters pre square meter for an hour and for each hour of rain duration.
- The library distinguishes 8 rain levels entirely.
    - 6 rain intensity levels are dedicated for 3 ranks of rain duration during a rainfall, namely for 1st hour, 2nd hour, and 3rd hours or more.
    - 1 rain intensity level is dedicated for a rainfall start with just one bucket tip, which represents unknown rain intensity level.
    - 1 rain level is dedicated for no rain situation, i.e., between rainfalls.
- In the following table the numerical values are thresholds for determining particular level in millimeters pre squar meter for an hour. The threshold is the lowest (staring) value for a level as depicted in the table just for lowest rain level.

|Level|1st hr.|2nd hr.|3rd+ hr.|Rain|
|:---:|:---:|:---:|:---:|:---:|
|0|=0|=0|=0|None|
|1|>0|>0|>0|Light|
|2|1|1.5|2|Shower|
|3|5|7.5|9|Moderate|
|4|10|14|11.5|Strong|
|5|15|21|23.5|Heavy|
|6|23|30.5|33|Intense|
|7|58|64|72|Torrential|
|8|n.a.|n.a.|n.a.|Unknown|


<a id="dependency"></a>

## Dependency

- **gbj\_appcore**: Parent library loaded from the file `gbj_appbase.h`.
- **gbj\_serial\_debug**: Auxilliary library for debug serial output loaded from the file `gbj_serial_debug.h`. It enables to exclude serial outputs from final compilation.

#### Espressif ESP8266 platform
- **Arduino.h**: Main include file for the Arduino platform.

#### Espressif ESP32 platform
- **Arduino.h**: Main include file for the Arduino platform.

#### Particle platform
- **Particle.h**: Includes alternative (C++) data type definitions.


<a id="constants"></a>

## Constants

- **gbj\_appbucket::VERSION**: Name and semantic version of the library.

Other constants and enumerations are inherited from the parent library.


<a id="interface"></a>

## Interface

- [gbj_appbucket()](#gbj_appbucket)
- [run()](#run)
- [isr()](#isr)


### Setters

- [setDelay()](#setDelay)
- [setRainfalls()](#setRainfalls)


### Getter

- [isRain()](#isRain)
- [getDelay()](#getDelay)
- [getDuration()](#getDuration)
- [getOffset()](#getOffset)
- [getVolume()](#getVolume)
- [getTips()](#getTips)
- [getRate()](#getRate)
- [getRateTips()](#getRateTips)
- [getIntens()](#getIntens)
- [getRainfalls()](#getRainfalls)


<a id="gbj_appbucket"></a>

## gbj_appbucket()

#### Description
Constructor creates the class instance object and initiates internal resources.

#### Syntax
    gbj_appbucket()

#### Parameters
None

#### Returns
Object performing rainfall tipping bucket processing.

[Back to interface](#interface)


<a id="run"></a>

## run()

#### Description
The execution method as the implementation of the virtual method from parent class, which should be called frequently, usually in the loop function of a sketch.
- The method detects a rainfall and calculates rain parameters.

[Back to interface](#interface)


<a id="isr"></a>

## isr()

#### Description
The execution method that should be called within _Interruption Service Routing_ in the main project sketch.
- The method registers the very first and the recent bucket tip for rainfall processing.

#### Syntax
    void isr()

#### Parameters
None

#### Returns
None

#### Example
```cpp
gbj_appbucket bucket = gbj_appbucket();
IRAM_ATTR void isrBucket()
{
  bucket.isr();
}
void setup()
{
  pinMode(PIN_BUCKET, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_BUCKET), isrBucket, FALLING);
}
```

[Back to interface](#interface)


<a id="setDelay"></a>

## setDelay()

#### Description
The method sets the _Rainfall detection period_ in minutes for determing the finish of a rainfall.
- The rainfall finishes after that period since recent bucket tip.
- The rain delay period is usually stored in EEPROM of a microcontroller in order to retain it after power cycles.
- The method is useful for calling it from various IoT or cloud platforms for controlling rainfall detection.

#### Syntax
    void setDelay(byte delay)

#### Parameters
- **delay**: _Rainfall detection period_ in minutes.
  - *Valid values*: 0 ~ 255.
  - *Default value*: 20

#### Returns
None

#### See also
[getDelay()](#getDelay)

[isRain()](#isRain)

[Back to interface](#interface)


<a id="getDelay"></a>

## getDelay()

#### Description
The method returns recently set _Rainfall detection period_.

#### Syntax
    byte getDelay()

#### Parameters
None

#### Returns
Time period since recent bucket tip for detecting a rainfall finish.

#### See also
[setDelay()](#setDelay)

[isRain()](#isRain)

[Back to interface](#interface)


<a id="isRain"></a>

## isRain()

#### Description
The method returns a flag about a pending rainfall.
- A rainfall starts at the very first bucket tip after previous rainfall has finnished.
- A rainfall finishes after _Rainfall detection period_ since recent bucket tip.

#### Syntax
    bool isRain()

#### Parameters
None

#### Returns
Flag about current (pending) rainfall.

#### See also
[setDelay()](#setDelay)

[Back to interface](#interface)


<a id="getDuration"></a>

## getDuration()

#### Description
The method returns duration in seconds of the current rainfall.
- The duration is calculated as a time period between the very first and very last bucket tip of a rainfall regardless the time, when a rainfall finish has been detected.
- During the pending rainfall its duration is suitable for publishing to the IoT platform as telemetry.
- After a rainfall finish the rain duration is reset to zero.

#### Syntax
    unsigned int getDuration()

#### Parameters
None

#### Returns
Duration of the pending rainfall in seconds. The maximal rain duration is 65,535 seconds, which is 18 hours and 12.25 minutes. For a rainfall longer than that period the duration overflows and is reset to zero.

#### See also
[getOffset()](#getOffset)

[Back to interface](#interface)


<a id="getOffset"></a>

## getOffset()

#### Description
The method returns time period in seconds since the recent bucket tip regardless the pending rainfall or none one.
- If the offset is greater then Rainfall detection period, a pending rainfall is considered as finished.
- The offset is suitable for publishing to the IoT platform as telemetry.
- During the pending rainfall observing the offset is usefull for expecting the rainfall finish detection.
- In time without pending rainfall the offset is the time since recent bucket tip. In this case the offset determines real time since recent rain.
- The offset is reset to zero just at the very first bucket tip of a new rainfall.
- Despite the offset is 32-bit value, in fact it is calculated from internal timestamp in milliseconds. So that the maximal real rain offset can be only (2^32 - 1) / 1000 seconds.

#### Syntax
    unsigned long getOffset()

#### Parameters
None

#### Returns
Time in seconds since recent bucket tips. The maximal rain offset is 4,294,967.295 seconds, which is 49 days, 17 hours, and 2.79 minutes. After that period without rain the offset overflows and is reset to zero.

#### See also
[setDelay()](#setDelay)

[Back to interface](#interface)


<a id="getVolume"></a>

## getVolume()

#### Description
The method returns rain water volume in millimeters per square meter of the current rainfall.
- The volume is calculated from the very first bucket tip until the recent tip.
- During the pending rainfall its volume is suitable for publishing to the IoT platform as telemetry.
- After a rainfall finish the rain volume is reset to zero.

#### Syntax
    float getVolume()

#### Parameters
None

#### Returns
Rain volume of the pending rainfall in millimeters per square meter.

#### See also
[getTips()](#getTips)

[Back to interface](#interface)


<a id="getTips"></a>

## getTips()

#### Description
The method returns number of bucket tips of the current rainfall.
- After a rainfall finish the number of bucket tips is reset to zero.

#### Syntax
    unsigned int getTips()

#### Parameters
None

#### Returns
Number of bucket tips of the pending rainfall.

#### See also
[getVolume()](#getVolume)

[Back to interface](#interface)


<a id="getRate"></a>

## getRate()

#### Description
The method returns rain intensity in millimeters per square meter for an hour of the current rainfall.
- The rain rate is calculated from rain volume and rain duration, which is extrapolated or recalculated to one hour.
- The rate is calculated from minimum of 2 bucket tips.
- With just one bucket tip at a rainfall start the rain rate is designated with -1, which expresses unknown rate.

#### Syntax
    float getRate()

#### Parameters
None

#### Returns
Rain rate of the pending rainfall in millimeters per square meter for an hour. At the start of a rainfall it is -1 representing an unknown rate.

#### See also
[getRateTips()](#getRateTips)

[getVolume()](#getVolume)

[getDuration()](#getDuration)

[Back to interface](#interface)


<a id="getRateTips"></a>

## getRateTips()

#### Description
The method returns tipping intensity in bucket tips for an hour of the current rainfall.
- The rain tipping rate is calculated from number of rain tips and rain duration, which is extrapolated or recalculated to one hour.
- The tipping rate is calculated from minimum of 2 bucket tips.
- With just one bucket tip at a rainfall start the rain tipping rate is designated with -1, which expresses unknown rate.

#### Syntax
    float getRateTips()

#### Parameters
None

#### Returns
Rain tiping rate of the pending rainfall in bucket tips for an hour. At the start of a rainfall it is -1 representing an unknown tiping rate.

#### See also
[getRate()](#getRate)

[getTips()](#getTips)

[getDuration()](#getDuration)

[Back to interface](#interface)


<a id="setRainfalls"></a>

## setRainfalls()

#### Description
The method sets the initial value of an internal rainfalls counter to desired count, but usually it is used for resetting that counter by calling it without parameters.
- The rainfalls count is usually stored in EEPROM of a microcontroller in order to retain it after power cycles.
- The method is useful for calling it from various IoT or cloud platforms for initiating the rainfalls counter.

#### Syntax
    void setRainfalls(unsigned int rainfalls)

#### Parameters
- **rainfalls**: Number of rainfalls.
  - *Valid values*: 0 ~ 65,535.
  - *Default value*: 0

#### Returns
None

#### See also
[getRainfalls()](#getRainfalls)

[Back to interface](#interface)


<a id="getRainfalls"></a>

## getRainfalls()

#### Description
The method returns current number of rainfalls since resetting the rainfalls counter.

#### Syntax
    unsigned int getRainfalls()

#### Parameters
None

#### Returns
Number of rainfalls.

#### See also
[setRainfalls()](#setRainfalls)

[Back to interface](#interface)


<a id="getIntens"></a>

## getIntens()

#### Description
The method calculates the [level of rain intensity](#intensity) for rain rate and determines it for various rain duration.
- During a rainfall the level ranked from the rain rate depending on rain duration.
- Between rainfalls the method return the level for no rain status.
- Rainfalls intensity levels can be tranformed to descriptive names or rain type names for visualization purposes.

#### Syntax
    byte getIntens()

#### Parameters
None

#### Returns
Rain intensity level in the range 0 ~ 8.

#### See also
[getRate()](#getRate)

[getDuration()](#getDuration)

[Back to interface](#interface)
