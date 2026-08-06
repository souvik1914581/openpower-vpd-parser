# IPMI VPD Format
## Platform Management FRU Information Storage Definition
### v1.0, Rev 1.3 | Intel | Hewlett-Packard | NEC | Dell | 2015

Sources: IPMI FRU Info Storage Def v1.0 Rev 1.3 (2015) | Samsung NVMe E3.S Datasheet Analysis (PM9D3a / PM1753)

---

## What Is IPMI FRU VPD?

Vital Product Data (VPD) is machine-readable inventory information stored on every Field Replaceable Unit (FRU) — the minimal identity record needed to identify, track, and manage any replaceable hardware component.

The IPMI specification defines a binary storage format for this data in an on-board EEPROM, divided into six logical areas:

| Area               | Mandatory? | Purpose                                                                            |
|--------------------|------------|------------------------------------------------------------------------------------|
| Common Header      | Yes        | 8-byte header: format version + offsets to each area + checksum                   |
| Internal Use Area  | No         | Private non-volatile storage for the management controller (BMC)                  |
| Chassis Info Area  | No         | Chassis serial number, part number, chassis type — one per system                 |
| Board Info Area    | No         | Board manufacturer, product name, serial number, part number, mfg date            |
| Product Info Area  | No         | Product manufacturer, name, part/model, version, serial, asset tag                |
| MultiRecord Area   | No         | Extensible chain of typed records (power supply, DC output, NVMe ...)             |

Goal: every FRU ships with a software-readable EEPROM containing at minimum its part number and serial number — readable without an OS or device driver.

---

## Memory Layout & Common Header

The Common Header (always at EEPROM offset 0x00) is 8 bytes and is the single entry point for all FRU data. Every area offset is expressed in multiples of 8 bytes (0x00 = area absent).

### Common Header Byte Map

| Byte | Field                  | Description                                              |
|------|------------------------|----------------------------------------------------------|
| 0    | Format Version         | Bits 3:0 = 0x1 (this spec); bits 7:4 reserved           |
| 1    | Internal Use Offset    | Starting offset / 8  (0x00 = not present)               |
| 2    | Chassis Info Offset    | Starting offset / 8  (0x00 = not present)               |
| 3    | Board Info Offset      | Starting offset / 8  (0x00 = not present)               |
| 4    | Product Info Offset    | Starting offset / 8  (0x00 = not present)               |
| 5    | MultiRecord Offset     | Starting offset / 8  (0x00 = not present)               |
| 6    | PAD                    | Write as 0x00                                            |
| 7    | Header Checksum        | 2's-complement mod-256 zero checksum                     |

### Suggested 2K-bit EEPROM Layout

| Size      | Area               |
|-----------|--------------------|
| 8 bytes   | Common Header      |
| 72 bytes  | Internal Use Area  |
| 32 bytes  | Chassis Info Area  |
| 64 bytes  | Board Info Area    |
| >= 80 bytes | Product Info Area |

- Max addressable area start: 255 x 8 = 2,040 bytes
- Areas always appear in order: Header -> Internal Use -> Chassis -> Board -> Product -> MultiRecord
- Each area carries its own 2's-complement mod-256 zero checksum in its final byte. An application must verify the checksum before using any field in that area.

---

## Information Areas: Chassis / Board / Product

Each info area begins with a format version byte and a length byte (in multiples of 8 bytes), followed by predefined fields, optional custom OEM fields, the C1h end-of-fields sentinel, padding, and a checksum.

### Predefined Fields per Area

| Field                     | Chassis Info | Board Info                      | Product Info                  |
|---------------------------|--------------|---------------------------------|-------------------------------|
| Format Version            | Yes          | Yes                             | Yes                           |
| Area Length (x8 bytes)    | Yes          | Yes                             | Yes                           |
| Language Code             | --           | Yes                             | Yes                           |
| Mfg. Date / Time          | --           | Yes (3 bytes, minutes from 1/1/96) | --                         |
| Manufacturer Name         | --           | Yes                             | Yes                           |
| Product / Board Name      | --           | Yes                             | Yes                           |
| Serial Number *           | Yes          | Yes                             | Yes                           |
| Part / Model Number       | Yes          | Yes                             | Yes                           |
| Asset Tag / FRU File ID   | --           | FRU File ID                     | Asset Tag + FRU File ID       |

* Serial Number and Chassis Serial Number are always encoded as 8-bit ASCII (Language Code is ignored for these fields).

- Absent predefined fields use a null placeholder: type/length byte = 0x00.
- Custom OEM fields follow all predefined fields; each must be preceded by a type/length byte.
- The sentinel byte 0xC1 marks end-of-fields.

---

## Type/Length Byte Encoding

All variable-length fields in every area are preceded by a single Type/Length byte. Bits 7:6 specify encoding; bits 5:0 specify the byte count.

    [ 7  6 | 5  4  3  2  1  0 ]
      type    data length (0-63 bytes)

