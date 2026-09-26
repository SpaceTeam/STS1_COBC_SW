# RF Config Development

Master Settings file for Si4463.

This file contains Silabs WDS Tool Settings. These settings are used to generate configuration files for the Si4463 with the WDS Tool. The data from those files will be used in our own configuration.

## 1. Select project


"Standard packet TX" & "Standard packet RX"

> Jakob R.: we are actually using "Long packet TX" & "Long packet RX", but the packet handler is proven to work in our code.

> In the WDS Tool, the extra configuration of the packet field for long Packet is very long with many fields
> so we can just use standard packet in the WDS Tool and should ignore the Packet config values the WDS Tool spits out.



## TAB: Frequency and power

_Following configs are available for Direct TX & Direct RX_

### Frequency

- Base frequency [MHz]: 437.395
- Channel spacing [kHz]: 0.00
- Channel number: 0
- Center frequency (calculated/read-only) [MHz]: 437.395

### Crystal

- Crystal Frequency [MHz]: 26.0
- Crystal Cap. bank: 0x0
- Crystal tolerance RX [ppm]: 2.0
- Crystal tolerance TX [ppm]: 2.0
- Use external TCXO/Ref Source [y/n]: YES

### Clock

- Enable divided system clock output [y/n]: NO (Jakob R. NO with high certainty)
- Clock output (grayed out): system clock div
- 32 kHz clock: disabled

----
_Following configs are available ONLY for Direct TX_

### Power amplifier (PA)

- PA mode: ClassE/Square W
- PA Ramp TC: 31
- PA bias (grayed out): 0x0
- PA power level: 0x2F
- Enable ramp control of External PA [y/n]: NO
- Ramping time (grayed out): 0xF
- Ramping time [µs] (calculated, grayed out): 1.18us

## TAB: RF parameters

_Following configs are available for Direct TX & Direct RX_

### Modulation

- Modulation type: 2GFSK
- Data rate [kbps(=ksps)]: dataRateWeWant
- Deviation [kHz]: dataRateWeWant * 0.25

_Following configs are available ONLY for Direct RX_

- RX bandwidth [kHz]: auto calc
- RX bandwidth checkbox [y/n]: NO
- RX data rate error: 0% - 1%

### Optimize RX performance

Optimize RX performance:

- [ ] Low current consumption
- [X] High sensitivity
- [ ] Improved blocking
- [ ] Improved selectivity

- Enable PPL AFC [y/n]: YES (TODO: Try if it is better or worse on flatsat)
- Enable adaptive Ch. Fil. BW. [y/n]: YES (TODO: Try if it is better or worse on flatsat)
- Enable antenna diversity [y/n]: NO
- Enable IQ calibration [y/n]: NO (TODO: should be done for every cubesat and the values you get from that should be hardcoded for the speciffic sat)

### RSSI

- RSSI average: RSSI averaged over 4 (Jakob R. says it absolutely doesn't matter)
- RSSI latch: Disabled
- Check threshold at latch [y/n]: NO
- RSSI threshold: 0xFF

## TAB: GPIO and FRR

_Following configs are available for Direct TX & Direct RX_

### GPIO

**GPIO 0:**

- Enable pullup [y/n]: yes
- Pin configuration: TRISTATE - Input and output drivers disabled

**GPIO 1:**

- Enable pullup [y/n]: yes
- Pin configuration: TRISTATE - Input and output drivers disabled

**GPIO 2:**

- Enable pullup [y/n]: no
- Pin configuration: RX_STATE - This output is set hight while in RX state and low otherwise

**GPIO 3:**

- Enable pullup [y/n]: no
- Pin configuration: TX_STATE - This output is set hight while in TX state and low otherwise

**NIRQ:**

- Enable pullup [y/n]: yes
- Pin configuration: Active low interrupt signal

**SDO:**

- Enable pullup [y/n]: yes
- Pin configuration: SDO - Output SPI Serial data out

- Drive strength: highest strength

### Fast Response Registers

- Fast Response Register A: Disabled
- Fast Response Register B: Disabled
- Fast Response Register C: Disabled
- Fast Response Register D: Disabled
