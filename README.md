# Smartwatch Project

A DIY smartwatch project built using Mbed OS that displays time, weather, news, temperature, and humidity with alarm functionality.

## Features

- Real-time clock display
- Customizable alarm with snooze function
- Current weather conditions and temperature (using WeatherAPI)
- Local temperature and humidity sensor readings
- BBC News headlines feed
- Date and time information
- 16x2 LCD display interface
- Four-button navigation system

## Hardware Requirements

- Mbed-compatible development board
- 16x2 LCD Display (DFRobot RGB LCD)
- HTS221 Temperature & Humidity Sensor
- Piezo buzzer
- 4 push buttons
- Network connectivity (WiFi/Ethernet)

## Dependencies

- Mbed OS
- DFRobot_RGBLCD library
- HTS221Sensor library
- nlohmann/json for JSON parsing

## Button Controls

- Button 1: Navigate through pages
- Button 2: Return to home page
- Button 3: Access alarm settings
- Button 4: Toggle alarm ON/OFF

### Alarm Settings
- Button 1: Increase value (+)
- Button 2: Confirm selection
- Button 3: Decrease value (-)

### When Alarm is Active
- Button 1: Stop alarm
- Button 2: Snooze alarm (5 minutes)

## External APIs Used

- WeatherAPI for current weather conditions
- BBC News RSS feed for latest news headlines
- Unix Time API for initial time synchronization

## Setup

1. Connect all hardware components according to the pin configurations in the code
2. Install required libraries in your Mbed development environment
3. Configure your network credentials
4. Compile and flash the code to your device

## Note

You'll need to obtain your own API key for the WeatherAPI service to get weather data.