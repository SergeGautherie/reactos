/*
 * PROJECT:         ReactOS memwatch Utility
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            tools/memwatch/memwatch.cpp
 * PURPOSE:         Watching functions in memory, notifying if something changes.
 * PROGRAMMERS:     Mark Jansen
 */

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <getopt.h>

const DWORD kProtNotSet = 0xffffffff;
const size_t kNumBytesToWatch = 10;

struct HexPrint
{
    HexPrint(std::vector<BYTE>& data) : Data(data) { ; }

private:
    friend std::ostream& operator<<(std::ostream& strm, const HexPrint& pr);
    std::vector<BYTE>& Data;
};

std::ostream& operator<<(std::ostream& strm, const HexPrint& pr)
{
    for (size_t n = 0; n < pr.Data.size(); ++n)
    {
        strm << (n > 0 ? " ":"") << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)pr.Data[n];
    }
    return strm;
}

struct Prot2Str
{
    Prot2Str(DWORD protection) : Protection(protection) { ; }

private:
    friend std::ostream& operator<<(std::ostream& strm, const Prot2Str& prot2str);
    DWORD Protection;
};


#define STREAMIF(x)	if( Prot & (x) )\
{\
    strm << (first ? "" : "|") << #x;\
    first = false;\
    Prot &= ~(x);\
}
std::ostream& operator<<(std::ostream& strm, const Prot2Str& prot2str)
{
    DWORD Prot = prot2str.Protection;
    bool first = true;
    STREAMIF(PAGE_NOACCESS);
    STREAMIF(PAGE_READONLY);
    STREAMIF(PAGE_READWRITE);
    STREAMIF(PAGE_WRITECOPY);
    STREAMIF(PAGE_EXECUTE);
    STREAMIF(PAGE_EXECUTE_READ);
    STREAMIF(PAGE_EXECUTE_READWRITE);
    STREAMIF(PAGE_EXECUTE_WRITECOPY);
    STREAMIF(PAGE_GUARD);
    STREAMIF(PAGE_NOCACHE);
    STREAMIF(PAGE_WRITECOMBINE);
    if (Prot) {
        strm << (first ? "" : "|") << std::hex << "0x" << Prot;
    }
    return strm;
}
#undef STREAMIF



struct MemCheck
{
    MemCheck(const char* module)
        :Module(module ? module : "")
        , Target(NULL)
        , ExpectedProtection(kProtNotSet)
    {
        Handle = LoadLibraryA(Module.c_str());
    }
    ~MemCheck()
    {
        FreeLibrary(Handle);
    }

