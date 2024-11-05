#include <fstream>
#include <iostream>
#include <sstream>

#include "utils/common.h"
#include "utils/byte_buffer.h"
#include "vm.h"

int main(int argc, const char* argv[])
{
    int resultCode = 0;

    Compiler                      compiler;
    Compiler::Configuration       compilerConfiguration;
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

    struct Param
    {
        const char* arg;
        const char* desc;
        enum class Type
        {
            help,
#if USING(EXTENDED_ERROR_REPORT)
            extended_errors,
#endif  // #if USING(EXTENDED_ERROR_REPORT)
        };
        Type        type;
        const char* params = nullptr;
    };

#define ADD_PARAM(TYPE, DESC) {#TYPE, DESC, Param::Type::TYPE}
#define ADD_PARAM_WITH_PARAMS(TYPE, DESC, PARAMS) {#TYPE, DESC, Param::Type::TYPE, PARAMS}
    const Param params[] = {
        ADD_PARAM(help, "Shows this help"),
#if USING(EXTENDED_ERROR_REPORT)
        ADD_PARAM_WITH_PARAMS(extended_errors, "Show extended error reporting", "<0 / 1>"),
#endif  // #if USING(EXTENDED_ERROR_REPORT)
    };
#undef ADD_PARAM
    auto showHelpFunc = [&](std::ostream& ostr)
    {
        ostr << "CLOX-variant VirtualMachine version " << VERSION << std::endl;
        ostr << format("Usage: %s [arguments] [filepath]\n", argv[0]);
        size_t longerArg = 0;
        for (const Param& param : params)
        {
            longerArg = std::max<size_t>(longerArg, strlen(param.arg));
        }
        for (const Param& param : params)
        {
            const size_t spaceCount = longerArg - (strlen(param.arg) + (param.params ? strlen(param.params) + 1 : 0));
            ostr << format("\t-%s%s%s", param.arg, param.params ? " " : "", param.params ? param.params : "");
            ostr << format("%*s\t%s\n", (int)spaceCount, " ", param.desc);
        }
    };
    auto errorReportWithHelpFunc = [&](const char* msg, int32_t errCode = -1)
    {
        auto result = errorReportFunc(msg, errCode);
        showHelpFunc(std::cerr);
        return result;
    };

    /////////////////////////////////////////////////////////////////////////////////

    struct
    {
        bool        hasToShowHelp = false;
        const char* filepath      = nullptr;
    } config;

    auto isArgFunc = [](const char* arg) { return (arg[0] == '-'); };
    for (const char** argvPtr = &argv[1]; argvPtr != &argv[argc]; ++argvPtr)
    {
        const char* curArg = *argvPtr;
        if (isArgFunc(curArg))
        {
            bool validParam = false;
            for (const Param& param : params)
            {
                if (0 == strcmp(&curArg[1], param.arg))
                {
                    validParam = true;

                    switch (param.type)
                    {
                        case Param::Type::help: config.hasToShowHelp = true; break;
                        default: validParam = false; break;
                    }
                }
            }
            if (!validParam)
            {
                return errorReportWithHelpFunc(format("Invalid parameter: %s", curArg).c_str());
            }
        }
        else
        {
            if (config.filepath != nullptr)
            {
                return errorReportWithHelpFunc(format("Argument <%s> unexpected", curArg).c_str());
            }
            config.filepath = *argvPtr;
        }
    }

    if (config.hasToShowHelp)
    {
        showHelpFunc(std::cout);
    }
    else
    {
        if (config.filepath == nullptr)
        {
            return errorReportWithHelpFunc(format("Missing binary filepath").c_str());
        }
        else
        {
            VirtualMachine VM;
            VM.init(virtualMachineConfiguration);
            ScopedCallback vmFinish([&VM] { VM.finish(); });

            Chunk code(config.filepath);
            {
                std::ifstream ifs(config.filepath, std::ifstream::binary);
                if (!ifs.is_open() || !ifs.good())
                {
                    return errorReportFunc(format("Failed to open file '%s' for reading", config.filepath).c_str());
                }
                auto deserializeResult = code.deserialize(ifs);
                if (!deserializeResult.isOk())
                {
                    return errorReportFunc(
                        format("Failed loading bytecode: %s", deserializeResult.error().message().c_str()).c_str());
                }
            }

            auto result = VM.runFromByteCode(code);
            if (!result.isOk())
            {
                resultCode = -1;
                return errorReportFunc(result.error().message().c_str());
            }
        }
    }
    return resultCode;
}