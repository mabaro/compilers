#include <fstream>
#include <iostream>
#include <sstream>

#include "utils/byte_buffer.h"
#include "utils/common.h"
#include "vm.h"

int main(int argc, const char* argv[])
{
    int resultCode = 0;

    VirtualMachine::Configuration virtualMachineConfiguration;

    auto errorReportFunc =
        [&resultCode](const char* errorMessage, int errorCode = -1, std::function<void()> postErrorCallback = nullptr)
    {
        resultCode = errorCode;
        LOG_ERROR("(CODE: %d) %s\n", errorCode, errorMessage);
        if (postErrorCallback)
        {
            postErrorCallback();
        }
        return resultCode;
    };

    {
        const char codeStr[] = {
            // -> CODE
            'C',  'O',  'D',  'E',  '4',  '2',  0x00, 0x00, 0x01, 'a',  '.',  'D',  'A',  'T',  'A',  0x01, 0x04, 0x00,
            0x31, 'h',  'e',  'l',  'l',  'o',  ' ',  'w',  'o',  'r',  'l',  'd',  '!',  '.',  'C',  'O',  'D',  'E',
            0x04, 0x00, 0x01, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x0E, 0x0A, 0x0D, 0x0B, 0x0E, 0x0E, 0x0F};

        auto isCarFunc = [](char a) { return (a >= '0' && a <= 'z') || a == ' ' || a == '.'; };
        for (uint8_t a : codeStr)
        {
            const bool isCar = isCarFunc(a);
            if (isCar)
            {
                std::cout << a;
            }
            else
            {
                std::cout << (int)a;
            }
        }
        std::cout << std::endl;

        VirtualMachine VM;
        VM.init(virtualMachineConfiguration);
        ScopedCallback vmFinish([&VM] { VM.finish(); });

        Chunk                 code("LOADER");
        const volatile char*  codeBegin = codeStr;
        const volatile size_t codeLen   = sizeof(codeStr);
        ByteStream            istr((const uint8_t*)codeBegin, codeLen);
        auto                  deserializeResult = code.deserialize(istr);
        ASSERT(deserializeResult.isOk());
        if (argc > 1 && (nullptr != strstr(argv[1], "disassemble")))
        {
            disassemble(code, "VM");
        }

        auto result = VM.runFromByteCode(code);
        if (!result.isOk())
        {
            return errorReportFunc(result.error().message().c_str());
        }
    }

    return resultCode;
}