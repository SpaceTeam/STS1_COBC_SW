#include <cppserdes/serdes.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
struct MyPacket : serdes::packet_base
{
    int8_t x = 3;
    int8_t y = 17;
    int16_t z = 256;

    // Define a format (used for BOTH serialization and deserialization)
    void format(serdes::packet & serdesProcess) override
    {
        serdesProcess + x + y + z;
    }
};


class SerializeThread : public RODOS::StaticThread<>
{
    void run() override
    {
        uint16_t serialData[] = {0x0102, 0x7B00};

        MyPacket obj1;
        obj1.load(serialData);

        // MyPacket obj2;
        // obj2.store(serialData);
        
        RODOS::PRINTF("%i", obj1.x);
        RODOS::PRINTF("%i", obj1.y);
        RODOS::PRINTF("%i", obj1.z);
        // int8_t x = 3;
        // int16_t y = 256;
        // int8_t z = 17;

        // // auto buffer = std::array<uint8_t, 6>{};
        // uint8_t buffer[6] = {};
        // serdes::packet(buffer) << x << y << z;
        // x = 0;
        // y = 0;
        // z = 0;
        // serdes::packet(buffer) >> x >> y >> z;
    }
};


auto const serializeThread = SerializeThread();
}