west build -p auto -b nrf52840dongle_nrf52840

rm controller_dongle.zip
nrfutil pkg generate --hw-version 52 --sd-req=0x00 --application build/zephyr/zephyr.hex --application-version 1 controller_dongle.zip
nrfutil dfu usb-serial -pkg controller_dongle.zip -p /dev/ttyACM1