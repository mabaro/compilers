#include "compiler.h"

Compiler::result_t Compiler::compileFromSource(const char                       *sourceCode,
                                               Optional<Compiler::Configuration> optConfiguration)
{
    return compile(sourceCode, "SOURCE", optConfiguration);
}

Compiler::result_t Compiler::compileFromFile(const char *path, Optional<Compiler::Configuration> optConfiguration)
{
    Result<std::unique_ptr<char[]>> source = utils::readFile(path);
    if (!source.isOk())
    {
        return makeResultError<result_t>(source.error().message());
    }
    char *buffer = source.value().get();

    return compile(buffer, path, optConfiguration);
}