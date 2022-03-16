# common
This repo contains common files used by all or at least a plurality of KAUF products.

***components* directory** - Custom components needed to compile KAUF firmwares. These don't need to be downloaded, the yaml files automatically grab them by reference to this github repo. Every subfolder not starting with kauf_* is copied from stock ESPHome and edited for our products.

***esphome-webserver* directory** - "fork" of the [esphome/esphome-webserver](https://github.com/esphome/esphome-webserver) repo used to generate custom web_server and captive_portal pages.
