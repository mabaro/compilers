#include "common.h"
#include "compiler.h"

#include <cstring>
#include <cstdlib>


enum class InterpretResult
{
    Ok,
    CompileError,
    RuntimeError
};

Result<void> initVM() { return makeResult(); }
Result<void> freeVM() { return makeResult(); }
Result<InterpretResult> interpret(const char *source) {
    compile(source);
    return makeResult(InterpretResult::Ok); 
}

Result<char*> readFile(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == nullptr)
    {
        LOG_ERROR( "Couldn't open file '%s'\n", path);
        return makeResult<char*>(ErrorCode::Undefined);
    }

    fseek(file, 0L, SEEK_END);
    const size_t fileSize = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(fileSize) + 1;
    if (buffer == nullptr)
    {
        LOG_ERROR("Couldn't allocate memory for reading the file '%s' with size %l byte(s)\n", path, fileSize);
        fclose(file);
        return makeResult<char*>(ErrorCode::Undefined);
    }
    const size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize)
    {
        LOG_ERROR("Couldn't read the file '%s'\n", path);
        fclose(file);
        return makeResult<char*>(ErrorCode::Undefined);
    }
    buffer[bytesRead] = '\0';

    fclose(file);
    return makeResult<char*>(buffer);
}

Result<InterpretResult> runFile(const char *path)
{
    Result<char*> source = readFile(path);
    if (!source.isOk())
    {
        return source.error();
    }
    Result<InterpretResult > result = interpret(source.value());
    free(source.value());

    return result;
}

Result<void> repl()
{
    char line[1024];
    for (;;)
    {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin))
        {
            printf("\n");
            break;
        }

        Result<InterpretResult> result = interpret(line);
        if (!result.isOk())
        {
            return makeResult<void>(result.error());
        }
    }

    return makeResult();
}