### Encoding Types

| Bits 7:6 | Encoding              | Details                                                                                              |
|----------|-----------------------|------------------------------------------------------------------------------------------------------|
| 00       | Binary / unspecified  | Raw bytes; application defines interpretation                                                        |
| 01       | BCD plus              | Nibbles: 0-9 = digits; A = space, B = dash, C = period, D-F = reserved                             |
| 10       | 6-bit ASCII (packed)  | 64 printable chars from ASCII 0x20; 4 chars packed into 3 bytes                                     |
| 11       | 8-bit ASCII / Unicode | English Language Code -> ASCII+Latin-1 (U+0000-U+00FF); other languages -> 2-byte Unicode LE       |

### 6-bit ASCII Packing Example — "IPMI" encoded into 3 bytes

| Char | 6-bit value | Hex | Binary  | Note                               |
|------|-------------|-----|---------|------------------------------------|
| I    | 101001b     | 29h | 10 1001 | I = ASCII 0x49 -> 0x49 - 0x20 = 0x29 |
| P    | 110000b     | 30h | 11 0000 | P = ASCII 0x50 -> 0x50 - 0x20 = 0x30 |
| M    | 101101b     | 2Dh | 10 1101 | M = ASCII 0x4D -> 0x4D - 0x20 = 0x2D |
| I    | 101001b     | 29h | 10 1001 | Packed result: 0x29  0xDC  0xA6    |

- Type/Length = 0xC1 (type=11b, length=1) means end-of-fields.
- Type/Length = 0x00 (any type, length=0) means null/empty field.

---

## MultiRecord Area: Extensibility

The MultiRecord Area is a forward-linked chain of typed records. Each record has its own header identifying type, length, and checksums — allowing new record types to be added without changing existing area definitions.

### 5-Byte Record Header (per record)

| Byte(s) | Field                                                          |
|---------|----------------------------------------------------------------|
| 0       | Record Type ID (see table below)                              |
| 1       | Bit 7: End-of-List flag  |  Bits 3:0: Record Format Version (= 2h) |
| 2       | Record Length (0-255 bytes of data follow)                    |
| 3       | Record Checksum (zero checksum over record data)              |
| 4       | Header Checksum (zero checksum over bytes 0-3)                |

### Standard Record Types

| Type ID    | Record Type                                          |
|------------|------------------------------------------------------|
| 0x00       | Power Supply Information                             |
| 0x01       | DC Output                                            |
| 0x02       | DC Load                                              |
| 0x03       | Management Access Record                             |
| 0x04       | Base Compatibility Record                            |
| 0x05       | Extended Compatibility Record                        |
| 0x06-0x08  | Reserved (ASF)                                       |
| 0x09       | Extended DC Output                                   |
| 0x0A       | Extended DC Load                                     |
| 0x0B-0x0F  | Reserved -- NVM Express working group (added Rev 1.3, 2015) |
| 0xC0-0xFF  | OEM Record Types (vendor-defined)                    |

- Rev 1.3 (2015) reserved record types 0x0B-0x0F for the NVM Express working group, allowing NVMe-specific records (form factor, PCIe port info, power requirements) to co-exist with the standard IPMI FRU layout.
- OEM range 0xC0-0xFF provides a proprietary escape hatch; parsers that encounter unknown type IDs must skip them using the Record Length field.

---

## Access Protocols

FRU Inventory Devices can be accessed via two paths. BMC-mediated access is the preferred approach for production systems.

### Path 1: Direct SEEPROM (I2C / SMBus)

- Compatible device types: 24C02-style I2C SEEPROM interface, Dallas DS1624 Temperature Sensor/SEEPROM
- Software must know the specific I2C bus address and register layout of the storage device
- No error handling beyond what the device provides
- Manufacturing can program via direct pin access (circuit test fixture) without a host OS

### Path 2: BMC-Mediated (Preferred)

- Management controller exposes IPMI Read/Write FRU Inventory Data commands
- Software uses identical commands regardless of underlying EEPROM type (hardware abstraction)
- BMC provides additional data integrity checking and error handling not available via direct access
- Isolates host software from EEPROM bus topology

### Application Access Flow (applies to both paths)

