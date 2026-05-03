// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include "kettle/bytecode.h"
#include "kettle/compiler.h"
#include "kettle/graph_view.h"

#include <iostream>

namespace kettle
{
void registerBuiltinNatives(NativeRegistry& registry);
void registerBuiltinNodeCompilers(NodeCompilerRegistry& registry);
void setNativeContext(const GraphView* graph, const Program* program);
} // namespace kettle

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: kettle_cli tmp/graph.ktl\n";
        return 1;
    }

    try
    {
        kettle::GraphView graph = kettle::GraphView::load(argv[1]);

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

        std::cout << "Bytecode instructions: " << compiler.program.code.size() << "\n";


        kettle::setNativeContext(&graph, &compiler.program);

        kettle::VM vm;
        vm.natives = &natives;
        vm.execute(compiler.program);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Kettle error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}