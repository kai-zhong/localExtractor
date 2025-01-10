#include <iostream>
#include <chrono>
#include <ctime>

#include "./util/common.h"
#include "./util/edgeReader.h"
#include "./localExtractor/localExtractor.h"

const int addBatchNum = 1000;
const int delBatchNum = 500;

int main(int argc, char* argv[])
{
    cmdOptions options = parseCmdLineArgs(argc, argv);

    Graph graph;
    localExtractor extractor;
    EdgeReader addEdgeReader(options.addFilename);
    EdgeReader delEdgeReader(options.deleteFilename);

    // 图加载
    graph.loadGraphfromFile(options.filename);
    
    // 图节点签名
    auto start = std::chrono::high_resolution_clock::now();
    extractor.signGraph(graph);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time taken for signature computation: " << duration.count() << " ms" << std::endl << std::endl;

    size_t cnt = 0;
    while(!addEdgeReader.isEndOfFile() || !delEdgeReader.isEndOfFile())
    {
        std::cout << ++cnt << "th round of adding edges : " << std::endl;
        std::vector<std::pair<VertexID, VertexID>> addEdges = addEdgeReader.readNextEdges(addBatchNum);
        start = std::chrono::high_resolution_clock::now();
        for(auto edge : addEdges)
        {
            VertexID srcVid = edge.first;
            VertexID dstVid = edge.second;
            graph.addEdge(srcVid, dstVid, true, true);
            extractor.signAddUpdate(graph.getVertex(srcVid), graph.getVertex(dstVid));
        }
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Add [" << addEdges.size() << "] Edges Time taken: " << duration.count() << " ms" << std::endl;

        std::vector<std::pair<VertexID, VertexID>> delEdges = delEdgeReader.readNextEdges(delBatchNum);
        start = std::chrono::high_resolution_clock::now();
        for(auto edge : delEdges)
        {
            VertexID srcVid = edge.first;
            VertexID dstVid = edge.second;
            graph.removeEdge(srcVid, dstVid, true, true);
            bool srcExists = graph.hasVertex(srcVid);
            bool dstExists = graph.hasVertex(dstVid);
            if(!srcExists)
            {
                extractor.signDeleteVertexUpdate(srcVid);
            }
            else
            {
                extractor.signDeleteEdgeUpdate(graph.getVertex(srcVid));
            }
            if(!dstExists)
            {
                extractor.signDeleteVertexUpdate(dstVid);
            }
            else
            {
                extractor.signDeleteEdgeUpdate(graph.getVertex(dstVid));
            }
        }
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Delete [" << delEdges.size() << "] Edges Time taken: " << duration.count() << " ms" << std::endl << std::endl;
 
        // k-core 图抽取
        start = std::chrono::high_resolution_clock::now();
        Graph kcoreGraph = extractor.kcoreExtract(graph, options.query, options.k);
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "K-core Graph Extraction Time taken: " << duration.count() << " ms" << std::endl << std::endl;
        std::cout << "Result Graph Vertex Num : " << extractor.getCandGraph().getVertexNum() << std::endl;
        std::cout << std::endl;

        // VO验证
        extractor.mclInit();
        start = std::chrono::high_resolution_clock::now();
        extractor.verifyKcoreGraph(extractor.getVO());
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Time taken for verification: " << duration.count() << " ms" << std::endl << std::endl;

        // VO大小计算
        extractor.calculateVOSize();
        std::cout << std::endl;
    }

    return 0;
}