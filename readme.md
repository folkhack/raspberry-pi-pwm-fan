# PWM Raspberry Pi Fan Controller

<img src="https://github.com/folkhack/raspberry-pi-pwm-fan/blob/multi_pin_support/docs/example_img.jpg" width="300" title="Example Raspberry Pi with PWM Fan Installed" alt="Example Raspberry Pi with PWM Fan Installed">

Lightweight low-complexity fan controller that uses CPU temp that resides at `/usr/sbin/pwm_fan_control`.

Also comes with easy to use systemd service.

---

### Wiring:

**This configuration is for a 5V PWM fan!**

Pin numbers are basic Raspberry Pi "pin 1 starts at J8" numbers (not GPIO/etc. numbers).

* **PWM +5V** - Pin 4
* **PWM Ground** - Pin 6
* **PWM Speed Set** - Pin 12
* **PWM Speed Read** - (not required/supported) Pin 18; bridged to pin 17 (3.3V) with a 1k Ohm resistor

Wiring Notes:

* For a better visual follow wiring diagram listed here: https://blog.driftking.tw/en/2019/11/Using-Raspberry-Pi-to-Control-a-PWM-Fan-and-Monitor-its-Speed/#Wiring
* The PWM speed read is optional and can be wired if you want to read the actual fan speed as seen in the previous link

---

### Building/Installing:

**Requirements:**
* `wiringPi.h` obtainable from https://github.com/WiringPi/WiringPi (`build` script is in root of WiringPi project)

```bash
# Compile
make compile

# Compile with debugging enabled
make compile-debug

# Running (requires sudo to control GPIO)
sudo ./pwm_fan_control

# Installing (will make persistent service that starts at boot)
sudo make install
sudo make uninstall
```

---

### Use

```bash
# Display help:
pwm_fan_control --help

# Raspberry Pi CPU PWM Fan Controller 
#
# Usage: pwm_fan_control [OPTION]... 
#
# Watches CPU temp and sets PWM fan speed accordingly with WiringPi library.
#
# --gpio BCM_GPIO_PIN_NUMBER       (default 18) BCM GPIO pin # for setting PWM fan speed 
#                                               12, 13, 18, 19 pins supported (hardware PWM) 
#
# Exit status: 1 if error 

##############

# Set a different PWM speed set GPIO pin
pwm_fan_control --gpio 13
```

**systemd Service GPIO Override:** If you're using this with a GPIO different than 18 you will want to update the
systemd service to reference the correct gpio.

```bash
sudo nano /etc/systemd/system/
# Replace "ExecStart=/usr/sbin/pwm_fan_control" with "ExecStart=/usr/sbin/pwm_fan_control --gpio 13"
```

---

### Helpful Resources:

* https://www.electronicwings.com/raspberry-pi/raspberry-pi-pwm-generation-using-python-and-c
* https://blog.driftking.tw/en/2019/11/Using-Raspberry-Pi-to-Control-a-PWM-Fan-and-Monitor-its-Speed/#Wiring
* https://www.raspberrypi.org/forums/viewtopic.php?f=32&t=133251&sid=519dae0c236d41f941c140190fe2bec7
* https://raspberrypi.stackexchange.com/a/6979
* http://wiringpi.com/
