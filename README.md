# Smart Poultry System - ESP32

A comprehensive, commercial-grade smart poultry monitoring and control system built with ESP32 and ESP-IDF.

## Documentation

- **[System Architecture](docs/SYSTEM_ARCHITECTURE.md)** - Detailed technical documentation of system components, hardware architecture, software design, and control algorithms
- **[User Manual](docs/USER_MANUAL.md)** - Complete user guide covering installation, configuration, operation, and troubleshooting

## Features

### Sensors
- **DHT22**: Temperature and humidity monitoring
- **MQ-137**: Ammonia (NH3) concentration monitoring
- **MH-Z19B**: CO2 concentration monitoring
- **LDR**: Light intensity monitoring
- **Ultrasonic/Float**: Water level monitoring
- **Load Cell**: Feed level monitoring

### Actuators
- **Exhaust Fan**: PWM-controlled ventilation
- **Inlet Fan**: PWM-controlled air intake
- **Heater**: PWM-controlled heating element
- **Feeder Motor**: PWM-controlled feed dispensing
- **Water Pump**: On/Off water dispensing
- **Lighting**: PWM-controlled LED lighting
- **Ventilation Servo**: Servo-controlled ventilation dampers
- **Alarm**: Audible/visual alert system

### Control Algorithms
- **PID Controllers**: Temperature, humidity, and ventilation control
- **Climate Control**: Automated climate management based on sensor data
- **Feeding Schedule**: Hourly automated feeding with customizable amounts
- **Lighting Schedule**: 24-hour lighting schedule with sunrise/sunset simulation
- **Alert System**: Multi-level alerts with cooldown periods

### Connectivity
- **WiFi**: Station and Access Point modes
- **MQTT**: Real-time data publishing and remote control
- **Web Server**: Browser-based monitoring dashboard

### Data Management
- **NVS Storage**: Configuration persistence
- **Data Logger**: Historical data logging with CSV export
- **Alert History**: Alert event tracking

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    ESP32 Microcontroller                     │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   Sensors    │  │   Actuators  │  │   Control    │      │
│  │  - DHT22     │  │  - Fans      │  │  - PID       │      │
│  │  - NH3       │  │  - Heater    │  │  - Climate   │      │
│  │  - CO2       │  │  - Feeder    │  │  - Schedule  │      │
│  │  - Light     │  │  - Pump      │  │  - Alerts    │      │
│  │  - Water     │  │  - Lights    │  │              │      │
│  │  - Feed      │  │  - Vent      │  │              │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│                                                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   WiFi       │  │   MQTT       │  │   Web Server │      │
│  │  - Station   │  │  - Publish   │  │  - Dashboard │      │
│  │  - AP Mode   │  │  - Subscribe │  │  - API       │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│                                                              │
│  ┌──────────────┐  ┌──────────────┐                        │
│  │   Storage    │  │   Logger     │                        │
│  │  - NVS       │  │  - CSV       │                        │
│  │  - Config    │  │  - History   │                        │
│  └──────────────┘  └──────────────┘                        │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

## Hardware Requirements

### ESP32 Development Board
- ESP32-WROOM-32 or similar
- Minimum 4MB flash
- WiFi and Bluetooth support

### Sensors
1. **DHT22** - Temperature & Humidity
   - GPIO: 4
   - Pull-up resistor: 10kΩ

2. **MQ-137** - Ammonia Sensor
   - ADC Channel: GPIO 36 (ADC1_CH0)
   - Voltage: 3.3V

3. **MH-Z19B** - CO2 Sensor
   - ADC Channel: GPIO 39 (ADC1_CH3)
   - Voltage: 5V

4. **LDR** - Light Sensor
   - ADC Channel: GPIO 34 (ADC1_CH6)
   - Voltage divider: 10kΩ

5. **Ultrasonic Sensor** - Water Level
   - ADC Channel: GPIO 35 (ADC1_CH7)
   - Voltage: 3.3V

6. **Load Cell** - Feed Level
   - ADC Channel: GPIO 32 (ADC1_CH4)
   - HX711 amplifier

### Actuators
1. **Exhaust Fan** - 12V DC
   - GPIO: 16
   - MOSFET driver

2. **Inlet Fan** - 12V DC
   - GPIO: 17
   - MOSFET driver

3. **Heater** - 220V AC
   - GPIO: 18
   - Relay module

4. **Feeder Motor** - 12V DC
   - GPIO: 21
   - MOSFET driver

5. **Water Pump** - 12V DC
   - GPIO: 22
   - MOSFET driver

6. **LED Lighting** - 12V DC
   - GPIO: 23
   - MOSFET driver

7. **Ventilation Servo** - 5V
   - GPIO: 26
   - PWM control

8. **Alarm** - 5V Buzzer
   - GPIO: 25
   - Transistor driver

## Software Requirements

### ESP-IDF
- Version: 4.4 or later
- Toolchain: xtensa-esp32-elf

### Required Components
- nvs_flash
- esp_wifi
- esp_event
- esp_netif
- mqtt
- json
- esp_http_server
- driver
- esp_adc
- esp_timer
- spiffs
- fatfs

## Installation

### 1. Install ESP-IDF

```bash
# Clone ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf

# Install tools
./install.sh

# Set environment variables
. ./export.sh
```

### 2. Clone Project

```bash
git clone <repository-url>
cd smart-poultry-system
```

### 3. Configure Project

```bash
idf.py menuconfig
```

Configure the following options:
- **Serial flasher config** → Default serial port
- **Partition table** → Custom partition table CSV
- **ESP32 specific** → CPU frequency → 240MHz

### 4. Build and Flash