1. Read Common Header at offset 0x00 -- verify format version = 0x01
2. Validate Common Header checksum (2's-complement mod-256 = 0)
3. Extract starting offset for the desired area from Common Header bytes 1-5
4. If offset = 0x00, the area is not present -- skip
5. Read the area header -- verify area format version and area checksum
6. Walk fields sequentially using type/length bytes until 0xC1 end-of-fields
7. Display predefined fields by name; custom fields with generic labels (Custom Field N)
8. For MultiRecord: read record header, use Record Length to advance to next record; stop when End-of-List bit = 1

---

## Advantages & Disadvantages

### Advantages

- Vendor-neutral open standard (Intel, HP, NEC, Dell -- 1998 to present)
- Out-of-band access via I2C/SMBus -- no OS or PCIe link required
- Per-area and per-record checksummed integrity (2's-complement mod-256)
- Extensible via MultiRecord standard types + OEM record range (0xC0-0xFF)
- Multiple encoding types: binary, BCD+, 6-bit ASCII, 8-bit ASCII/Latin-1, Unicode
- Industry-wide ecosystem support: BMC firmware, OpenBMC, IPMI tools, NVMe MI working group

### Disadvantages

- Limited address space: 8-bit offset addressing gives a 256-byte ceiling on direct-access single-EEPROM devices
- Write-cycle exhaustion is permanent -- no in-spec recovery mechanism
- Shared SMBus contention: all bus devices must be serialised (VPD, temperature sensor, etc.)
- No per-field versioning -- any field update requires a full area rewrite
- Binary encoding is opaque: EEPROM content requires a dedicated parser to interpret
- OEM/MultiRecord extensibility can fragment cross-vendor interoperability if parsers skip unknown records

---

## Real-World Usage: NVMe E3.S Drives

Samsung NVMe E3.S drives (PM9D3a / PM1753) store VPD in an IPMI FRU-compatible EEPROM on their SMBus interface -- a direct implementation of this specification.

### E3.S EEPROM Memory Map (256 bytes, SMBus address 0xA6)

| EEPROM Offset  | Size      | Area / Content                                                          |
|----------------|-----------|-------------------------------------------------------------------------|
| 0x00 - 0x07    | 8 bytes   | Common Header (IPMI format v1, area offsets, checksum)                  |
| 0x08 - 0x77    | 112 bytes | Product Info Area (Mfg Name, Product Name, Part No., S/N, Asset Tag)   |
| 0x78 - 0xB7    | 64 bytes  | NVMe MultiRecord (form factor 0x55 = E3.S, 3.3V / 12V power requirements) |
| 0xB8 - 0xCF    | 24 bytes  | NVMe PCIe Port MultiRecord (port number, link width, speed, power negotiation) |

### Three VPD Collection Paths

1. SMBus (out-of-band): Address 0xA6, 8-bit offset, no PCIe link needed.
   Captures: Manufacturer, Product Name, Part Number, Serial Number.

2. NVMe Admin Identify (in-band): PCIe Gen.4/5, CSTS.RDY = 1 required.
   Captures: Serial Number, Model Number, Firmware Revision, Subsystem Vendor ID.

3. NVMe-MI (MCTP over SMBus): VPD read/write and subsystem management without a full NVMe driver.

### Critical Checks

| Check                        | PM9D3a (WW General)                                             | PM1753 (IBM)                                          |
|------------------------------|-----------------------------------------------------------------|-------------------------------------------------------|
| Serial Number cross-validation | EEPROM PSN bytes 89-109 must equal NVMe Identify SN bytes 23:4 | Same requirement -- mismatch means corrupted EEPROM   |
| VPD Write Cycle Info (VWCI)  | VWCI = 0x00 -- Exhausted, EEPROM field update impossible        | VWCI = 0xFF -- 255 write cycles remaining             |
| Subsystem Vendor ID          | 0x144D (Samsung)                                                | 0x1014 (IBM) -- parsers must handle both              |

---

## Summary & Key Takeaways

IPMI FRU VPD is an open, EEPROM-based binary inventory format standardised by Intel / HP / NEC / Dell in 1998 and actively extended through 2015 (Rev 1.3) -- still the mandated format for NVMe drives via NVMe MI Rev 1.2+.

### 6 Implementation Rules (from NVMe E3.S field experience)

| # | Rule                          | Detail                                                                                                                                  |
|---|-------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------|
| 1 | Validate checksums first      | Verify Common Header checksum and every area/record checksum before reading any field. A single corrupt bit invalidates the entire area. |
| 2 | Check VWCI before EEPROM write | Read VWCI (NVMe Identify byte 254) before writing to the EEPROM. If VWCI = 0x00, abort -- write cycles are permanently exhausted.      |
| 3 | Serialise all SMBus accesses  | Never poll the temperature sensor or any other SMBus device concurrently with a VPD read/write. Use a bus-level lock.                   |
| 4 | Cross-validate serial number  | EEPROM Product Info Area PSN (bytes 89-109) must equal NVMe Identify Controller SN (bytes 23:4). Mismatch = corrupted identity.         |
| 5 | Apply vendor-specific rules   | IBM drives use Subsystem Vendor ID 0x1014 and IBM-formatted model strings. Standard Samsung parsers will fail without OEM rules.        |
| 6 | Respect PCIe startup delays   | Poll CSTS.RDY after power-on. Allow up to 80 s for 30.72 TB drives after a sudden power-off/recovery event before in-band commands.    |

Spec first published 1998. NVMe MI working group reserved MultiRecord range 0x0B-0x0F in Rev 1.3 (2015), confirming continued relevance in modern storage.
