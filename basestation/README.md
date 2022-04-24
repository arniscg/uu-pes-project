# Basestation

Basestation will automatically connect to the controller and print received messages.

### Flash to nrf52840dk
```
west build -p auto -b nrf52840dk_nrf52840
west flash
```

### Flash to nrf52840dongle
In case you want to flash nrf52840 dongle board for testing purposes, there is a script that does just that. Since this board doesn't have J-Link, the flashing process is different and you will need `nrfutil` tool to do so.
```
sh scripts/flash_dongle.sh
```
You might want to change the port in the script.