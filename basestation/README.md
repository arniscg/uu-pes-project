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

### Wiring
nrf52840dk pin P0.05 --- LED anode (+) | LED cathode (-) --- resistor --- GND 

### TO-DO
Decide if we want to ignore button messages (mode change requests) whilst in adjustment mode, defer handling them after adjustment or process them immediately.