<a id="library"></a>

# gbj\_appbucket
This is an application library, which is used usually as a project library for particular PlatformIO project. However; in every project utilizing the rainfall tipping bucket should be copied the same library, so that it is located in central library storage.

- Library specifies (inherits from) the application `gbj_appbase` library.
- Library utilizes error handling from the parent class.
- Library processes rainfall tipping bucket impulses for detecting and evaluating a rainfall.
- Library detects and determines a rainfall based on _Rainfall Detection Period_ (hereinafter referred to as "RDP") between bucket tips. As a rainfall is considered an event when at least 3 bucket tips have happend withing the RDP. It eliminates rainfall detection due to random bucket tips, e.g., from fog condensation. The rainfall end is detected after RDP since recent bucket tip.


<a id="dependency"></a>

## Dependency

- **gbj\_appbase**: Parent library loaded from the file `gbj_appbase.h`.
- **gbj\_timer**: Library for executing internal timer within an instance object loaded from the file `gbj_timer.h`.
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
- [setPeriod()](#period)
- [getPeriod()](#period)
- [setDelay()](#setDelay)
- [getDelay()](#getDelay)
- [getDuration()](#getDuration)
- [getVolume()](#getVolume)
- [getSpeed()](#getSpeed)
- [isRain()](#isRain)


<a id="gbj_appbucket"></a>
## gbj_appbucket()

#### Description
Constructor creates the class instance object and initiates internal resources.
- It creates one internal timer without a timer handler.

#### Syntax
    gbj_appbucket()

#### Parameters
None

#### Returns
Object performing connection and reconnection to the wifi network.

[Back to interface](#interface)


<a id="run"></a>

## run()

#### Description
The execution method as the implementation of the virtual method from parent class, which should be called frequently, usually in the loop function of a sketch.
- The method detects a rainfall and calculates its parameters.

#### See also
[isRain](#isRain)

[getDuration()](#getDuration)

[getVolume()](#getVolume)

[getSpeed()](#getSpeed)

[Back to interface](#interface)


<a id="isr"></a>

## isr()

#### Description
The execution method that should be called in _Interruption Service Routing_ in the main project sketch.
- The method registers the very first and the very last bucket tip for rainfall duration calculation.

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

#### See also
[isRain](#isRain)

[getDuration()](#getDuration)

[Back to interface](#interface)


<a id="period"></a>

## setPeriod(), getPeriod()

#### Description
The methods are just straitforward implementation of the virual methods from the parent class.

[Back to interface](#interface)


<a id="setDelay"></a>

## setDelay()

#### Description
The method sets the _RDP_ in minutes for determing begining and end of a rainfall.
- The rainfall begining, i.e, a new rainfall is detected when at least 3 bucket tips have happend within that period.
- The rainfall end is detected after that period from the very recent bucket tip.

#### Syntax
    void setDelay(byte delay)

#### Parameters
- **delay**: RDP in minutes.
  - *Valid values*: 0 ~ 255.
  - *Default value*: 5

#### Returns
None

#### See also
[getDelay()](#getDelay)

[isRain()](#isRain)

[Back to interface](#interface)


<a id="getDelay"></a>

## getDelay()

#### Description
The method returns recently set _RDP_.

#### Syntax
    byte getDelay()

#### Parameters
None

#### Returns
Delay between subsequent rainfalls and period for detecting a rainfall.

#### See also
[setDelay()](#setDelay)

[Back to interface](#interface)


<a id="getDuration"></a>

## getDuration()

#### Description
The method returns duration in seconds of the current or recent rainfall.
- The duration is calculated from the very first bucket tip until the recent tip regardless the rainfall is detected after 3 tips within RDP and its end is detected after that period from the recent tip.
- During the pending rainfall its duration is suitable for publishing to the IoT platform as telemetry.

#### Syntax
    unsigned int getDuration()

#### Parameters
None

#### Returns
Duration of the pending or recent rainfall in seconds. The maximal rain duration without interruption for at least RDP, i.e., at breaking into separate rainfalls, is 65535 seconds, which is 18 hours and 12.25 minutes.

[Back to interface](#interface)


<a id="getVolume"></a>

## getVolume()

#### Description
The method returns rain water volume in millimeters per square meter of the current or recent rainfall.
- The volume is calculated from the very first bucket tip until the recent tip.
- During the pending rainfall its volumne is suitable for publishing to the IoT platform as telemetry.

#### Syntax
    float getVolume()

#### Parameters
None

#### Returns
Rain volume of the pending or recent rainfall in millimeters per square meter.

[Back to interface](#interface)


<a id="getSpeed"></a>

## getSpeed()

#### Description
The method returns rain intensity in millimeters per square meter and hour of the current or recent rainfall.
- The speed is calculated from rainfall volume and rainfall duration.
- During the pending rainfall its speed is suitable for publishing to the IoT platform as telemetry about rain intensity.

#### Syntax
    float getSpeed()

#### Parameters
None

#### Returns
Rain intensity of the pending or recent rainfall in millimeters per square meter and hour.

#### See also
[getVolume()](#getVolume)

[getDuration()](#getDuration)

[Back to interface](#interface)


<a id="isRain"></a>

## isRain()

#### Description
The method returns a flag whether a rainfall has been detected.
- As a rainfall is considered an event when at least 3 bucket tips have happend withing the RDP. It eliminates random bucket tips, e.g., from fog condensation.
- In case of detecting rainfall, its duration is registered from the time of the first bucket tip, not from the third tip.
- The rainfall end is detected after RDP since recent bucket tip. Rainfall duration is registered until the very last bucket tip, not until the end of RDP.

#### Syntax
    bool isRain()

#### Parameters
None

#### Returns
Flag about current (pending) rainfall.

#### See also
[setDelay()](#setDelay)

[Back to interface](#interface)
