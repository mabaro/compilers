#pragma once

#include "common.h"
#include "compiler.h"
#include "debug.h"

#include <cstring>
#include <cstdlib>


struct VirtualMachine
{
	enum class ErrorCode
	{
		CompileError,
		RuntimeError,
		Undefined = 0x255
	};
	using Error = Error<ErrorCode>;
	using result_t = Result<void, Error>;

	result_t init() { return makeResultError<result_t>(); }
	result_t finish() { return makeResultError<result_t>(); }

	VirtualMachine() {}
	~VirtualMachine() {}

	result_t run() {
		disassemble(*_currentChunk, "test chunk");
		return makeResultError<result_t>();
	}

	result_t interpret(const char* source) {
		Compiler::result_t result = _compiler.compile(source);
		if (!result.isOk()) {
			return makeResultError<result_t>(ErrorCode::CompileError, result.error().message());
		}
		const Chunk* currentChunk = result.value();
		_currentChunk = currentChunk;
		_currentIP = currentChunk->getCode();

		result_t runResult = run();

		return runResult;
	}

	Result<char*> readFile(const char* path)
	{
		FILE* file = fopen(path, "rb");
		if (file == nullptr)
		{
			LOG_ERROR("Couldn't open file '%s'\n", path);
			return makeResultError<Result<char*>>(Result<char*>::error_t::code_t::Undefined);
		}

		fseek(file, 0L, SEEK_END);
		const size_t fileSize = ftell(file);
		rewind(file);

		char* buffer = (char*)malloc(fileSize) + 1;
		if (buffer == nullptr)
		{
			char message[1024];
			sprintf_s(message, "Couldn't allocate memory for reading the file '%s' with size %l byte(s)\n", path, fileSize);
			LOG_ERROR(message);
			fclose(file);
			return makeResultError<Result<char*>>(Result<char*>::error_t::code_t::Undefined, message);
		}
		const size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
		if (bytesRead < fileSize)
		{
			LOG_ERROR("Couldn't read the file '%s'\n", path);
			fclose(file);
			return makeResultError<Result<char*>>(Result<char*>::error_t::code_t::Undefined);
		}
		buffer[bytesRead] = '\0';

		fclose(file);
		return makeResult<Result<char*>>(buffer);
	}

	result_t runFile(const char* path)
	{
		Result<char*> source = readFile(path);
		if (!source.isOk())
		{
			return makeResultError<result_t>(source.error().message());
		}
		result_t result = interpret(source.value());
		free(source.value());

		return result;
	}

	result_t repl()
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

			result_t result = interpret(line);
			if (!result.isOk())
			{
				char message[1024];
				sprintf_s(message, "INTERPRETER: %s", result.error().message().c_str());
				return makeResultError<result_t>(result_t::error_t::code_t::CompileError, message);
			}
		}

		return makeResultError<result_t>();
	}

protected:
	const Chunk* _currentChunk = nullptr;
	const uint8_t* _currentIP = nullptr;

	Compiler _compiler;
};

