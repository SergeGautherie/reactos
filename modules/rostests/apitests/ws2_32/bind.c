/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for bind
 * PROGRAMMER:      Peter Hater
 */

#include "ws2_32.h"

#define PORT 58888

static
VOID
TestBind(IN_ADDR Address)
{
    const UCHAR b1 = Address.S_un.S_un_b.s_b1;
    const UCHAR b2 = Address.S_un.S_un_b.s_b2;
    const UCHAR b3 = Address.S_un.S_un_b.s_b3;
    const UCHAR b4 = Address.S_un.S_un_b.s_b4;

    int Error;
    struct
    {
        INT Type;
        INT Proto;
        struct sockaddr_in Addr;
        INT ExpectedResult;
        INT ExpectedWSAResult;
        struct sockaddr_in ExpectedAddr;
    } Tests[] =
    {
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, PORT, {{{ 0x7f, 0x00, 0x00, 0x01 }}} }, 0, 0, { AF_INET, PORT, {{{ 0x7f, 0x00, 0x00, 0x01 }}} } },
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, PORT, {{{ 0x00, 0x00, 0x00, 0x00 }}} }, 0, 0, { AF_INET, PORT, {{{ 0x00, 0x00, 0x00, 0x00 }}} } },
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, PORT, {{{ b1, b2, b3, b4 }}} }, 0, 0, { AF_INET, PORT, {{{ b1, b2, b3, b4 }}} } },
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, PORT, {{{ 0xff, 0xff, 0xff, 0xff }}} }, SOCKET_ERROR, WSAEADDRNOTAVAIL },
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, 0, {{{ 0x7f, 0x00, 0x00, 0x01 }}} }, 0, 0, { AF_INET, 0, {{{ 0x7f, 0x00, 0x00, 0x01 }}} } },
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, 0, {{{ 0x00, 0x00, 0x00, 0x00 }}} } },
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, 0, {{{ b1, b2, b3, b4 }}} }, 0, 0, { AF_INET, 0, {{{ b1, b2, b3, b4 }}} } },
        { SOCK_STREAM, IPPROTO_TCP, { AF_INET, 0, {{{ 0xff, 0xff, 0xff, 0xff }}} }, SOCKET_ERROR, WSAEADDRNOTAVAIL },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, PORT, {{{ 0x7f, 0x00, 0x00, 0x01 }}} }, 0, 0, { AF_INET, PORT, {{{ 0x7f, 0x00, 0x00, 0x01 }}} } },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, PORT, {{{ 0x00, 0x00, 0x00, 0x00 }}} }, 0, 0, { AF_INET, PORT, {{{ 0x00, 0x00, 0x00, 0x00 }}} } },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, PORT, {{{ b1, b2, b3, b4 }}} }, 0, 0, { AF_INET, PORT, {{{ b1, b2, b3, b4 }}} } },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, PORT, {{{ 0xff, 0xff, 0xff, 0xff }}} }, SOCKET_ERROR, WSAEADDRNOTAVAIL },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, 0, {{{ 0x7f, 0x00, 0x00, 0x01 }}} }, 0, 0, { AF_INET, 0, {{{ 0x7f, 0x00, 0x00, 0x01 }}} } },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, 0, {{{ 0x00, 0x00, 0x00, 0x00 }}} } },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, 0, {{{ b1, b2, b3, b4 }}} }, 0, 0,{ AF_INET, 0, {{{ b1, b2, b3, b4 }}} } },
        { SOCK_DGRAM, IPPROTO_UDP, { AF_INET, 0, {{{ 0xff, 0xff, 0xff, 0xff }}} }, SOCKET_ERROR, WSAEADDRNOTAVAIL },
    };
    const INT TestCount = _countof(Tests);
    INT i, AddrSize;
    SOCKET Socket;
    struct sockaddr_in Addr;
    // 49152-65535 is IANA dynamic port range, for both UDP and TCP.
    USHORT min_port = 65535, max_port = 0;
    BOOL Broadcast = TRUE;

    for (i = 0; i < TestCount; i++)
    {
        trace("%d: %s %d.%d.%d.%d:%d\n", i, Tests[i].Type == SOCK_STREAM ? "TCP" : "UDP", Tests[i].Addr.sin_addr.S_un.S_un_b.s_b1, Tests[i].Addr.sin_addr.S_un.S_un_b.s_b2, Tests[i].Addr.sin_addr.S_un.S_un_b.s_b3, Tests[i].Addr.sin_addr.S_un.S_un_b.s_b4, Tests[i].ExpectedAddr.sin_port);
        Socket = socket(AF_INET, Tests[i].Type, Tests[i].Proto);
        if (Socket == INVALID_SOCKET)
        {
            skip("Failed to create socket with error %d for test %d, skipping\n", WSAGetLastError(), i);
            continue;
        }
        WSASetLastError(0xdeadbeef);
        Error = bind(Socket, (const struct sockaddr *) &Tests[i].Addr, sizeof(Tests[i].Addr));
        ok(Error == Tests[i].ExpectedResult, "Error %d differs from expected %d for test %d\n", Error, Tests[i].ExpectedResult, i);
        if (Error)
        {
            ok(WSAGetLastError() == Tests[i].ExpectedWSAResult, "Error %d differs from expected %d for test %d\n", WSAGetLastError(), Tests[i].ExpectedWSAResult, i);
        }
        else
        {
            AddrSize = sizeof(Addr);
            Error = getsockname(Socket, (struct sockaddr *) &Addr, &AddrSize);
            ok(Error == 0, "Unexpected error %d %d on getsockname for test %d\n", Error, WSAGetLastError(), i);
            ok(AddrSize == sizeof(Addr), "Returned size %d differs from expected %d for test %d\n", AddrSize, sizeof(Addr), i);
            ok(Addr.sin_addr.s_addr == Tests[i].ExpectedAddr.sin_addr.s_addr, "Expected address %lx differs from returned address %lx for test %d\n", Tests[i].ExpectedAddr.sin_addr.s_addr, Addr.sin_addr.s_addr, i);
            if (Tests[i].ExpectedAddr.sin_port)
            {
                ok(Addr.sin_port == Tests[i].ExpectedAddr.sin_port, "Returned port %d differs from expected %d for test %d\n", Addr.sin_port, Tests[i].ExpectedAddr.sin_port, i);
            }
            else
            {
                ok(Addr.sin_port != 0, "Port remained zero for test %d\n", i);
                if (Addr.sin_port != 0)
                {
                    trace("Returned port: %d\n", Addr.sin_port);
                    ok(Addr.sin_port >= 49152, "Unexpected port: %d\n", Addr.sin_port);
                    if (Addr.sin_port < min_port)
                        min_port = Addr.sin_port;
                    if (Addr.sin_port > max_port)
                        max_port = Addr.sin_port;
                }
            }
        }
        Error = closesocket(Socket);
        ok(Error == 0, "Unexpected error %d %d on closesocket for test %d\n", Error, WSAGetLastError(), i);
    }
    trace("Dynamic port range seen: %d-%d\n", min_port, max_port);
    ok(min_port >= 49152, "Unexpected min_port: %d\n", min_port);
    ok(max_port >= 49152, "Unexpected max_port: %d\n", max_port);

    /* Check double bind */
    Socket = socket(AF_INET, Tests[0].Type, Tests[0].Proto);
    ok(Socket != INVALID_SOCKET, "Failed to create socket with error %d for double bind test, next tests might be wrong\n", WSAGetLastError());
    WSASetLastError(0xdeadbeef);
    Error = bind(Socket, (const struct sockaddr *) &Tests[0].Addr, sizeof(Tests[0].Addr));
    ok(Error == Tests[0].ExpectedResult, "Error %d differs from expected %d for double bind test\n", Error, Tests[0].ExpectedResult);
    if (Error)
    {
        ok(WSAGetLastError() == Tests[i].ExpectedWSAResult, "Error %d differs from expected %d for double bind test\n", WSAGetLastError(), Tests[0].ExpectedWSAResult);
    }
    else
    {
        AddrSize = sizeof(Addr);
        Error = getsockname(Socket, (struct sockaddr *) &Addr, &AddrSize);
        ok(Error == 0, "Unexpected error %d %d on getsockname for double bind test\n", Error, WSAGetLastError());
        ok(AddrSize == sizeof(Addr), "Returned size %d differs from expected %d for double bind test\n", AddrSize, sizeof(Addr));
        ok(Addr.sin_addr.s_addr == Tests[0].ExpectedAddr.sin_addr.s_addr, "Expected address %lx differs from returned address %lx for double bind test\n", Tests[0].ExpectedAddr.sin_addr.s_addr, Addr.sin_addr.s_addr);
        if (Tests[0].ExpectedAddr.sin_port)
        {
            ok(Addr.sin_port == Tests[0].ExpectedAddr.sin_port, "Returned port %d differs from expected %d for double bind test\n", Addr.sin_port, Tests[0].ExpectedAddr.sin_port);
        }
        else
        {
            ok(Addr.sin_port != 0, "Port remained zero for double bind test\n");
        }
        WSASetLastError(0xdeadbeef);
        Error = bind(Socket, (const struct sockaddr *) &Tests[2].Addr, sizeof(Tests[2].Addr));
        ok_dec(Error, SOCKET_ERROR);
        ok_dec(WSAGetLastError(), WSAEINVAL);
    }
    Error = closesocket(Socket);
    ok(Error == 0, "Unexpected error %d %d on closesocket for double bind test\n", Error, WSAGetLastError());

    /* Check SO_BROADCAST and bind to broadcast address */
    Socket = socket(AF_INET, Tests[10].Type, Tests[10].Proto);
    ok(Socket != INVALID_SOCKET, "Failed to create socket with error %d for broadcast test, next tests might be wrong\n", WSAGetLastError());
    Error = setsockopt(Socket, SOL_SOCKET, SO_BROADCAST, (const char *) &Broadcast, sizeof(Broadcast));
    ok(Error == 0, "Unexpected error %d %d on setsockopt for broadcast test\n", Error, WSAGetLastError());
    Error = bind(Socket, (const struct sockaddr *) &Tests[10].Addr, sizeof(Tests[10].Addr));
    ok(Error == 0, "Unexpected error %d %d on bind for broadcast test\n", Error, WSAGetLastError());
    Error = closesocket(Socket);
    ok(Error == 0, "Unexpected error %d %d on closesocket for broadcast test\n", Error, WSAGetLastError());
}

