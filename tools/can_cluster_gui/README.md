# CAN Cluster Control Center

Unified Qt 6 Quick GUI for the CAN gateway / UDS cluster simulation project.

This app intentionally follows the visual direction of `firmware/board_b_gateway/GUI/boongboong`:

- dark dashboard background `#101418`
- gateway-style panels `#181f26` / `#202831`
- thin borders `#2a343f`
- accent blue `#3ba7ff`
- status colors: green `#28c76f`, warning `#ffb020`, red `#ea5455`
- large metric cards, status pills, latest CAN frame byte cells, and cockpit-style gauges

## Pages

- **Dashboard**: combined system view for Engine, Body, Gateway, UDS, cluster output and latest CAN frame
- **Engine ECU**: Board A serial connection, throttle/brake controls, ADC/UART mode, RPM/speed/coolant telemetry
- **Body ECU**: Board D serial connection, door/signal/light controls, vehicle body preview
- **Gateway**: Board B serial connection, CAN1/CAN2 counters, gateway health, 0x390 body decode, CAN log filter
- **UDS Tester**: Board C serial connection, DID buttons, clear DTC, raw CLI
- **CAN Log**: unified serial/CAN/diagnostic event stream

## Build

```bash
cmake -S tools/can_cluster_gui -B build/can_cluster_gui
cmake --build build/can_cluster_gui --config Release
```

Qt requirements:

- Qt 6
- Qt Quick
- Qt Quick Controls 2
- Qt SerialPort

## Serial command expectations

### Board A Engine

The Engine page assumes the existing Board A CLI:

```text
mode adc
mode uart
pedal <throttle> <brake>
stop
sim_reset
monitor on <interval_ms>
monitor off
monitor once
status
```

The parser accepts key/value monitor lines such as:

```text
MON mode=uart throttle=50 brake=0 rpm=3200 speed=42 coolant=88 tx=1234
```

### Board B Gateway

The Gateway page accepts the same style of UART monitor/log lines used by `boongboong`, including gateway status and `[RX1]`, `[TX1]`, `[RX2]`, `[TX2]` CAN frame lines.

The GUI sends gateway log commands such as:

```text
canlog all
canlog id 100
canlog on
canlog off
log on
log off
```

### Board C UDS Tester

The UDS page sends the existing Board C CLI commands:

```text
read vin
read rpm
read speed
read temp
read adas
read front
read rear
read fault
clear dtc
```

### Board D Body

The Body page is implemented for direct UART control, but current Board D firmware may still be GPIO/DIP-input oriented. To make the Body page fully functional, add a UART CLI to Board D firmware using commands like:

```text
body mode gpio
body mode uart
body ign 1
body door fl 1
body door fr 0
body door rl 0
body door rr 0
body turn left 1
body turn right 0
body high 1
body fog 0
body status
body monitor on 200
body monitor off
```

Suggested monitor line:

```text
BODY mode=uart ign=1 fl=1 fr=0 rl=0 rr=0 left=1 right=0 high=1 fog=0 tx=1234
```

### Layout note

The QML pages use scrollable canvases and extra panel height padding so Qt Creator on Windows does not crop controls when font metrics differ slightly from Linux/macOS. If you add new controls inside `Panel`, size the panel for the content area, not the outer rectangle: the panel header consumes about 62 px before child content starts.