```bash
# Build project
idf.py build

# Flash to ESP32
idf.py -p /dev/ttyUSB0 flash

# Monitor output
idf.py -p /dev/ttyUSB0 monitor
```

## Configuration

### WiFi Configuration

Edit `main/app_main.c` and update:

```c
// WiFi configuration
strncpy(system_config.wifi_ssid, "YOUR_WIFI_SSID", WIFI_SSID_MAX_LEN - 1);
strncpy(system_config.wifi_password, "YOUR_WIFI_PASSWORD", WIFI_PASSWORD_MAX_LEN - 1);
```

### MQTT Configuration

```c
// MQTT configuration
strncpy(system_config.mqtt_broker, "mqtt://broker.hivemq.com", MQTT_BROKER_MAX_LEN - 1);
system_config.mqtt_port = 1883;
strncpy(system_config.mqtt_username, "your_username", 31);
strncpy(system_config.mqtt_password, "your_password", 31);
```

### Control Setpoints

```c
// Control setpoints
system_config.temp_setpoint = 26.0f;      // Target temperature (°C)
system_config.humidity_setpoint = 60.0f;  // Target humidity (%)
system_config.ammonia_limit = 25.0f;      // Ammonia limit (ppm)
system_config.co2_limit = 3000.0f;        // CO2 limit (ppm)
```

### Feeding Schedule

```c
// Feeding schedule (grams per hour)
system_config.feeding_schedule[6] = 50;   // 6 AM: 50g
system_config.feeding_schedule[7] = 60;   // 7 AM: 60g
system_config.feeding_schedule[8] = 50;   // 8 AM: 50g
// ... add more hours as needed
```

### Lighting Schedule

```c
// Lighting schedule (intensity per hour, 0-100%)
for (int i = 6; i <= 18; i++) {
    system_config.lighting_schedule[i] = 100;  // Day: 100%
}
system_config.lighting_schedule[5] = 20;      // 5 AM: 20%
system_config.lighting_schedule[19] = 80;     // 7 PM: 80%
```

## Web Dashboard

Access the web dashboard at: `http://<ESP32_IP_ADDRESS>`

### Features
- Real-time sensor data display
- Actuator status monitoring
- System status (uptime, free heap, alerts)
- Manual controls (alarm toggle, emergency stop)
- Auto-refresh every 5 seconds

### API Endpoints

- `GET /` - Web dashboard
- `GET /api/sensors` - Sensor data (JSON)
- `GET /api/actuators` - Actuator states (JSON)
- `GET /api/status` - System status (JSON)
- `POST /api/alarm/toggle` - Toggle alarm
- `POST /api/emergency_stop` - Emergency stop all actuators

## MQTT Topics

### Publish Topics

- `poultry/<client_id>/sensors` - Sensor data
- `poultry/<client_id>/actuators` - Actuator states
- `poultry/<client_id>/alerts` - Alert notifications

### Subscribe Topics

- `poultry/<client_id>/control` - Remote control commands
- `poultry/<client_id>/config` - Configuration updates

### Message Format

```json
{
  "temperature": 26.5,
  "humidity": 62.3,
  "ammonia": 12.4,
  "co2": 1850.0,
  "light": 350.0,
  "water_level": 75.2,
  "feed_level": 45.8,
  "timestamp": 1234567890
}
```

## Alert System

### Alert Types
- `ALERT_TEMP_HIGH` - Temperature too high
- `ALERT_TEMP_LOW` - Temperature too low
- `ALERT_HUMIDITY_HIGH` - Humidity too high
- `ALERT_HUMIDITY_LOW` - Humidity too low
- `ALERT_AMMONIA_HIGH` - Ammonia level critical
- `ALERT_CO2_HIGH` - CO2 level too high
- `ALERT_WATER_LOW` - Water level low
- `ALERT_FEED_LOW` - Feed level low
- `ALERT_SENSOR_FAILURE` - Sensor malfunction
- `ALERT_ACTUATOR_FAILURE` - Actuator malfunction
- `ALERT_SYSTEM_ERROR` - System error

### Alert Thresholds

Default thresholds (configurable):
- Temperature: 20-32°C
- Humidity: 40-70%
- Ammonia: 25 ppm
- CO2: 3000 ppm
- Water Level: 20%
- Feed Level: 15%

## Data Logging

### Log Format
- Timestamp (Unix epoch)
- All sensor readings
- All actuator states
- Active alert type

### Storage
- NVS: Configuration and recent logs
- CSV Export: Available via web API

## Troubleshooting

### WiFi Connection Issues
1. Check SSID and password
2. Ensure WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
3. Check signal strength

### Sensor Reading Issues
1. Verify wiring connections
2. Check power supply voltage
3. Verify ADC channel configuration

### Actuator Control Issues
1. Check GPIO pin assignments
2. Verify MOSFET/relay drivers
3. Check power supply capacity

### MQTT Connection Issues
1. Verify broker URL and port
2. Check username/password
3. Ensure broker is accessible from network

## Performance

### Resource Usage
- Flash: ~1.5MB
- RAM: ~200KB
- CPU: <30% average load

### Timing
- Sensor reading: 5 seconds
- Control loop: 10 seconds
- MQTT publish: 30 seconds
- Data logging: 60 seconds
- Web update: 2 seconds

## License

This project is provided as-is for educational and commercial use.

## Support

For issues and questions:
1. Check the troubleshooting section
2. Review ESP-IDF documentation
3. Check component datasheets

## Version History

### v1.0.0 (Initial Release)
- Complete sensor suite
- Full actuator control
- PID climate control
- Automated scheduling
- WiFi and MQTT connectivity
- Web dashboard
- Data logging
- Alert system

## Credits

Built with:
- ESP-IDF Framework
- FreeRTOS
- ESP32 Microcontroller