/*
static
VOID
TestDynamicRange(IN_ADDR Address)
{
    // 49152-65535 is IANA dynamic port range.
    USHORT min_port = 65535, max_port = 49152;
    INT i;
    struct sockaddr_in Addr;

    for (i = 0; i < 10; i++)
    {
        bind(socket, localhost, port=0);
        getsockname(socket, &sockname);
        if (sockname.port < min_port)
            min_port = sockname.port;
        if (sockname.port > max_port)
            max_port = sockname.port;
    }
    trace("%d %d\n", i, min_port, max_port);
    ok(min_port >= 49152);
    ok(max_port <= 65535);
}
*/

START_TEST(bind)
{
    WSADATA WsaData;
    int Error;
    CHAR LocalHostName[256];
    struct hostent *Hostent;
    IN_ADDR Address = { 0 };
    SOCKET Socket;
    struct sockaddr_in Addr = { AF_INET };

    /* not yet initialized */
    StartSeh()
        Error = bind(INVALID_SOCKET, NULL, 0);
        ok_dec(Error, -1);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        Error = bind(INVALID_SOCKET, InvalidPointer, 0);
        ok_dec(Error, -1);
    EndSeh(STATUS_SUCCESS);

    Error = WSAStartup(MAKEWORD(2, 2), &WsaData);
    ok(Error == 0, "WSAStartup() failed, error %d\n", Error);
    if (Error != 0)
    {
        skip("No Winsock\n");
        return;
    }

    /* initialize Address for tests */
    Error = gethostname(LocalHostName, sizeof(LocalHostName));
    ok(Error == 0, "gethostname() failed, error %d. Following test results may be wrong\n", WSAGetLastError());
    if (Error == 0)
    {
        trace("Local host name is '%s'\n", LocalHostName);
        Hostent = gethostbyname(LocalHostName);
        ok(Hostent != NULL, "gethostbyname() failed, error %d\n", WSAGetLastError());
        if (Hostent && Hostent->h_addr_list[0] && Hostent->h_length == sizeof(IN_ADDR))
        {
            memcpy(&Address, Hostent->h_addr_list[0], sizeof(Address));
            trace("Local address: '%s'\n", inet_ntoa(Address));
        }
        else
        {
            ok(FALSE, "Could not determine Address. Following test results may be wrong\n");
        }
    }

    /* parameter tests */
    StartSeh()
        WSASetLastError(0xdeadbeef);
        Error = bind(INVALID_SOCKET, NULL, 0);
        ok_dec(Error, SOCKET_ERROR);
        ok_dec(WSAGetLastError(), WSAENOTSOCK);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        WSASetLastError(0xdeadbeef);
        Error = bind(INVALID_SOCKET, InvalidPointer, 0);
        ok_dec(Error, SOCKET_ERROR);
        ok_dec(WSAGetLastError(), WSAENOTSOCK);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        WSASetLastError(0xdeadbeef);
        Error = bind(Socket, NULL, 0);
        ok_dec(Error, SOCKET_ERROR);
        ok_dec(WSAGetLastError(), WSAEFAULT);
        closesocket(Socket);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        WSASetLastError(0xdeadbeef);
        Error = bind(Socket, InvalidPointer, 0);
        ok_dec(Error, SOCKET_ERROR);
        ok_dec(WSAGetLastError(), WSAEFAULT);
        closesocket(Socket);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        WSASetLastError(0xdeadbeef);
        Error = bind(Socket, NULL, sizeof(Addr));
        ok_dec(Error, SOCKET_ERROR);
        ok_dec(WSAGetLastError(), WSAEFAULT);
        closesocket(Socket);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        WSASetLastError(0xdeadbeef);
        Error = bind(Socket, InvalidPointer, sizeof(Addr));
        ok_dec(Error, SOCKET_ERROR);
        ok_dec(WSAGetLastError(), WSAEFAULT);
        closesocket(Socket);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        WSASetLastError(0xdeadbeef);
        Error = bind(Socket, (const struct sockaddr *) &Addr, 0);
        ok_dec(Error, SOCKET_ERROR);
        ok_dec(WSAGetLastError(), WSAEFAULT);
        closesocket(Socket);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        WSASetLastError(0xdeadbeef);
        Error = bind(Socket, (const struct sockaddr *) &Addr, sizeof(Addr)-1);
        ok_dec(Error, SOCKET_ERROR);
        ok_dec(WSAGetLastError(), WSAEFAULT);
        closesocket(Socket);
    EndSeh(STATUS_SUCCESS);

    TestBind(Address);
    /* TODO: test IPv6 */

    Error = WSACleanup();
    ok(Error == 0, "WSACleanup() failed, error %d\n", WSAGetLastError());
}
