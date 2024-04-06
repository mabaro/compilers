#pragma once

#include "scanner.h"
#include "common.h"

Result<void> compile(const char *source) {
    sScanner.initScanner(source);

    int line = -1;
    for(;;)
    {
        Result<TokenHolder> tokenResult = sScanner.scanToken();
        if (!tokenResult.isOk())
        {
            LOG_ERROR("Invalid token: %s\n", tokenResult.error().message);
            break;
        }
        const TokenHolder& token = tokenResult.value();
        if (token.line != line)
        {
            printf("%4d ", token.line);
            line = token.line;
        } else {
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

    return makeResult(); 
}
