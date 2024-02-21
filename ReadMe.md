A set of projects with the same functionality "WebRadio" for different WiFi MCU modules:
* ESP8266 (ESP8266MOD)
* ESP32 (ESP32-WROOM-32)
* CC3200 (EByte E103-W02)
* CC3235SF (EByte E103-W06)

with VS1053b module as hardware audio codec.

Projects are mainly educational and research in nature. Projects use the following SDK and toolchains:
* ESP8266
  * [ESP8266_RTOS_SDK Release v3.4](https://github.com/espressif/ESP8266_RTOS_SDK)
  * [mkspiffs](https://github.com/homewsn/mkspiffs)
  * Visual Studio Code (C/C++ extension)
  * Ubuntu
* ESP32
  * [ESP-IDF Release v5.1.2](https://github.com/espressif/esp-idf)
  * [mkspiffs](https://github.com/homewsn/mkspiffs)
  * Visual Studio Code (C/C++ extension)
  * Ubuntu
* CC3200
  * [CC3200SDK 1.3.0](https://www.ti.com/tool/download/CC3200SDK)
  * [UniFlash 3.4.1](https://www.ti.com/tool/download/UNIFLASH/3.4.1)
  * [Pin Mux v4.0.1543](https://software-dl.ti.com/ccs/esd/pinmux/pinmux_release_archive.html)
  * IAR EW ARM 7.60.1
  * Windows
* CC3235SF
  * [SIMPLELINK-CC32XX-SDK 7.10.00.13](https://www.ti.com/tool/SIMPLELINK-CC32XX-SDK)
  * [UniFlash 4+](https://www.ti.com/tool/UNIFLASH)
  * IAR EW ARM 8.50.9
  * Windows
