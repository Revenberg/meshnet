# MeshNet ESP32 Node Firmware

Phase 3 implementation for distributed mesh network communication on Heltec ESP32 LoRa v3 boards.

## Features

- **LoRa Mesh Networking**: 868MHz EU ISM band communication
- **OLED Display**: Real-time status and message display (SSD1306)
- **Battery Management**: Voltage monitoring and percentage tracking
- **Hardware Button**: User interface for status/actions
- **Over-the-Air Updates**: Support for remote firmware updates
- **Protocol Compliance**: Full implementation of MeshNet protocol specification

## Hardware Requirements

- **Heltec ESP32 LoRa v3** board
- **USB-C** cable for programming and power
- **Li-ion Battery** (3.7V, 3000mAh recommended)

## Required Libraries

Install via Arduino IDE Library Manager:

1. **RadioLib** (v6.0.0+) - LoRa radio support
   - Provides SX1262 radio interface
   - Supports multiple frequency bands

2. **Heltec Unofficial** (v0.9.2+)
   - Official board support package
   - Includes display and button drivers
   - Already includes RadioLib, SSD1306 driver, HotButton

3. **ArduinoJson** (v6.20.0+) - Optional
   - JSON parsing for configuration
   - Used for protocol messages

4. **ESP32** Board Support Package (latest)

## Board Configuration

Select in Arduino IDE:
- **Board**: Heltec ESP32 LoRa v3 (from Heltec unofficial)
- **CPU Frequency**: 240MHz
- **Flash Size**: 8MB
- **Partition Scheme**: Huge APP (for OTA updates)
- **Port**: Auto-detect USB-C

## Build Configuration

### Compilation Settings

```
Board: ht_u_esp32_lora_v3
Variant: ht_u_esp32_lora_v3
Flash Freq: 80MHz
Flash Mode: DIO
CPU Freq: 240MHz
Upload Speed: 921600 baud
```

### Memory Configuration

- **Total Flash**: 8MB
- **App Partition**: ~3.3MB
- **SPIFFS**: ~1.5MB
- **OTA Storage**: ~3.3MB

## Protocol Implementation

### Message Types

1. **DISCOVER** (Type 1)
   - Sent at startup and every 5 minutes
   - Announces node to mesh network
   - Contains: nodeId, name, version, battery, uptime

2. **STATUS** (Type 2)
   - Sent every 10 seconds
   - Reports current node state
   - Contains: battery, message count, connectivity

3. **RELAY** (Type 3)
   - Multi-hop message forwarding
   - TTL/hop counting for loop prevention
   - Transparent message relaying

4. **CONFIG** (Type 4)
   - Remote configuration updates
   - Node name, LoRa settings
   - Applied immediately with NVS persistence

5. **DATA** (Type 5)
   - User application data
   - Game state, sensor readings, etc.

### Message Format

```
[TYPE][FROM][TO][MSG_ID][TTL][HOPS][DATA_LEN][DATA]
 1B    6B   6B    4B     1B   1B    1B      Variable
```

## Project Structure

```
mesh_node/
├── mesh_node.ino          # Main sketch
├── LoRaRadio.h/cpp        # LoRa communication
├── NodeDisplay.h/cpp      # OLED display management
├── BatteryManager.h/cpp   # Battery monitoring
├── ConfigManager.h/cpp    # Configuration storage
└── README.md              # This file
```

## Quick Start

1. **Clone/Copy** firmware files to Arduino sketches folder
2. **Connect** Heltec board via USB-C
3. **Select** Board: Heltec ESP32 LoRa v3
4. **Verify** → **Upload**
5. **Monitor** Serial output (Tools → Serial Monitor, 115200 baud)

### Expected Serial Output

```
[Setup] MeshNet Node Firmware v1.0.0
[NodeDisplay] ✓ Initialized
[BatteryManager] ✓ Initialized
[LoRaRadio] ✓ Initialized
[ConfigManager] Loaded: MeshNode
[Setup] ✓ All systems initialized
[LoRaRadio] ✓ TX: DISCOVER sent
```

