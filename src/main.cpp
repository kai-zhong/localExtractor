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
    extractor.signGraph(graph);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time taken for signature computation: " << duration.count() << " ms" << std::endl << std::endl;

    start = std::chrono::high_resolution_clock::now();
    Graph kcoreGraph = extractor.kcoreExtract(graph, options.query, options.k);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time taken: " << duration.count() << " ms" << std::endl << std::endl;

    // extractor.getCandGraph().printGraphInfoSimple();
    std::cout << "Result Graph Vertex Num : " << extractor.getCandGraph().getVertexNum() << std::endl;
    std::cout << std::endl;

    extractor.mclInit();

    start = std::chrono::high_resolution_clock::now();
    extractor.verifyKcoreGraph(extractor.getVO());
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time taken for verification: " << duration.count() << " ms" << std::endl << std::endl;

    std::cout << std::endl;
    extractor.calculateVOSize();

    return 0;
}