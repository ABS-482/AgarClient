#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

// Аналог DataView из JS: последовательное чтение байт с ручным
// продвижением offset, с проверкой выхода за границы буфера.
class PacketReader
{
public:
    PacketReader(const uint8_t* data, size_t size)
        : m_data(data), m_size(size)
    {
    }

    size_t offset() const { return m_offset; }
    size_t remaining() const { return m_size - m_offset; }
    bool hasMore() const { return m_offset < m_size; }

    void skip(size_t bytes)
    {
        require(bytes);
        m_offset += bytes;
    }

    int8_t readInt8()
    {
        return static_cast<int8_t>(readUint8());
    }

    int16_t readInt16LE()
    {
        return static_cast<int16_t>(readUint16LE());
    }

    int32_t readInt32LE()
    {
        return static_cast<int32_t>(readUint32LE());
    }

    uint8_t readUint8()
    {
        require(1);
        return m_data[m_offset++];
    }

    // little-endian, как getUint16(offset, true) в JS
    uint16_t readUint16LE()
    {
        require(2);
        uint16_t value =
            static_cast<uint16_t>(m_data[m_offset]) |
            (static_cast<uint16_t>(m_data[m_offset + 1]) << 8);
        m_offset += 2;
        return value;
    }

    uint32_t readUint32LE()
    {
        require(4);
        uint32_t value =
            static_cast<uint32_t>(m_data[m_offset]) |
            (static_cast<uint32_t>(m_data[m_offset + 1]) << 8) |
            (static_cast<uint32_t>(m_data[m_offset + 2]) << 16) |
            (static_cast<uint32_t>(m_data[m_offset + 3]) << 24);
        m_offset += 4;
        return value;
    }

    float readFloat32LE()
    {
        uint32_t bits = readUint32LE();
        float value;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    uint64_t readUint64LE()
    {
        require(8);
        uint64_t value = 0;

        for (int i = 0; i < 8; ++i)
        {
            value |= static_cast<uint64_t>(m_data[m_offset + i]) << (8 * i);
        }

        m_offset += 8;
        return value;
    }

    double readFloat64LE()
    {
        uint64_t bits = readUint64LE();
        double value;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    // Аналог encode() из вашего JS-парсера:
    // null-terminated строка, 2 байта (UTF-16LE) на символ.
    std::string readUtf16String()
    {
        std::string result;

        for (;;)
        {
            uint16_t code = readUint16LE();

            if (code == 0)
                break;

            // Упрощение: считаем, что коды укладываются в ASCII/latin-1.
            // Если встретите ники с не-ASCII символами — вернёмся
            // и сделаем нормальную UTF-16 -> UTF-8 конвертацию.
            result += static_cast<char>(code);
        }

        return result;
    }

private:
    void require(size_t bytes) const
    {
        if (m_offset + bytes > m_size)
        {
            throw std::out_of_range(
                "PacketReader: attempt to read past end of buffer"
            );
        }
    }

    const uint8_t* m_data;
    size_t m_size;
    size_t m_offset = 0;
};