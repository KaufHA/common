# KAUF External Components

Components in this folder can be added to your ESPHome project using the following yaml:

```
external_components:
  - source:
      type: git
      url: https://github.com/KaufHA/common
    components: [ ddp, kauf_hlw8012 ]
    refresh: always
```

`components:` is a comma-separated list of components that you actually want to add.  You can remove the `components:` line to import all components, but that is not recommended.

The components which might be particularly interesting to you are described below.  All other components are probably not going to be worth trying to use outside of our specific yaml packages.

## DDP

The DDP component will allow your light to accept DDP packets from WLED, xLights, etc.  Currently, only RGB (3 channels per pixel) is supported.

### Installation and Usage

(1) Import the DDP component into your project as an external component using the following yaml:

```
external_components:
  - source:
      type: git
      url: https://github.com/KaufHA/common
    components: [ ddp ]
    refresh: always
```

(2) Invoke the ddp component by adding `ddp:` at the top level of your yaml file.  Top level means that `ddp:` is at the beginning of its line and not tabbed over at all.  This can be seen in the first line of both examples below.

(3) Add either ddp or addressable_ddp as an effect to any light entity.  The ddp effect is for single lights such as bulbs.  The addressable_ddp effect is for addressable lights such as RGB strips.

Either effect can optionally utilize the following configuration variables:
- **name** (*Optional*, string): The name of the effect.  Defaults to `DDP` for the ddp effect and `Addressable DDP` for the addressable_ddp effect.
- **timeout** (*Optional*, [Time](https://esphome.io/guides/configuration-types.html#config-time)): A duration of time after which, if no DDP packets are received, the light will automatically return to the color set via Home Assistant.  Defaults to `10s`.  Set to `0s` to disable timeout.
- **disable_gamma** (*Optional*, boolean): If true, gamma will be disabled for color values received via ddp.  If set to false, gamma will not be disabled.  Defaults to `true`.
- **brightness_scaling** (*Optional*): Control whether and how DDP values will be scaled by the Home Assistant light entity brightness.
  - `NONE` (default) - DDP values will be displayed as received without being scaled based on the Home Assistant entity brightness.
  - `MULTIPLY` - All received DDP values will be multiplied by the Home Assistant entity brightness before being displayed, where the entity brightness is considered a float value between 0.0 and 1.0.
  - `PIXEL` - Each pixel will individually be scaled up or down to the brightness of the Home Assistant light entity.
  - `STRIP` - Each strip will be scaled up or down so that the brightest pixel of the strip is at the brightness of the Home Assistant light entity.  `PIXEL` and `STRIP` are the same for bulbs.
  - `PACKET` - Each entire packet will be scaled so that the brightest pixel of the packet is at the brightness of the Home Assistant light entity.  `PACKET` and `STRIP` are the same for devices with one LED strip.  

DDP example:

```
ddp:

light:
  - platform: rgb
    name: "Test DDP Bulb"
    id: kauf_light
    red: pwm_red
    green: pwm_green
    blue: pwm_blue
    effects:
      - ddp:
          name: DDP
          timeout: 10s
          disable_gamma: true
          brightness_scaling: none
```

Addressable DDP example:

```
ddp:

light:
  - platform: neopixelbus
    type: GRB
    variant: WS2811
    pin: D1
    num_leds: 10
    name: "Test DDP Strip"
    effects:
      - addressable_ddp:
          name: Addressable DDP
          timeout: 10s
          disable_gamma: true
          brightness_scaling: none
```

(4)  Activate the effect.  The effects can be enabled by turning on the light in Home Assistant and then selecting the effect from the light entity's information popup as below:

![image](https://user-images.githubusercontent.com/89616381/206888603-fbd7d5e8-6ccd-4c30-bac6-235cc163dc8c.png)

This component is a work in progress with only basic functionality completed so far.  Still to implement:
- Chain or Tree excess DDP pixels, i.e., forward unused pixel data to subsequent IP addresses.

### DDP Troubleshooting

For now, the DDP component requires a data offset of zero, since someone had issues with a bulb receiving packets not intended for the bulb.  The only convenient way to filter these out was to filter any packet with a non-zero data offset.  You can get xLights to always send out a zero offset by unchecking "Keep Channel Numbers" for each device.

If anyone can explain the specific definition of the data offset field and how it should be used, that would be appreciated.  The spec only really explains its use either with loading data from local storage or with DMX legacy mode.  It makes no sense how xLights can send the same packet with different data offsets and have both be valid under the spec.

[DDP Spec](http://www.3waylabs.com/ddp/)

### Additional Notes on Header Fields

Currently, neither xLights nor WLED appear to follow the DDP header specification.  In particular, when sending RGB data, the data type field is not set properly.  Therefore, this component does not look at the data type field to determine number of channels and instead always just presumes RGB data.  There is no plan to modify this behavior until someone lets us know that it is needed.

The timecode field is not handled, and all received packets are presumed to not have a timecode field without checking.  Neither WLED nor xLights utilize the timecode field and since they don't follow the header spec in other ways we don't want to depend on the timecode flag being accurate.  Therefore, if data packets are sent with timecodes, the RGB data will be misinterpreted.  If this behavior becomes a problem for someone, let us know.


## KAUF_HLW8012

The kauf_hlw8012 component is a rewritten alternative to the stock ESPHome hlw8012 component.  The kauf_hlw8012 component constantly switches back-and-forth between reading current and voltage as quickly as possible, and then publishes the most recent reading of both every update interval.

The kauf_hlw8012 does not use a change_mode_every or initial_mode setting like the stock hlw8012 component, since our version always just switches back and forth as fast as possible.

Add the kauf_hlw8012 component to your project as follows:

```
external_components:
  - source:
      type: git
      url: https://github.com/KaufHA/common
    components: [ kauf_hlw8012 ]
    refresh: always
```

The configuration for the kauf_hlw8012 component is the same as the stock hlw8012 component, except that the change_mode_every and initial_mode settings are invalid.  There is also no energy meter any more.  Please use the [total_daily_energy](https://esphome.io/components/sensor/total_daily_energy.html) component if you want to track energy usage with the kauf_hlw8012 component.

For example, the following would be a valid configuration to update the power, current, and voltage sensors every 60s:

```
sensor:
  - platform: kauf_hlw8012
    sel_pin: 5
    cf_pin: 14
    cf1_pin: 13
    current:
      name: "HLW8012 Current"
    voltage:
      name: "HLW8012 Voltage"
    power:
      name: "HLW8012 Power"
    update_interval: 60s
```

Still to do:
- Settings to update sensors before the update interval period is complete if values change by a fixed value or percentage.
- Settings to average multiple readings for each sensor publication.
