#pragma once
/**
 * OSCSender – minimal OSC-over-UDP sender with no external dependencies.
 *
 * Sends messages matching the pattern:
 *   /ble/rssi  <deviceName:string>  <rssi:int32>
 *
 * Compliant with the OSC 1.0 wire format (big-endian, 4-byte-aligned strings).
 */

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "Ws2_32.lib")
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
#endif

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

class OSCSender
{
public:
    OSCSender() = default;

    ~OSCSender() { close(); }

    /** Open a UDP socket targeting host:port. Returns true on success. */
    bool open (const std::string& host, int port)
    {
#ifdef _WIN32
        WSADATA wsa{};
        WSAStartup (MAKEWORD (2, 2), &wsa);
        sock_ = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock_ == INVALID_SOCKET) return false;
#else
        sock_ = ::socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock_ < 0) return false;
#endif
        std::memset (&addr_, 0, sizeof (addr_));
        addr_.sin_family = AF_INET;
        addr_.sin_port   = htons (static_cast<uint16_t> (port));
        inet_pton (AF_INET, host.c_str(), &addr_.sin_addr);
        return true;
    }

    void close()
    {
#ifdef _WIN32
        if (sock_ != INVALID_SOCKET) { closesocket (sock_); sock_ = INVALID_SOCKET; }
#else
        if (sock_ >= 0) { ::close (sock_); sock_ = -1; }
#endif
    }

    /** Send  /ble/rssi  <deviceName>  <rssi>  as a UDP OSC packet. */
    void sendBLERSSI (const std::string& deviceName, int32_t rssi) const
    {
        std::vector<uint8_t> pkt;
        pkt.reserve (64);

        appendString (pkt, "/ble/rssi");
        appendString (pkt, ",si");
        appendString (pkt, deviceName);
        appendInt32  (pkt, rssi);

        ::sendto (sock_,
                  reinterpret_cast<const char*> (pkt.data()),
                  static_cast<int> (pkt.size()),
                  0,
                  reinterpret_cast<const sockaddr*> (&addr_),
                  sizeof (addr_));
    }

private:
    /** Append a null-terminated, zero-padded OSC string (4-byte aligned). */
    static void appendString (std::vector<uint8_t>& v, const std::string& s)
    {
        for (char c : s) v.push_back (static_cast<uint8_t> (c));
        v.push_back (0); // null terminator
        while (v.size() % 4 != 0) v.push_back (0);
    }

    /** Append a big-endian 32-bit signed integer. */
    static void appendInt32 (std::vector<uint8_t>& v, int32_t val)
    {
        const auto n = htonl (static_cast<uint32_t> (val));
        v.push_back ((n >> 24) & 0xFF);
        v.push_back ((n >> 16) & 0xFF);
        v.push_back ((n >>  8) & 0xFF);
        v.push_back ( n        & 0xFF);
    }

#ifdef _WIN32
    SOCKET sock_ { INVALID_SOCKET };
#else
    int sock_ { -1 };
#endif

    struct sockaddr_in addr_ {};
};
