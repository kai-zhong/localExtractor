#include <iostream>
#include <chrono>
#include <ctime>

#include "./util/common.h"
#include "./localExtractor/localExtractor.h"

int main(int argc, char* argv[])
{
    cmdOptions options = parseCmdLineArgs(argc, argv);

    Graph graph;
    localExtractor extractor;

    graph.loadGraphfromFile(options.filename);


    auto start = std::chrono::high_resolution_clock::now();
    extractor.kcoreExtract(graph, options.query, options.k);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time taken: " << duration.count() << " ms" << std::endl << std::endl;

    // extractor.getCandGraph().printGraphInfoSimple();
        std::cout << "Result Graph Vertex Num : " << extractor.getCandGraph().getVertexNum() << std::endl;

    return 0;
}