## Configuration

### NVRAM Storage

Configuration persists in device NVRAM (Preferences):

- `nodeName`: Display name for node
- `nodeId`: MAC address (set at first boot)
- `loraFreq`: Radio frequency (default: 868MHz)
- `loraPower`: TX power 2-17dBm (default: 17dBm)
- `loraSF`: Spreading factor 6-12 (default: 9)

### Programmatic Configuration

```cpp
ConfigManager::init();
ConfigManager::setNodeName("kitchen_sensor");
ConfigManager::setLoRaPower(14);
ConfigManager::setLoRaSF(10);
ConfigManager::save();
```

## Display Interface

### Line Layout

```
Line 0: Msgs: 42
Line 1: ID: AABBCCDD
Line 2: LoRa Ready
Line 3: Status indicators
Line 4: Bat: 87%
Line 5: RSSI: -65dBm
```

### Button Operations

- **Short Press**: Send status message
- **Long Press**: (TBD - reset, config mode, etc.)

## Testing & Verification

### Unit Tests

```bash
# Serial Monitor output verification
# Watch for successful initialization messages
# Monitor battery voltage readings
# Observe LoRa transmit/receive activity
```

### Integration Tests

1. **Power On**: Verify display shows initialization
2. **LoRa Discovery**: Check DISCOVER message sent to mesh
3. **Battery Monitoring**: Confirm battery % displayed
4. **Button Interaction**: Short press triggers status send
5. **Message Reception**: Monitor incoming messages

## Troubleshooting

### Board Not Detected

- Verify USB drivers installed (CH340/CH341 for Heltec boards)
- Try different USB port
- Use USB 2.0 port (some USB 3.0 ports have issues)

### Upload Timeout

- Reduce Upload Speed to 115200
- Hold BOOT button during upload
- Check USB cable quality

### LoRa Not Initializing

- Verify RadioLib library installed correctly
- Check frequency setting (868E6 for EU)
- Confirm SX1262 radio module is populated

### Display Blank

- Verify SSD1306 driver library installed
- Check I2C address (0x3C default for Heltec)
- Try different brightness setting

## Performance Metrics

- **Startup Time**: ~2-3 seconds
- **LoRa Transmit**: 100-500ms (depends on spreading factor)
- **Message Processing**: <100ms
- **Battery Consumption**: ~100mA active, <10mA sleep
- **Range**: 1-5km (depends on terrain and SF)

## Future Enhancements

### Phase 3b: Advanced Features

- [ ] Mesh topology mapping
- [ ] Automatic routing
- [ ] Encryption (AES-128)
- [ ] Power saving modes
- [ ] Temperature compensation

### Phase 3c: Game Integration

- [ ] Player data sync
- [ ] Real-time game state updates
- [ ] Multi-player coordination
- [ ] Scoring system integration

### Phase 3d: Cloud Connectivity

- [ ] WiFi AP mode for configuration
- [ ] MQTT bridge integration
- [ ] Mobile app communication
- [ ] Firebase integration

## References

- [Heltec Unofficial Board Setup](https://github.com/ropg/heltec_esp32_lora_v3)
- [RadioLib Documentation](https://jgromes.github.io/RadioLib/)
- [MeshNet Protocol Spec](../protocol/PROTOCOL_SPEC.md)
- [SX1262 Datasheet](https://semtech.my.salesforce.com/sfc/p/E0000000JelG/a/2R0000000W6S/OWCTA5FLJFYqYXtNpP4pKwNdJV5XDPf_zCNVqJGcuHY)

## License

Part of MeshNet project. See LICENSE for details.

## Support

For issues or questions:
1. Check serial monitor output
2. Review Protocol Spec documentation
3. Test with simple example sketches
4. Check GitHub issues in MeshNet project
