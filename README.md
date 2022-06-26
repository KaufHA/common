# common
This repo contains common files generally usable by any KAUF product.

***components* directory** - Custom components needed to compile KAUF firmwares. These don't need to be downloaded, the yaml files automatically grab them by reference to this github repo. Every subfolder not starting with kauf_* is copied from stock ESPHome and edited for our products.

***esphome-webserver* directory** - "fork" of the [esphome/esphome-webserver](https://github.com/esphome/esphome-webserver) repo used to generate custom web_server and captive_portal pages.


## Troubleshooting

### Issues Building Firmware in ESPHome Dashboard

Any build errors can usually be resolved by upgrading the ESPHome dashboard to the latest version.  On the days when ESPHome updates are released, it may take us up to 24 hours to make necessary changes to our custom components during which time you may see build errors with the new version.  Please be patient.  
  
If you are still getting errors after upgrading the ESPHome dashboard to the latest version, try cleaning build files for the device and manually deleting the .esphome/packages and .esphome/external_components subfolders from within the ESPHome config directory.  As a last resort, you can try using our lite or minimal yaml files instead of the default package files.

If you are stuck on ESPHome version 2022.3.1, you need to uninstall the Home Assistant Add-On, then reinstall using the [Official Add-On Repo](https://github.com/esphome/home-assistant-addon).

### Device Not Being Detected by Home Assistant

The ESPHome integration in Home Assistant requires that [zeroconf](https://www.home-assistant.io/integrations/zeroconf) be enabled to automatically detect ESPHome devices on the network. Zeroconf is enabled by default, but may not be enabled if you are not using the [default config](https://www.home-assistant.io/integrations/default_config/). Add zeroconf: to your configuration.yaml file to enable zeroconf.

You can also manually add a device to Home Assistant once the device is connected to Wi-Fi on the same network as your Home Assistant server. You will need to figure out the IP address of the device by logging into your router or by using a network scanning app.

From Home Assistant’s Integrations configuration page, press the add integration button (marked 1 in the figure), and then select ESPHome (2). Enter the IP address of the device in the resulting popup (3).

<img src="https://user-images.githubusercontent.com/89616381/175828410-4348fa85-9092-4681-9117-4ab886ae5242.png" width="50%">   <img src="https://user-images.githubusercontent.com/89616381/175828416-bc0a2acc-2d1a-4baa-b84d-482b7db3bd49.png" width="45%" align=right align=top>



### Can’t Connect to Device’s WiFi AP

If you see a device’s WiFi AP being broadcast, but are having trouble connecting, there is another option. Each KAUF device is pre-programmed to try to connect to a WiFi network with the ssid being either initial_ap or initial_ap2, and the password being asdfasdfasdfasdf. Currently, initial_ap2 is more likely to be right. If you set up a temporary WiFi network with those credentials, the plug or bulb will connect to it instead of broadcasting its own AP. Then you can simply determine the IP address of the device, browse to the IP address, and flash a new firmware from there. Either Tasmota or your own build of ESPHome with your local network credentials built in would work.

### Not Seeing a Device’s WiFi AP

If you are not seeing a device broadcast a WiFi AP as it should, there are ways to re-enable the Wifi AP that may work. For the bulbs, you have to power cycle the bulb without the bulb connecting to WiFi. Let the bulb power on for at least 10-15 seconds before removing power. After powering up a second time, the WiFi AP will be re-enabled. For the plug, hold down the button for at least 5 seconds and the plug will reboot with the WiFi AP enabled.

If the device is connecting to your local WiFi network, then the above procedures will not work. You will need to clear the existing credentials by toggling the “Clear WiFi Credentials” switch. This can be done by browsing to the device’s IP address in a web browser.

### Captive Portal not Showing SSID and Password Fields

When you are connected to a KAUF product’s WiFi AP, you should be able to access a captive portal web page with fields to enter your local network’s WiFi SSID and Password. In some cases, typically if there are an extreme number of networks found, the page may not fully load and the fields to enter your credentials won’t be displayed. The following URL can be used to configure WiFi credentials if you are connected to the KAUF WiFi AP but the captive portal web page is not loading properly. Replace YOUR_SSID with your local network’s WiFi SSID and replace YOUR_PASSWORD with your local network’s WiFi password.

`http://192.168.4.1/wifisave?ssid=YOUR_SSID&psk=YOUR_PASSWORD`

To configure credentials from a Linux command line, use the curl command like so:

`curl 'http://192.168.4.1/wifisave?ssid=YOUR_SSID&psk=YOUR_PASSWORD'`

### Device-Specific Troubleshooting

Some device repos have troubleshooting tips specificaly for those devices:

- [RGB Switch](https://github.com/KaufHA/kauf-rgb-switch#troubleshooting)
