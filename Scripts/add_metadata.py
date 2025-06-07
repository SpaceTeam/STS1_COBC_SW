import sys
import pathlib
import struct
import zlib


if __name__ == "__main__":
    firmware_bin_file = pathlib.Path(sys.argv[1])
    firmware_size = firmware_bin_file.stat().st_size
    print(f"Adding metadata to '{firmware_bin_file}'")

    METADATA_SIZE = 0x200
    metadata = bytearray([0xFF] * METADATA_SIZE)
    image_size = METADATA_SIZE + firmware_size
    metadata[:4] = struct.pack('<I', image_size)    # Store image size in little endian

    with firmware_bin_file.open('rb') as f:
        firmware = f.read()
    full_image = metadata + firmware
    crc32 = 0xFFFF_FFFF - zlib.crc32(full_image)    # Compute CRC-32/JAMCRC
    full_image += struct.pack('<I', crc32)          # Append CRC-32 in little endian

    full_image_bin_file = firmware_bin_file.with_stem(firmware_bin_file.stem + "WithMetadata")
    with full_image_bin_file.open('wb') as f:
        f.write(full_image)
    full_image_size = len(full_image)
    print(f"  image size = {full_image_size:6d} B,  CRC-32 = {crc32:#010X}")
