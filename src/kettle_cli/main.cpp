// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "kettle/bytecode.h"
#include "kettle/compiler.h"
#include "kettle/graph_view.h"
#include "kettle/ir.h"

namespace kettle
{
void registerBuiltinNatives(NativeRegistry& registry);
void registerBuiltinNodeCompilers(NodeCompilerRegistry& registry);
void setNativeContext(const GraphView* graph, const Program* program);
} // namespace kettle

struct CompileResult
{
    kettle::GraphView graph;
    kettle::NativeRegistry natives;
    kettle::IrProgram ir;
    kettle::Program program;
};

static void printUsage()
{
    std::cerr << "Usage:\n"
              << "  kettle_cli run <graph.ktl>\n"
              << "  kettle_cli dump-ir <graph.ktl> [-o out.kir.txt]\n"
              << "  kettle_cli dump-bytecode <graph.ktl> [-o out.kbc.txt]\n"
              << "  kettle_cli compile <graph.ktl> [-ir out.kir.txt] [-bc out.kbc.txt]\n";
}

static std::string parseOutputPath(int argc, char** argv, int startIndex)
{
    for (int i = startIndex; i < argc; ++i)
    {
        if (std::string_view(argv[i]) == "-o")
        {
            if (i + 1 >= argc)
                throw std::runtime_error("missing path after -o");

            return argv[i + 1];
        }
    }

    return {};
}

static std::string parseNamedOutputPath(int argc, char** argv, int startIndex, std::string_view option)
{
    for (int i = startIndex; i < argc; ++i)
    {
        if (std::string_view(argv[i]) == option)
        {
            if (i + 1 >= argc)
                throw std::runtime_error("missing path after output option");

            return argv[i + 1];
        }
    }

    return {};
}

static void writeTextFile(const std::string& path, const std::string& text)
{
    std::ofstream file(path, std::ios::binary);

    if (!file)
        throw std::runtime_error("failed to open output file: " + path);

    file << text;
    std::cout << "Wrote " << path << '\n';
}

static CompileResult compileGraph(const char* path)
{
    kettle::GraphView graph = kettle::GraphView::load(path);

    kettle::NativeRegistry natives;
    kettle::registerBuiltinNatives(natives);

    kettle::NodeCompilerRegistry nodeCompilers;
    kettle::registerBuiltinNodeCompilers(nodeCompilers);

    kettle::Compiler compiler{
        graph,
        natives,
        nodeCompilers,
    };

    compiler.compileFromEventBegin();

    return CompileResult{
        .graph = std::move(graph),
        .natives = std::move(natives),
        .ir = std::move(compiler.ir.program),
        .program = std::move(compiler.program),
    };
}

static int dumpGraphIr(const char* path, const std::string& outputPath)
{
    CompileResult result = compileGraph(path);
    const std::string text = kettle::formatIrProgram(result.ir);

    if (outputPath.empty())
        std::cout << text;
    else
        writeTextFile(outputPath, text);

    return 0;
}

static int dumpGraphBytecode(const char* path, const std::string& outputPath)
{
    CompileResult result = compileGraph(path);
    const std::string text = kettle::formatProgram(result.program);

    if (outputPath.empty())
        std::cout << text;
    else
        writeTextFile(outputPath, text);

    return 0;
}

static int compileGraphArtifacts(const char* path, const std::string& irOutput, const std::string& bytecodeOutput)
{
    CompileResult result = compileGraph(path);

    if (!irOutput.empty())
        writeTextFile(irOutput, kettle::formatIrProgram(result.ir));

    if (!bytecodeOutput.empty())
        writeTextFile(bytecodeOutput, kettle::formatProgram(result.program));

    return 0;
}

static int runGraph(const char* path)
{
    CompileResult result = compileGraph(path);

    kettle::setNativeContext(&result.graph, &result.program);

    kettle::VM vm;
    vm.natives = &result.natives;
    vm.execute(result.program);

    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printUsage();
        return 1;
    }

    const std::string_view command = argv[1];

    try
    {
        if (command == "run")
        {
            if (argc != 3)
                throw std::runtime_error("Usage: kettle_cli run <graph.ktl>");

            return runGraph(argv[2]);
        }

        if (command == "dump-ir")
        {
            if (argc < 3)
                throw std::runtime_error("Usage: kettle_cli dump-ir <graph.ktl> [-o out.kir.txt]");

            return dumpGraphIr(argv[2], parseOutputPath(argc, argv, 3));
        }

        if (command == "dump-bytecode")
        {
            if (argc < 3)
                throw std::runtime_error("Usage: kettle_cli dump-bytecode <graph.ktl> [-o out.kbc.txt]");

            return dumpGraphBytecode(argv[2], parseOutputPath(argc, argv, 3));
        }

        if (command == "compile")
        {
            if (argc < 3)
                throw std::runtime_error("Usage: kettle_cli compile <graph.ktl> [-ir out.kir.txt] [-bc out.kbc.txt]");

            const std::string irPath = parseNamedOutputPath(argc, argv, 3, "-ir");
            const std::string bytecodePath = parseNamedOutputPath(argc, argv, 3, "-bc");

            if (irPath.empty() && bytecodePath.empty())
                throw std::runtime_error("compile needs at least one text output: -ir or -bc");

            return compileGraphArtifacts(argv[2], irPath, bytecodePath);
        }

        throw std::runtime_error("unknown command");
    }
    catch (const std::exception& e)
    {
        std::cerr << "Kettle error: " << e.what() << "\n";
        return 1;
    }
}