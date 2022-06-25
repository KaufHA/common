# common
This repo contains common files generally usable by any KAUF product.

***components* directory** - Custom components needed to compile KAUF firmwares. These don't need to be downloaded, the yaml files automatically grab them by reference to this github repo. Every subfolder not starting with kauf_* is copied from stock ESPHome and edited for our products.

***esphome-webserver* directory** - "fork" of the [esphome/esphome-webserver](https://github.com/esphome/esphome-webserver) repo used to generate custom web_server and captive_portal pages.


## Troubleshooting
Any build errors can usually be resolved by upgrading the ESPHome dashboard to the latest version.  On the days when ESPHome updates are released, it may take us up to 24 hours to make necessary changes to our custom components during which time you may see build errors with the new version.  Please be patient.  
  
If you are still getting errors after upgrading the ESPHome dashboard to the latest version, try deleting the .esphome/packages and .esphome/external_components subfolders from within the ESPHome config directory.

If you are stuck on ESPHome version 2022.3.1, you need to uninstall the Home Assistant Add-On, then reinstall using the [Official Add-On Repo](https://github.com/esphome/home-assistant-addon).