    void Proc(const char* name)
    {
        if (Target)
            return;
        if (!Handle)
        {
            std::cout << "Failed loading " << Module << std::endl;
            return;
        }
        // assume entrypoint
        if (!name || !*name)
        {
            Function = "Entrypoint";
            PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)Handle;
            PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(((PBYTE)dos) + dos->e_lfanew);
            Target = (PBYTE)Handle + nt->OptionalHeader.AddressOfEntryPoint;
        }
        else if (*name == '#')
        {
            Function = name;
            int ord = atoi(name+1);
            Target = (LPVOID)GetProcAddress(Handle, (LPCSTR)ord);
        }
        else
        {
            Function = name;
            Target = (LPVOID)GetProcAddress(Handle, name);
        }
    }

    void Init()
    {
        if (ExpectedData.empty())
            ExpectedData.resize(kNumBytesToWatch);
        Proc(NULL);
        Read(ExpectedData, ExpectedProtection != kProtNotSet ? &ExpectedProtection : NULL);
        if (Target)
        {
            const char* andExtra = ExpectedProtection != kProtNotSet ? " and protection" : "";
            std::cout << "Checking " << ExpectedData.size() << " bytes" << (andExtra) << " from " << Module << "!" << Function << std::endl;
        }
    }

    void Update()
    {
        if (!Handle)
            return;
        std::vector<BYTE> Result(ExpectedData.size(), 0);
        DWORD ReadProt = 0;

        Read(Result, ExpectedProtection != kProtNotSet ? &ReadProt : NULL);
        if (ExpectedProtection != kProtNotSet)
        {
            if (ExpectedProtection != ReadProt)
            {
                std::cout << Module << "!" << Function << ", protection changed from: "
                    << std::hex << Prot2Str(ExpectedProtection) << " to " << std::hex << Prot2Str(ReadProt) << std::endl;
                ExpectedProtection = ReadProt;
            }
        }
        if (memcmp(&ExpectedData[0], &Result[0], Result.size()))
        {
            std::cout << Module << "!" << Function << ", data changed from: " << std::endl;
            std::cout << "<" << HexPrint(ExpectedData) << "> to:" << std::endl;
            std::cout << "<" << HexPrint(Result) << ">" << std::endl;
            ExpectedData = Result;
        }
    }

    void Read(std::vector<BYTE>& data, PDWORD Prot)
    {
        if (!Handle)
            return;
        if (Prot)
        {
            MEMORY_BASIC_INFORMATION mbi = { 0 };
            VirtualQuery(Target, &mbi, sizeof(mbi));
            *Prot = mbi.Protect;
        }
        memcpy(&data[0], Target, data.size());
    }

    std::string Module;
    std::string Function;
    HMODULE Handle;
    LPVOID Target;
    std::vector<BYTE> ExpectedData;
    DWORD ExpectedProtection;
};


std::vector<MemCheck*> g_Checks;

int print_usage()
{
    std::cout << "Usage: memwatch --dll=<dll> [--func=<func>] [--prot] [--num=<num>]" << std::endl;
    std::cout << "      -d<dll> or --dll=<dll>      - specify the dll to monitor" << std::endl;
    std::cout << "      -f<func> or --func=<func>   - specify the function in the dll to monitor." << std::endl;
    std::cout << "      -p or --prot                - Also monitor memory protection flags." << std::endl;
    std::cout << "      -n<num> or --num=<num>      - Specify the number of bytes to watch."<< std::endl;
    std::cout << std::endl << "  Dll can be specified multiple times," << std::endl;
    std::cout << "  all other options operate on the last Dll specified." << std::endl;
    std::cout << "  When no function is specified, the entrypoint will be used." << std::endl;
    std::cout << "  When no number of bytes is specified, it will default to " << kNumBytesToWatch << "." << std::endl;
    return 1;
}

int main(int argc, char ** argv)
{
    static struct option long_options[] = {
        { "dll", required_argument, 0, 'd' },
        { "func", required_argument, 0, 'f' },
        { "prot", no_argument, 0, 'p' },
        { "num", required_argument, 0, 'n' },
        { 0, 0, 0, 0 }
    };
    MemCheck* cur = NULL;
    int long_index = 0, opt;
    while ((opt = getopt_long(argc, argv, "d:f:pn:", long_options, &long_index)) != -1)
    {
        switch (opt)
        {
        case 'd':
            if (cur)
                cur->Init();
            cur = new MemCheck(optarg);
            g_Checks.push_back(cur);
            break;
        case 'f':
            if (!cur)
                return print_usage();
            cur->Proc(optarg);
            break;
        case 'p':
            if (!cur)
                return print_usage();
            if (cur->ExpectedProtection == 0xffffffff)
                cur->ExpectedProtection = 0;
            break;
        case 'n':
            if (cur)
            {
                int num = atoi(optarg);
                cur->ExpectedData.resize(num);
            }
            else
            {
                return print_usage();
            }
            break;
        default:
            return print_usage();
        }
    }
    if (!cur)
    {
        std::cout << "You did not specify a module!" << std::endl;
        return print_usage();
    }
    cur->Init();
    for (;;)
    {
        for (size_t n = 0; n < g_Checks.size(); ++n)
        {
            g_Checks[n]->Update();
        }
        Sleep(500);
    }
    return 0;
}
