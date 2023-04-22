# Connector pinning 
1. ???
2. 24VDC
3. GND
4. ???
5. RS485, DATA-
6. RS485, DATA+

# Communication basics
* RS485 communication
* 19200Baud
* 8 databits
* No parity
* 1 Stop bit


# Typical message flow

## Startup
After Powerup the master sends the **Broadcast status** and the **Slave scan** messages alternating. The slave address used for the **Slave scan** is decremented each time a new message is send. The repeatedly used slave address range is `0x90` down to `0x10`.

## Slave detection
If a slave answers to its **Slave scan** message with **Slave scan response** the master detects the slave an starts asking the slave with **Slave status request** messages.

## Normal operation
The master sends the **Broadcast status** and the **Slave status request** messages alternating. The slave has to respond with **Slave status response**. If the slave doesn't respond the master will show error **7** and the door will not move anymore.

# Messages

## Message structure
| 0          | 1       | 2              | 3  |     | length+2 | length+3 |
|------------|---------|----------------|----|-----|----------|----------|
| Sync break | address | counter+length | d0 | ... | dn       | crc      |

* `counter` are the upper 4 bits of byte 2. The slave has to answer with `cnt+1` when he received `cnt` as counter from the master.
* `length` are the lower 4 bits of byte 2. Length is the number of data bytes `d0`up to `dn`.

## CRC calculation
* 8 bit CRC
* Polynomial: `0x07`
* Initial value: `0xF3`
* The CRC is calculated over all bytes starting with the address byte and ending with the last data byte

## Known addresses
* `0x00`: Broadcast
* `0x80`: Master (door drive)
* `0x28`: UAP1

## Known commands
* `0x01`: Slave scan
* `0x20`: Slave status request
* `0x29`: Slave status response

## Known messages

### Broadcast status
Sender : Master

| 0          | 1              | 2    | 3  | 4  | 5   |
|------------|----------------|----- |----|----|-----|
| Sync break | Broadcast addr | 0x?2 | d0 | d1 | crc |

* d0 bits:
    * `0`: door open
    * `1`: door closed
    * `2`: external relay supposed to be switched on (relay 03 on UAP1)
    * `3`: light relay (not 100% sure)
    * `4`: error active
    * `5`: direction when moving (=1 closing, =0 opening)
    * `6`: door is moving
    * `7`: door is in venting position (**H** on display)
* d1 bits:
    * `0`: pre-warning active
    * `1`: ??? (seems to be always 1)
    * `2`: ???
    * `3`: ???
    * `4`: ???
    * `5`: ???
    * `6`: ???
    * `7`: ???

### Slave scan
Sender: Master

| 0          | 1          | 2    | 3   | 4      | 5   |
|------------|------------|----- |-----|--------|-----|
| Sync break | Slave addr | 0x?2 | cmd | sender | crc |

* cmd = `0x01`: Slave scan
* sender = `0x80`: Master address

### Slave scan response
Sender: Slave

| 0          | 1           | 2    | 3    | 4    | 5   |
|------------|-------------|----- |------|------|-----|
| Sync break | Master addr | 0x?2 | type | addr | crc |

* type = `0x14`: UAP1 type
* addr = `0x28`: UAP1 address

### Slave status request
Sender: Master

| 0          | 1          | 2    | 3   | 4   |
|------------|------------|----- |-----|-----|
| Sync break | Slave addr | 0x?1 | cmd | crc |

* cmd = `0x20`: Slave status request

### Slave status response
Sender: Slave

| 0          | 1           | 2    | 3   | 4  | 5  | 6   |
|------------|-------------|----- |-----|----|----|-----|
| Sync break | Master addr | 0x?3 | cmd | d0 | d1 | crc |

* cmd = `0x29`: Slave status response
* d0 bits:
    * `0`: Trigger open door
    * `1`: Trigger close door
    * `2`: Trigger reverse direction (not 100% sure)
    * `3`: Toggle light relais
    * `4`: Trigger venting position
    * `5`: ???
    * `6`: ???
    * `7`: ???
* d1 bits:
    * `0`: ???
    * `1`: ???
    * `2`: ???
    * `3`: ???
    * `4`: State of pin `S0` on UAP1, i.e. the emergency cut-off button (normally `1`; if this bit is set to `0` the drive stops and error **4** is shown on the display)
    * `5`: ???
    * `6`: ???
    * `7`: ???

## Examples

### Broadcast status
| 0  | 1    | 2    | 3    | 4    | 5    |
|----|------|------|------|------|------|
|    | 0x00 | 0x52 | 0x01 | 0x02 | 0xD0 |

* `0x00`: Broadcast
* `0x52`: ((0x52 & 0xF0) >> 4) = 5 = counter , (0x52 & 0x0F) = 2 = length
* `0x01`: d0, door open
* `0x02`: d1, ???
* `0xD0`: CRC

### Slave status response
| 0  | 1    | 2    | 3    | 4    | 5    | 6    |
|----|------|------|------|------|------|------|
|    | 0x80 | 0x63 | 0x29 | 0x01 | 0x10 | 0x4B |

* `0x80`: Master
* `0x63`: ((0x63 & 0xF0) >> 4) = 6 = counter , (0x63 & 0x0F) = 3 = length
* `0x29`: Slave status response 
* `0x01`: d0, trigger door open
* `0x10`: d1, no emergency cut-off
* `0x4B`: CRC
