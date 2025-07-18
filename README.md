<p align="center">
    <img src="front/logo.svg" height=100 center>
    <h1 align="center">Weather Station</h1>
</p>

A ESP32-based weather station that monitors and displays temperature, humidity, and barometric pressure.

## Overview
This project implements a simple weather monitoring system using an ESP32-S3 microcontroller connected to environmental sensors. It provides a web interface to visualize sensor data in real time through interactive charts.

## Features
- Real-time monitoring of:
    - Temperature (°C)
    - Humidity (%)
    - Barometric pressure (hPa)
- Web interface with interactive charts
- RESTful API for accessing sensor data
- Local network access via mDNS ([weather-station.local](http://weather-station.local/))

## Hardware Requirements
- ESP32-S3 development board
- AHT20 temperature and humidity sensor
- BMP390 barometric pressure sensor

## License
This project is licensed under the GNU General Public License v3.0 or later. It also uses a third-party framework and libraries licensed under the Apache License 2.0 and the MIT License. Both licenses are compatible with the GPLv3.

See the [COPYING](COPYING) file for the full license text.