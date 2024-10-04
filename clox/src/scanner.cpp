#include "scanner.h"

namespace unit_tests
{
namespace scanner
{
void run()
{
    Token t1{};
    t1.line = 15;
    Token t2;
    auto tokenResult = makeResult<Token>(t1);
    ASSERT(tokenResult.value().line == t1.line);
}
}  // namespace scanner
}  // namespace unit_tests