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
            0x19, 0x53, 0x4f, 0x55, 0x52, 0x43, 0x45, 0x00, 0x5f, 0x43, 0x4f, 0x44, 0x45, 0x34,
            0x32, 0x5f, 0x00, 0x00, 0x01, 0x61, 0x2e, 0x44, 0x41, 0x54, 0x41, 0x01, 0x04, 0x00,
            0x3d, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0x20,
            0x3a, 0x29, 0x2e, 0x43, 0x4f, 0x44, 0x45, 0x04, 0x00, 0x01, 0x00, 0x0f, 0x00,
        };

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

        ObjectFunction*       function  = ObjectFunction::Create("LOADER");
        const volatile char*  codeBegin = codeStr;
        const volatile size_t codeLen   = sizeof(codeStr);
        ByteStream            istr((const uint8_t*)codeBegin, codeLen);
        auto                  deserializeResult = function->deserialize(istr);
        ASSERT(deserializeResult.isOk());
        if (argc > 1 && (nullptr != strstr(argv[1], "disassemble")))
        {
            disassemble(function->chunk, "VM");
        }

        auto result = VM.runFromByteCode(function->chunk);
        if (!result.isOk())
        {
            return errorReportFunc(result.error().message().c_str());
        }
    }

    return resultCode;
}