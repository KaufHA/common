# KAUF External Components

Components in this folder can be added to your ESPHome project using the following yaml:

```
external_components:
  - source:
      type: git
      url: https://github.com/KaufHA/common
    components: [ ddp, kauf_hlw8012 ]
```

`components:` is a comma-separated list of components that you actually want to add.  You can remove the `components:` line to import all components, but that is not recommended.

The components which might be particularly interesting to you are described below.  All other components are probably not going to be worth trying to use outside of our specific yaml packages.

## DDP

The DDP component will allow your light to accept DDP packets from WLED, xLights, etc.  Currently, only RGB (3 channels per pixel) is supported.

You need to add the ddp component into your project using `ddp:` at the top level of your yaml file.  This is shown in the below examples.  Then, the ddp and addressable_ddp effects will be available to add in lights.

Import the DDP project into your project using the following yaml:

```
external_components:
  - source:
      type: git
      url: https://github.com/KaufHA/common
    components: [ ddp ]
```

DDP can be added to a single light using the ddp effect as follows.

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
      - ddp
```

DDP can be added to an addressable light using the addressable_ddp effect as follows.

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
      - addressable_ddp
```

In either case, the effect will need to be activated before the light responds to DDP packets.  The effects can be enabled by turning on the light in Home Assistant and then selecting the effect from the light entity's information popup as below:

![image](https://user-images.githubusercontent.com/89616381/206888603-fbd7d5e8-6ccd-4c30-bac6-235cc163dc8c.png)

This component is a work in progress with only basic functionality completed so far.  Still to implement:
- Customizable timeout after which the light will return to the value set in Home Assistant if no DDP packet is received.
- Read DDP header to determine whether to use data as RGB, RGBW, or grayscale.
- Chain or Tree excess DDP pixels, i.e., forward unused pixel data to subsequent IP addresses.
- Option to scale brightness to the light entity's brightness in Home Assistant
- Handle timecode field, which changes the offset where pixel data begins in the UDP packet.

### DDP Troubleshooting

For now, the DDP component requires a data offset of zero, since someone had issues with a bulb receiving packets not intended for the bulb.  The only convenient way to filter these out was to filter any packet with a non-zero data offset.  You can get xLights to always send out a zero offset by unchecking "Keep Channel Numbers" for each device.

If anyone can explain the specific definition of the data offset field and how it should be used, that would be appreciated.  The spec only really explains its use either with loading data from local storage or with DMX legacy mode.  It makes no sense how xLights can send the same packet with different data offsets and have both be valid under the spec.

[DDP Spec](http://www.3waylabs.com/ddp/)

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
```

The configuration for the kauf_hlw8012 component is the same as the stock hlw8012 component, except that the change_mode_every and initial_mode settings are invalid.  There is also no energy meter any more.  Please use the [total_daily_energy](https://esphome.io/components/sensor/total_daily_energy.html) component if you want to track energy usage with the kauf_hlw8012 component.

For example, the following would be a valid configuration to update the power, current, and voltage sensors every 60s:

```
sensor:
  - platform: hlw8012
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
