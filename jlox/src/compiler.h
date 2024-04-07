#pragma once

#include "scanner.h"
#include "vm.h"
#include "common.h"

struct Chunk
{
    Chunk() {}
    ~Chunk() {}

    char* code = nullptr;
};

struct Compiler
{
    enum class ErrorCode {
        ScannerError,
        Undefined
    };
    using result_t = Result<Chunk, Error<ErrorCode>>;

    result_t compile(const char* source) {
        sScanner.initScanner(source);

        Chunk chunk;

        int line = -1;
        for (;;)
        {
            Scanner::TokenResult_t tokenResult = sScanner.scanToken();
            if (!tokenResult.isOk())
            {
                char message[1024];
                sprintf(message, "Invalid token: %s\n", tokenResult.error().message.c_str());
				return makeError< result_t>(result_t::error_t::code_t::ScannerError, message);
                //break;
            }
            const TokenHolder& token = tokenResult.value();
            if (token.line != line)
            {
                printf("%4d ", token.line);
                line = token.line;
            }
            else {
                printf("  | ");
            }
            printf("%2d '%.*s'\n", token.type, token.length, token.start);

            if (token.type == Token::Exit) {
                exit(0);//kk
            }
            if (token.type == Token::Eof)
            {
                break;
            }
        }

        return makeResult<result_t>(std::move(chunk));
    }
};
Compiler sCompiler;
