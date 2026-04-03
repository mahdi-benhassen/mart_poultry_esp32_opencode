# Smart Poultry System - User Manual

## Table of Contents
1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [Hardware Setup](#hardware-setup)
4. [Software Installation](#software-installation)
5. [Initial Configuration](#initial-configuration)
6. [Web Dashboard](#web-dashboard)
7. [MQTT Integration](#mqtt-integration)
8. [Operating Modes](#operating-modes)
9. [Sensor Monitoring](#sensor-monitoring)
10. [Actuator Control](#actuator-control)
11. [Schedule Management](#schedule-management)
12. [Alert System](#alert-system)
13. [Data Logging](#data-logging)
14. [Troubleshooting](#troubleshooting)
15. [Maintenance](#maintenance)
16. [Safety Guidelines](#safety-guidelines)

---

## Introduction

Welcome to the Smart Poultry System - a comprehensive, commercial-grade monitoring and control solution for poultry farms. This system automates environmental management, ensuring optimal conditions for your poultry while providing remote monitoring and control capabilities.

### What This System Does

- **Monitors** temperature, humidity, ammonia, CO2, light, water level, and feed level
- **Controls** fans, heater, feeder, water pump, lighting, and ventilation
- **Automates** feeding schedules and lighting schedules
- **Alerts** you when conditions exceed safe thresholds
- **Logs** data for historical analysis and compliance
- **Connects** via WiFi for remote monitoring and control

### Target Audience

This manual is designed for:
- Farm owners and operators
- Technical staff responsible for system maintenance
- IT personnel handling network integration
- Anyone responsible for poultry welfare

---

## Getting Started

### What's in the Box

When you receive your Smart Poultry System, verify you have:

1. **ESP32 Development Board** (1x)
2. **Sensor Kit**:
   - DHT22 Temperature/Humidity sensor
   - MQ-137 Ammonia sensor
   - MH-Z19B CO2 sensor
   - LDR Light sensor
   - Ultrasonic Water level sensor
   - Load Cell Feed level sensor
   - Gas sensor (MQ-2 or similar)
3. **Actuator Kit**:
   - Exhaust Fan (12V DC)
   - Inlet Fan (12V DC)
   - Heater (220V AC)
   - Feeder Motor (12V DC)
   - Water Pump (12V DC)
   - LED Lighting (12V DC)
   - Ventilation Servo (5V)
   - Alarm Buzzer (5V)
4. **Power Supply** (12V/5A recommended)
5. **Connecting Cables** and **Mounting Hardware**

### Quick Start Checklist

- [ ] Unpack and inspect all components
- [ ] Install ESP-IDF development environment
- [ ] Flash firmware to ESP32
- [ ] Connect sensors and actuators
- [ ] Configure WiFi credentials
- [ ] Power on the system
- [ ] Access web dashboard
- [ ] Verify sensor readings
- [ ] Test actuator controls
- [ ] Configure schedules

---

## Hardware Setup

### Step 1: ESP32 Preparation

1. **Install ESP-IDF** (if not already installed):
   ```bash
   # Clone ESP-IDF
   git clone --recursive https://github.com/espressif/esp-idf.git
   cd esp-idf
   
   # Install tools (Linux/Mac)
   ./install.sh
   
   # Set environment
   . ./export.sh
   ```

2. **Connect ESP32** to your computer via USB cable

3. **Verify Connection**:
   ```bash
   idf.py -p /dev/ttyUSB0 monitor
   ```
   You should see boot messages from the ESP32

### Step 2: Sensor Wiring

#### DHT22 (Temperature/Humidity)
```
DHT22 Pin    →    ESP32 Pin
VCC          →    3.3V
DATA         →    GPIO 4
GND          →    GND
NC           →    Not connected
```
**Note**: Add a 10kΩ pull-up resistor between VCC and DATA

#### MQ-137 (Ammonia Sensor)
```
MQ-137 Pin   →    ESP32 Pin
VCC          →    5V
AOUT         →    GPIO 36 (ADC1_CH0)
GND          →    GND
```

#### MH-Z19B (CO2 Sensor)
```
MH-Z19B Pin  →    ESP32 Pin
VCC          →    5V
AOUT         →    GPIO 39 (ADC1_CH3)
GND          →    GND
```

#### LDR (Light Sensor)
```
LDR Pin      →    ESP32 Pin
One leg      →    3.3V
Other leg    →    GPIO 34 (ADC1_CH2)
             ↓
         10kΩ resistor
             ↓
           GND
```

#### Ultrasonic (Water Level)
```
Ultrasonic Pin →  ESP32 Pin
VCC            →  3.3V
AOUT           →  GPIO 35 (ADC1_CH7)
GND            →  GND
```

#### Load Cell (Feed Level)
```
Load Cell Pin →   ESP32 Pin
E+            →   5V
E-            →   GND
A+            →   GPIO 32 (ADC1_CH4)
A-            →   GND
```

#### Gas Sensor
```
Gas Sensor Pin →  ESP32 Pin
VCC            →  5V
AOUT           →  GPIO 33 (ADC1_CH5)
GND            →  GND
```

### Step 3: Actuator Wiring

#### Exhaust Fan
```
Fan Pin      →    ESP32 Pin
VCC (12V)    →    12V Power Supply
GND          →    GND
Control      →    GPIO 16 (via MOSFET)
```

#### Inlet Fan
```
Fan Pin      →    ESP32 Pin
VCC (12V)    →    12V Power Supply
GND          →    GND
Control      →    GPIO 17 (via MOSFET)
```

#### Heater
```
Heater Pin   →    ESP32 Pin
Live (220V)  →    Relay COM
Neutral      →    Direct to mains
Control      →    GPIO 18 (via Relay)
```
**⚠️ WARNING**: High voltage! Use proper relay module and follow electrical safety codes.

#### Feeder Motor
```
Motor Pin    →    ESP32 Pin
VCC (12V)    →    12V Power Supply
GND          →    GND
Control      →    GPIO 21 (via MOSFET)
```

#### Water Pump
```
Pump Pin     →    ESP32 Pin
VCC (12V)    →    12V Power Supply
GND          →    GND
Control      →    GPIO 22 (via MOSFET)
```

#### LED Lighting
```
LED Pin      →    ESP32 Pin
VCC (12V)    →    12V Power Supply
GND          →    GND
Control      →    GPIO 23 (via MOSFET)
```

#### Ventilation Servo
```
Servo Pin    →    ESP32 Pin
VCC (5V)     →    5V Power Supply
Signal       →    GPIO 26
GND          →    GND
```

#### Alarm Buzzer
```
Buzzer Pin   →    ESP32 Pin
VCC (5V)     →    5V Power Supply
Signal       →    GPIO 25 (via Transistor)
GND          →    GND
```

### Step 4: Power Supply

1. **Calculate Total Power**:
   - ESP32: ~1W
   - Sensors: ~2W
   - Fans: ~12W
   - Heater: ~1100W (if used)
   - Other actuators: ~20W

2. **Recommended Power Supply**:
   - **12V/10A** for actuators (without heater)
   - **12V/100A** or separate 220V circuit for heater
   - **5V/2A** for servo and buzzer

3. **Wiring**:
   ```
   12V Power Supply
   ├── ESP32 VIN
   ├── Exhaust Fan
   ├── Inlet Fan
   ├── Feeder Motor
   ├── Water Pump
   └── LED Lighting
   
   5V Power Supply
   ├── Servo
   └── Buzzer
   
   220V Mains
   └── Heater (via Relay)
   ```

---

## Software Installation

### Step 1: Clone Repository

```bash
git clone <repository-url>
cd smart-poultry-system
```

### Step 2: Configure Project

```bash
idf.py menuconfig
```

Navigate through:
- **Serial flasher config** → Default serial port → Select your port
- **Partition table** → Custom partition table CSV → Select `partitions.csv`
- **ESP32 specific** → CPU frequency → 240MHz

### Step 3: Build Firmware

```bash
idf.py build
```

Expected output:
```
Project build complete.
To flash, run:
idf.py -p /dev/ttyUSB0 flash
```

### Step 4: Flash to ESP32

```bash
idf.py -p /dev/ttyUSB0 flash
```

### Step 5: Monitor Output

```bash
idf.py -p /dev/ttyUSB0 monitor
```

You should see:
```
I (xxx) APP_MAIN: ========================================
I (xxx) APP_MAIN: Smart Poultry System v1.0.0
I (xxx) APP_MAIN: ========================================
I (xxx) APP_MAIN: Initializing subsystems...
I (xxx) APP_MAIN: Sensor manager initialized
I (xxx) APP_MAIN: Gas sensor initialized
I (xxx) APP_MAIN: Actuator manager initialized
I (xxx) APP_MAIN: Climate control initialized
I (xxx) APP_MAIN: Feeding schedule initialized
I (xxx) APP_MAIN: Lighting schedule initialized
I (xxx) APP_MAIN: Alert system initialized
I (xxx) APP_MAIN: Gas leak detector initialized
I (xxx) APP_MAIN: WiFi manager initialized
I (xxx) APP_MAIN: MQTT client initialized
I (xxx) APP_MAIN: Data logger initialized
I (xxx) APP_MAIN: All subsystems initialized successfully
I (xxx) APP_MAIN: Connecting to WiFi...
I (xxx) APP_MAIN: WiFi connected successfully
I (xxx) APP_MAIN: Connecting to MQTT broker...
I (xxx) APP_MAIN: MQTT connected successfully
I (xxx) APP_MAIN: All tasks created successfully
I (xxx) APP_MAIN: ========================================
I (xxx) APP_MAIN: System initialized successfully!
I (xxx) APP_MAIN: Web server: http://192.168.1.100:80
I (xxx) APP_MAIN: ========================================
```

---

## Initial Configuration

### WiFi Configuration

Edit `main/app_main.c` and update the WiFi configuration section:

```c
// WiFi configuration
strncpy(system_config.wifi_ssid, "YOUR_WIFI_SSID", WIFI_SSID_MAX_LEN - 1);
strncpy(system_config.wifi_password, "YOUR_WIFI_PASSWORD", WIFI_PASSWORD_MAX_LEN - 1);
```

**Rebuild and flash** after making changes:
```bash
idf.py build flash
```

### MQTT Configuration

Update the MQTT configuration section in `main/app_main.c`:

```c
// MQTT configuration
strncpy(system_config.mqtt_broker, "mqtt://your-broker.com", MQTT_BROKER_MAX_LEN - 1);
system_config.mqtt_port = 1883;
strncpy(system_config.mqtt_username, "your_username", 31);
strncpy(system_config.mqtt_password, "your_password", 31);
strncpy(system_config.mqtt_client_id, "poultry_001", MQTT_CLIENT_ID_MAX_LEN - 1);
```

### Control Setpoints

Update the control setpoints section in `main/app_main.c`:

```c
// Control setpoints
system_config.temp_setpoint = 26.0f;      // Target temperature (°C)
system_config.humidity_setpoint = 60.0f;  // Target humidity (%)
system_config.ammonia_limit = 25.0f;      // Ammonia limit (ppm)
system_config.co2_limit = 3000.0f;        // CO2 limit (ppm)
```

### Feeding Schedule

Update the feeding schedule section in `main/app_main.c`:

```c
// Feeding schedule (grams per hour)
memset(system_config.feeding_schedule, 0, sizeof(system_config.feeding_schedule));
system_config.feeding_schedule[6] = 50;   // 6 AM: 50g
system_config.feeding_schedule[7] = 60;   // 7 AM: 60g
system_config.feeding_schedule[8] = 50;   // 8 AM: 50g
system_config.feeding_schedule[11] = 40;  // 11 AM: 40g
system_config.feeding_schedule[12] = 50;  // 12 PM: 50g
system_config.feeding_schedule[13] = 40;  // 1 PM: 40g
system_config.feeding_schedule[15] = 40;  // 3 PM: 40g
system_config.feeding_schedule[16] = 50;  // 4 PM: 50g
system_config.feeding_schedule[17] = 40;  // 5 PM: 40g
system_config.feeding_schedule[18] = 30;  // 6 PM: 30g
system_config.feeding_schedule[19] = 20;  // 7 PM: 20g
```

### Lighting Schedule

Update the lighting schedule section in `main/app_main.c`:

```c
// Lighting schedule (intensity per hour, 0-100%)
memset(system_config.lighting_schedule, 0, sizeof(system_config.lighting_schedule));
for (int i = 6; i <= 18; i++) {
    system_config.lighting_schedule[i] = 100;  // Day: 100%
}
system_config.lighting_schedule[5] = 20;      // 5 AM: 20%
system_config.lighting_schedule[19] = 80;     // 7 PM: 80%
system_config.lighting_schedule[20] = 60;     // 8 PM: 60%
system_config.lighting_schedule[21] = 40;     // 9 PM: 40%
system_config.lighting_schedule[22] = 20;     // 10 PM: 20%
```

---

## Web Dashboard

### Accessing the Dashboard

1. **Find ESP32 IP Address**:
   - Check serial monitor output
   - Or check your router's connected devices list
   - Default: `http://192.168.1.100`

2. **Open Browser**:
   ```
   http://<ESP32_IP_ADDRESS>
   ```

### Dashboard Features

#### Real-Time Sensor Display
- **Temperature**: Current temperature in °C
- **Humidity**: Current humidity in %
- **Ammonia**: NH3 concentration in ppm
- **CO2**: Carbon dioxide in ppm
- **Light**: Light intensity in lux
- **Water Level**: Water tank level in %
- **Feed Level**: Feed hopper level in %

#### Actuator Status
- **Exhaust Fan**: Speed (0-100%)
- **Inlet Fan**: Speed (0-100%)
- **Heater**: Power (0-100%)
- **Feeder**: Speed (0-100%)
- **Water Pump**: On/Off
- **Lighting**: Intensity (0-100%)
- **Ventilation**: Position (0-100%)
- **Alarm**: On/Off

#### System Status
- **Uptime**: System running time
- **Free Heap**: Available memory
- **Active Alert**: Current alert type
- **WiFi Status**: Connection status
- **MQTT Status**: Broker connection

#### Manual Controls
- **Toggle Alarm**: Turn alarm on/off
- **Emergency Stop**: Stop all actuators immediately

### API Endpoints

#### Get Sensor Data
```bash
curl http://<ESP32_IP>/api/sensors
```

Response:
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

#### Get Actuator States
```bash
curl http://<ESP32_IP>/api/actuators
```

Response:
```json
{
  "exhaust_fan_speed": 45,
  "inlet_fan_speed": 40,
  "heater_power": 0,
  "feeder_speed": 0,
  "water_pump_state": 0,
  "lighting_intensity": 100,
  "ventilation_position": 50,
  "alarm_state": false
}
```

#### Get System Status
```bash
curl http://<ESP32_IP>/api/status
```

Response:
```json
{
  "state": 1,
  "active_alert": 0,
  "uptime_seconds": 86400,
  "free_heap_size": 200000
}
```

#### Toggle Alarm
```bash
curl -X POST http://<ESP32_IP>/api/alarm/toggle
```

#### Emergency Stop
```bash
curl -X POST http://<ESP32_IP>/api/emergency_stop
```

---

## MQTT Integration

### Connecting to MQTT

The system connects to your MQTT broker automatically on startup. Verify connection in serial monitor:

```
I (xxx) MQTT_CLIENT: Connected to MQTT broker
I (xxx) MQTT_CLIENT: Subscribed to poultry/esp32_001/control
I (xxx) MQTT_CLIENT: Subscribed to poultry/esp32_001/config
```

### Topic Structure

#### Publish Topics (System → Broker)

| Topic | Description | Frequency |
|-------|-------------|-----------|
| `poultry/<client_id>/sensors` | Sensor data | Every 30 seconds |
| `poultry/<client_id>/actuators` | Actuator states | Every 30 seconds |
| `poultry/<client_id>/alerts` | Alert notifications | On alert |

#### Subscribe Topics (Broker → System)

| Topic | Description | Format |
|-------|-------------|--------|
| `poultry/<client_id>/control` | Remote commands | JSON |
| `poultry/<client_id>/config` | Configuration updates | JSON |

### Message Formats

#### Sensor Data Message
```json
{
  "temperature": 26.5,
  "humidity": 62.3,
  "ammonia": 12.4,
  "co2": 1850.0,
  "light": 350.0,
  "water_level": 75.2,
  "feed_level": 45.8,
  "gas_ppm": 5.2,
  "gas_alarm": false,
  "timestamp": 1234567890
}
```

#### Actuator States Message
```json
{
  "exhaust_fan_speed": 45,
  "inlet_fan_speed": 40,
  "heater_power": 0,
  "cooler_power": 0,
  "feeder_speed": 0,
  "water_pump_state": 0,
  "lighting_intensity": 100,
  "ventilation_position": 50,
  "curtain_position": 0,
  "alarm_state": false
}
```

#### Alert Message
```json
{
  "alert_type": "TEMP_HIGH",
  "severity": "WARNING",
  "message": "Temperature exceeded threshold: 33.5°C",
  "timestamp": 1234567890
}
```

### Remote Control Commands

#### Set Temperature Setpoint
```bash
mosquitto_pub -h broker.hivemq.com -t "poultry/esp32_001/control" -m '{
  "command": "set_temp_setpoint",
  "value": 27.0
}'
```

#### Set Humidity Setpoint
```bash
mosquitto_pub -h broker.hivemq.com -t "poultry/esp32_001/control" -m '{
  "command": "set_humidity_setpoint",
  "value": 65.0
}'
```

#### Toggle Alarm
```bash
mosquitto_pub -h broker.hivemq.com -t "poultry/esp32_001/control" -m '{
  "command": "toggle_alarm"
}'
```

#### Emergency Stop
```bash
mosquitto_pub -h broker.hivemq.com -t "poultry/esp32_001/control" -m '{
  "command": "emergency_stop"
}'
```

### Monitoring with MQTT Client

#### Using mosquitto_sub
```bash
# Monitor all sensor data
mosquitto_sub -h broker.hivemq.com -t "poultry/+/sensors" -v

# Monitor alerts
mosquitto_sub -h broker.hivemq.com -t "poultry/+/alerts" -v

# Monitor actuator states
mosquitto_sub -h broker.hivemq.com -t "poultry/+/actuators" -v
```

#### Using MQTT Explorer
1. Download MQTT Explorer from [mqtt-explorer.com](http://mqtt-explorer.com)
2. Connect to your broker
3. Navigate to `poultry` → `<client_id>`
4. View real-time data

---

## Operating Modes

### Automatic Mode (Default)

In automatic mode, the system:
- Continuously monitors sensors
- Adjusts actuators based on PID control
- Executes feeding and lighting schedules
- Triggers alerts when thresholds exceeded

**When to use**: Normal operation

### Manual Mode

In manual mode, you have direct control:
- Set actuator speeds manually via web dashboard or MQTT
- Schedules are disabled
- Alerts still active

**When to use**: Testing, maintenance, or special situations

### Scheduled Mode

In scheduled mode:
- Only feeding and lighting schedules are active
- Climate control is disabled
- Manual override available

**When to use**: When you want predictable schedules without automatic climate control

### Maintenance Mode

In maintenance mode:
- All automatic controls disabled
- Safe states for all actuators
- Data logging continues
- Web interface accessible

**When to use**: System maintenance, cleaning, or repairs

### Switching Modes

#### Via Web Dashboard
1. Access dashboard at `http://<ESP32_IP>`
2. Navigate to Settings
3. Select operating mode
4. Click Save

#### Via MQTT
```bash
mosquitto_pub -h broker.hivemq.com -t "poultry/esp32_001/config" -m '{
  "operating_mode": "MANUAL"
}'
```

#### Via Serial Monitor
```
The system outputs status information via ESP_LOG on the serial port.
Use `idf.py monitor` to view system logs.
```

---

## Sensor Monitoring

### Understanding Sensor Readings

#### Temperature
- **Optimal Range**: 24-28°C
- **Warning Threshold**: <20°C or >32°C
- **Critical Threshold**: <18°C or >35°C
- **Action**: Heater activates if too cold, fans activate if too hot

#### Humidity
- **Optimal Range**: 50-65%
- **Warning Threshold**: <40% or >70%
- **Critical Threshold**: <35% or >75%
- **Action**: Ventilation adjusts to maintain optimal levels

#### Ammonia (NH3)
- **Safe Level**: <20 ppm
- **Warning Threshold**: >20 ppm
- **Critical Threshold**: >25 ppm
- **Action**: Ventilation increases to dilute ammonia

#### CO2
- **Safe Level**: <2500 ppm
- **Warning Threshold**: >2500 ppm
- **Critical Threshold**: >3000 ppm
- **Action**: Ventilation increases to bring in fresh air

#### Light Intensity
- **Optimal Range**: 20-50 lux
- **Adjustment**: Based on lighting schedule
- **Action**: Lighting adjusts to match schedule

#### Water Level
- **Minimum**: 20%
- **Critical**: <10%
- **Action**: Pump activates to refill (if connected)

#### Feed Level
- **Minimum**: 15%
- **Critical**: <5%
- **Action**: Alert notification (manual refill required)

#### Gas (Combustible)
- **Safe Level**: <50 ppm
- **Warning Threshold**: >50 ppm
- **Critical Threshold**: >100 ppm
- **Action**: Emergency ventilation, alarm activation

### Sensor Calibration

#### DHT22 Calibration
The DHT22 is factory-calibrated. If readings seem off:
1. Compare with a reference thermometer/hygrometer
2. Note the offset
3. Adjust in software (future feature)

#### MQ-137 Calibration
1. Allow 24-48 hours for initial warm-up
2. Calibrate in clean air (0 ppm ammonia)
3. Use calibration gas for accurate readings

#### MH-Z19B Calibration
1. Auto-calibrates every 24 hours
2. Manual calibration via UART command
3. Zero-point calibration in fresh air

#### Load Cell Calibration
1. Empty hopper: Record ADC value
2. Full hopper: Record ADC value
3. Linear interpolation for percentage

---

## Actuator Control

### Fan Control

#### Exhaust Fan
- **Purpose**: Remove stale air and heat
- **Control**: PWM (0-100%)
- **Automatic**: Adjusts based on temperature and gas levels
- **Manual**: Set via dashboard or MQTT

#### Inlet Fan
- **Purpose**: Bring in fresh air
- **Control**: PWM (0-100%)
- **Automatic**: Coordinates with exhaust fan
- **Manual**: Set via dashboard or MQTT

### Heater Control

- **Purpose**: Raise temperature when too cold
- **Control**: PWM (0-100%) via relay
- **Safety**: Thermal shutdown at 40°C
- **Automatic**: PID-controlled based on temperature error

### Feeder Control

- **Purpose**: Dispense feed according to schedule
- **Control**: PWM (0-100%)
- **Automatic**: Activates at scheduled times
- **Manual**: Trigger feed dispensing via dashboard

### Water Pump Control

- **Purpose**: Maintain water level
- **Control**: On/Off
- **Automatic**: Activates when level drops below threshold
- **Manual**: Toggle via dashboard

### Lighting Control

- **Purpose**: Provide appropriate lighting
- **Control**: PWM (0-100%)
- **Automatic**: Adjusts based on lighting schedule
- **Manual**: Set intensity via dashboard

### Ventilation Control

- **Purpose**: Control air flow direction
- **Control**: Servo position (0-180°)
- **Automatic**: Adjusts based on gas levels
- **Manual**: Set position via dashboard

### Alarm Control

- **Purpose**: Alert operators of critical conditions
- **Control**: On/Off
- **Automatic**: Activates on critical alerts
- **Manual**: Toggle via dashboard

### Emergency Stop

**⚠️ USE IN EMERGENCIES ONLY**

Activating emergency stop:
1. Immediately stops all actuators
2. Disables heater
3. Sets fans to maximum
4. Activates alarm
5. Requires manual reset

**How to activate**:
- Web Dashboard: Click "Emergency Stop" button
- MQTT: Send emergency stop command
- Serial: Type `emergency_stop`

---

## Schedule Management

### Feeding Schedule

The feeding schedule controls when and how much feed is dispensed.

#### Default Schedule
| Time | Amount |
|------|--------|
| 6:00 AM | 50g |
| 7:00 AM | 60g |
| 8:00 AM | 50g |
| 11:00 AM | 40g |
| 12:00 PM | 50g |
| 1:00 PM | 40g |
| 3:00 PM | 40g |
| 4:00 PM | 50g |
| 5:00 PM | 40g |
| 6:00 PM | 30g |
| 7:00 PM | 20g |

#### Modifying Schedule

Edit the feeding schedule section in `main/app_main.c`:

```c
// Feeding schedule (grams per hour)
memset(system_config.feeding_schedule, 0, sizeof(system_config.feeding_schedule));
system_config.feeding_schedule[6] = 50;   // 6 AM: 50g
system_config.feeding_schedule[7] = 60;   // 7 AM: 60g
// ... add more hours as needed
```

**Rebuild and flash** after changes.

### Lighting Schedule

The lighting schedule controls light intensity throughout the day.

#### Default Schedule
| Time | Intensity |
|------|-----------|
| 5:00 AM | 20% |
| 6:00 AM - 6:00 PM | 100% |
| 7:00 PM | 80% |
| 8:00 PM | 60% |
| 9:00 PM | 40% |
| 10:00 PM | 20% |
| 11:00 PM - 4:00 AM | 0% |

#### Modifying Schedule

Edit the lighting schedule section in `main/app_main.c`:

```c
// Lighting schedule (intensity per hour, 0-100%)
memset(system_config.lighting_schedule, 0, sizeof(system_config.lighting_schedule));
for (int i = 6; i <= 18; i++) {
    system_config.lighting_schedule[i] = 100;  // Day: 100%
}
system_config.lighting_schedule[5] = 20;      // 5 AM: 20%
// ... add more hours as needed
```

**Rebuild and flash** after changes.

---

## Alert System

### Alert Types

| Alert Type | Description | Severity | Action |
|------------|-------------|----------|--------|
| `ALERT_TEMP_HIGH` | Temperature too high | WARNING | Fans increase |
| `ALERT_TEMP_LOW` | Temperature too low | WARNING | Heater activates |
| `ALERT_HUMIDITY_HIGH` | Humidity too high | WARNING | Ventilation increases |
| `ALERT_HUMIDITY_LOW` | Humidity too low | WARNING | Ventilation decreases |
| `ALERT_AMMONIA_HIGH` | Ammonia level critical | CRITICAL | Ventilation maximum |
| `ALERT_CO2_HIGH` | CO2 level too high | CRITICAL | Ventilation maximum |
| `ALERT_WATER_LOW` | Water level low | WARNING | Pump activates |
| `ALERT_FEED_LOW` | Feed level low | WARNING | Notification only |
| `ALERT_GAS_LEAK` | Combustible gas detected | CRITICAL | Emergency ventilation |
| `ALERT_SENSOR_FAILURE` | Sensor malfunction | ERROR | Check wiring |
| `ALERT_ACTUATOR_FAILURE` | Actuator malfunction | ERROR | Check wiring |
| `ALERT_SYSTEM_ERROR` | System error | ERROR | Check logs |

### Alert Behavior

1. **Detection**: System continuously monitors thresholds
2. **Triggering**: Alert activates when threshold exceeded
3. **Cooldown**: 5-minute cooldown between same alert types
4. **Notification**: 
   - Serial monitor log
   - MQTT publish (if connected)
   - Web dashboard update
   - Alarm activation (if enabled)

### Configuring Alerts

#### Enable/Disable Alerts
```c
system_config.alerts_enabled = true;  // or false
```

#### Adjust Thresholds
```c
system_config.temp_setpoint = 26.0f;      // Target temperature
system_config.humidity_setpoint = 60.0f;  // Target humidity
system_config.ammonia_limit = 25.0f;      // Ammonia limit
system_config.co2_limit = 3000.0f;        // CO2 limit
```

---

## Data Logging

### What Gets Logged

Every 60 seconds, the system logs:
- Timestamp (Unix epoch)
- All sensor readings
- All actuator states
- Active alert type

### Log Format

```
timestamp,temperature,humidity,ammonia,co2,light,water_level,feed_level,gas,alert_type
1234567890,26.5,62.3,12.4,1850.0,350.0,75.2,45.8,5.2,0
```

### Accessing Logs

#### Via Web API
```bash
# Get recent logs
curl http://<ESP32_IP>/api/logs

# Download CSV
curl http://<ESP32_IP>/api/logs/csv -o poultry_data.csv
```

#### Via Serial Monitor
```
logs
```

#### Via MQTT
Logs are published to `poultry/<client_id>/logs` topic

### Log Storage

- **NVS Storage**: Recent logs (last 1000 entries)
- **SPIFFS**: Historical logs (up to 10 files, 1MB each)
- **Automatic Rotation**: Oldest logs deleted when storage full

### Analyzing Data

#### Import to Spreadsheet
1. Download CSV via web API
2. Open in Excel, Google Sheets, or LibreOffice
3. Create charts and analyze trends

#### Python Analysis
```python
import pandas as pd
import matplotlib.pyplot as plt

# Load data
df = pd.read_csv('poultry_data.csv')

# Plot temperature
plt.figure(figsize=(12, 6))
plt.plot(df['timestamp'], df['temperature'])
plt.xlabel('Time')
plt.ylabel('Temperature (°C)')
plt.title('Temperature Over Time')
plt.show()
```

---

## Troubleshooting

### WiFi Connection Issues

**Symptom**: System shows "Failed to connect to WiFi"

**Solutions**:
1. Verify SSID and password in `main/app_main.c`
2. Ensure WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
3. Check signal strength (move ESP32 closer to router)
4. Verify router isn't blocking new devices
5. Check serial monitor for detailed error messages

### Sensor Reading Issues

**Symptom**: Sensors show 0 or invalid values

**Solutions**:
1. **Check wiring**: Verify all connections match pin mapping
2. **Check power**: Ensure sensors receive correct voltage
3. **Check ADC**: Verify ADC channels aren't conflicting
4. **Check timing**: Some sensors need warm-up time
5. **Check serial monitor**: Look for error messages

**Specific Sensor Issues**:

| Sensor | Common Issue | Solution |
|--------|--------------|----------|
| DHT22 | No response | Check pull-up resistor |
| MQ-137 | Always 0 | Allow 24h warm-up |
| MH-Z19B | Invalid readings | Check UART connections |
| LDR | No change | Check voltage divider |
| Ultrasonic | Always max | Check TRIG/ECHO pins |
| Load Cell | No change | Check HX711 connections |

### Actuator Control Issues

**Symptom**: Actuators don't respond

**Solutions**:
1. **Check GPIO**: Verify pin assignments match code
2. **Check power**: Ensure adequate power supply
3. **Check drivers**: Verify MOSFETs/relays are working
4. **Check serial monitor**: Look for error messages
5. **Test manually**: Use GPIO commands to test

### MQTT Connection Issues

**Symptom**: "Failed to connect to MQTT broker"

**Solutions**:
1. Verify broker URL and port
2. Check username/password
3. Ensure broker is accessible from network
4. Check firewall settings
5. Try different broker (e.g., broker.hivemq.com)

### Web Dashboard Issues

**Symptom**: Can't access web dashboard

**Solutions**:
1. Verify ESP32 IP address (check serial monitor)
2. Ensure you're on the same network
3. Try different browser
4. Check firewall settings
5. Verify web server task is running

### System Freezes

**Symptom**: System stops responding

**Solutions**:
1. Check power supply stability
2. Monitor free heap memory
3. Check for infinite loops in code
4. Review task stack sizes
5. Check for watchdog timer resets

---

## Maintenance

### Daily Checks

- [ ] Verify sensor readings are reasonable
- [ ] Check actuator responses
- [ ] Review alert history
- [ ] Monitor system uptime

### Weekly Checks

- [ ] Clean sensor surfaces
- [ ] Check wiring connections
- [ ] Verify water and feed levels
- [ ] Review data logs for anomalies
- [ ] Test emergency stop

### Monthly Checks

- [ ] Calibrate sensors (if needed)
- [ ] Check power supply voltages
- [ ] Inspect actuator mechanisms
- [ ] Update firmware (if available)
- [ ] Backup configuration

### Quarterly Checks

- [ ] Full system inspection
- [ ] Replace worn components
- [ ] Update documentation
- [ ] Performance optimization
- [ ] Security audit

### Cleaning Procedures

#### Sensors
1. Power off system
2. Gently clean with soft brush
3. Use compressed air for dust
4. Avoid liquid cleaners
5. Allow to dry completely

#### Actuators
1. Power off system
2. Remove debris from fans
3. Lubricate moving parts
4. Check for wear
5. Test operation

#### ESP32
1. Power off system
2. Clean with compressed air
3. Check for corrosion
4. Verify connections
5. Test operation

---

## Safety Guidelines

### Electrical Safety

**⚠️ HIGH VOLTAGE WARNING**

The heater operates at 220V AC. Follow these guidelines:

1. **Always** use a proper relay module
2. **Never** work on live circuits
3. **Use** appropriate wire gauges
4. **Install** proper fuses and circuit breakers
5. **Follow** local electrical codes
6. **Ground** all metal enclosures
7. **Keep** away from water

### Fire Safety

1. **Install** smoke detectors
2. **Keep** fire extinguisher nearby
3. **Maintain** clearances around heater
4. **Monitor** for overheating
5. **Have** emergency shutdown procedure

### Animal Safety

1. **Secure** all wiring away from animals
2. **Protect** sensors from pecking
3. **Ensure** no exposed sharp edges
4. **Maintain** proper ventilation
5. **Monitor** for distress signs

### System Safety

1. **Test** emergency stop regularly
2. **Backup** configuration frequently
3. **Monitor** system logs
4. **Keep** firmware updated
5. **Document** all changes

---

## Support

### Getting Help

1. **Check this manual** for solutions
2. **Review serial monitor** output
3. **Check ESP-IDF documentation**
4. **Search online forums**
5. **Contact technical support**

### Useful Resources

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [FreeRTOS Documentation](https://www.freertos.org/)
- [ESP32 Forum](https://www.esp32.com/)
- [MQTT Documentation](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/)

### Reporting Issues

When reporting issues, include:
1. System version
2. Error messages from serial monitor
3. Steps to reproduce
4. Expected vs actual behavior
5. Configuration changes made

---

## Appendix

### A. Default Configuration Values

| Parameter | Default | Range | Unit |
|-----------|---------|-------|------|
| Temperature Setpoint | 26.0 | 20-32 | °C |
| Humidity Setpoint | 60.0 | 40-70 | % |
| Ammonia Limit | 25.0 | 10-50 | ppm |
| CO2 Limit | 3000.0 | 1000-5000 | ppm |
| Water Level Min | 20.0 | 10-50 | % |
| Feed Level Min | 15.0 | 5-30 | % |

### B. GPIO Pin Reference

| Function | GPIO | Type |
|----------|------|------|
| DHT22 | 4 | Digital |
| Ammonia | 36 | ADC |
| CO2 | 39 | ADC |
| Light | 34 | ADC |
| Water Level | 35 | ADC |
| Feed Level | 32 | ADC |
| Gas | 33 | ADC |
| Exhaust Fan | 16 | PWM |
| Inlet Fan | 17 | PWM |
| Heater | 18 | PWM |
| Feeder | 21 | PWM |
| Water Pump | 22 | Digital |
| Lighting | 23 | PWM |
| Alarm | 25 | Digital |
| Servo | 26 | PWM |

### C. Alert Codes

| Code | Alert Type |
|------|------------|
| 0 | None |
| 1 | Temperature High |
| 2 | Temperature Low |
| 3 | Humidity High |
| 4 | Humidity Low |
| 5 | Ammonia High |
| 6 | CO2 High |
| 7 | Water Low |
| 8 | Feed Low |
| 9 | Gas Leak |
| 10 | Sensor Failure |
| 11 | Actuator Failure |
| 12 | System Error |

### D. Serial Commands

| Command | Description |
|---------|-------------|
| `help` | Show available commands |
| `status` | Show system status |
| `sensors` | Show sensor readings |
| `actuators` | Show actuator states |
| `logs` | Show recent logs |
| `reset` | Restart system |
| `emergency_stop` | Emergency stop all |
| `mode <mode>` | Change operating mode |

---

**Document Version**: 1.0.0  
**Last Updated**: 2026-03-31  
**Author**: Smart Poultry System Team
