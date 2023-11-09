# Welcome to OpenMatrix, a project by [Livegrid](https://livegrid.tech/) ✨

<p align="center">
  <img src="https://raw.githubusercontent.com/livegrid/OpenMatrix/main/docs/images/MQTT.webp" alt="Cover Image">
</p>

Use ESP32 to control RGB HUB75 matrices using a simple webserver !

*Please note that this repository is still being setup and is not ready to use yet.*

## ⚙️ Features

<p align="center">
  <img src="https://raw.githubusercontent.com/livegrid/OpenMatrix/main/docs/images/Features.webp" alt="Cover Image">
</p>

- [x] Uses ESP32 DMA based on [this amazing library](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA) to control the matrices leaving the processor free for other important functions.
- [x] Built fully on [ESPAsyncWebserver Library](https://github.com/me-no-dev/ESPAsyncWebServer) enabling fast and easy communication.
- [x] Connect to your local WiFi using a WiFi Manager (partly implemented, uses [Khoih-prog WiFiManager library](https://github.com/khoih-prog/ESPAsync_WiFiManager) for now).
- [x] Library of effects to play with and customise (Fastnoise added as an example for now)
- [x] E.131 supported (Art-Net WIP)
- [x] Use your own OpenWeather free API to pull weather data (WIP)
- [ ] MQTT supported (WIP)
- [ ] OTA supported (WIP)
- [ ] Simple timers/schedules (WIP)

## 🚫 Current Limitations 

- Only tested on ESP-WROOM-32 and and ESP32-S3 modules.
- Can only control upto 8192 pixels, but lot depends on the implementation.

## 🎢 Quick start guide

1. You will need [Platformio](https://platformio.org/) installed on your system to start.
2. Clone the repo and open it on Platformio, using whichever IDE you prefer.
3. Go to 00Platformio folder and open 'platformio.ini'. Make sure the settings are correct for your setup.
4. In the Project Tasks tab, Build Filesystem Image > Upload Filesystem Image > Build > Upload and Monitor.
5. If everything worked, you should see a message on your display.
6. ESP32 will create an access point, you need to connect to it using your phone or computer. This will then take you to the WiFiManager where you can enter your WiFi details.
7. If the connection is succesful, the display will show you the IP address you need to connect to.
8. Make sure you are on the same WiFi, and enter the IP address on any browser to access the webserver.
9. Have fun. 🎉

## 👾 Compatible hardware

See [here](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA)!

## ✌️ Other

Licensed under the MIT license  

This project is hugely inspired from [WLED](https://github.com/Aircoookie/WLED), one the best open source projects I have ever used.

<!-- You can also send me mails to [contact@livegrid.tech](mailto:contact@livegrid.tech), but please, only do so if you want to talk to me privately.   -->