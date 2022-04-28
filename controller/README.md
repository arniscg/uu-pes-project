# Controller

Controller will automatically connect to basestation and send the ambient light sensor value reading using the bluetooth GATT protocol.

## Setup
### Zephyr repository
This code depends on VEML7700 sensor driver that is not in the main Zephyr repository. You have to checkout into this pull request branch https://github.com/zephyrproject-rtos/zephyr/pull/44953 (made by Nikolaus) in your Zephyr repository.

```
git fetch origin pull/44952/head:veml7700
git checkout veml77700
```


### How to connect VEML7700 to DK board

```
Sensor    Board
---------------
VIN  -->  5V
GND  -->  GND
SCL  -->  P0.27
SDA  -->  P0.26
```

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

## Reading output through SEGGER RTT (VMs)
Add thse lines to `prj.conf` file
```
CONFIG_USE_SEGGER_RTT=y
CONFIG_RTT_CONSOLE=y
CONFIG_UART_CONSOLE=n
```

Then run command
```
/opt/SEGGER/JLink/JLinkRTTLogger -Device NRF52840_XXAA -if SWD -Speed 4000 -RTTChannel 0 /dev/stdout
```