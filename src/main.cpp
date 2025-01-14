#include <iostream>
#include <chrono>
#include <ctime>

#include "./util/common.h"
#include "./util/edgeReader.h"
#include "./localExtractor/localExtractor.h"

const int addBatchNum = 10000;
const int delBatchNum = 5000;

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
    extractor.generateGraphDigest(graph);
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
            bool srcExists = graph.hasVertex(srcVid);
            bool dstExists = graph.hasVertex(dstVid);
            unsigned char srcOldDigest[SHA256_DIGEST_LENGTH];
            unsigned char dstOldDigest[SHA256_DIGEST_LENGTH];
            if(srcExists)
            {
                memcpy(srcOldDigest, graph.getVertexDigest(srcVid).data(), SHA256_DIGEST_LENGTH);
            }
            if(dstExists)
            {
                memcpy(dstOldDigest, graph.getVertexDigest(dstVid).data(), SHA256_DIGEST_LENGTH);
            }
            graph.addEdge(srcVid, dstVid, true, true);
            extractor.updateGraphDigest(srcExists? srcOldDigest : nullptr, graph.getVertexDigest(srcVid).data());
            extractor.updateGraphDigest(dstExists? dstOldDigest : nullptr, graph.getVertexDigest(dstVid).data());
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
            unsigned char srcOldDigest[SHA256_DIGEST_LENGTH];
            unsigned char dstOldDigest[SHA256_DIGEST_LENGTH];
            memcpy(srcOldDigest, graph.getVertexDigest(srcVid).data(), SHA256_DIGEST_LENGTH);
            memcpy(dstOldDigest, graph.getVertexDigest(dstVid).data(), SHA256_DIGEST_LENGTH);
            graph.removeEdge(srcVid, dstVid, true, true);
            bool srcExists = graph.hasVertex(srcVid);
            bool dstExists = graph.hasVertex(dstVid);
            extractor.updateGraphDigest(srcOldDigest, srcExists? graph.getVertexDigest(srcVid).data() : nullptr);
            extractor.updateGraphDigest(dstOldDigest, dstExists? graph.getVertexDigest(dstVid).data() : nullptr);
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
        start = std::chrono::high_resolution_clock::now();
        extractor.verifyKcoreGraphXOR(extractor.getXORVO());
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Time taken for verification: " << duration.count() << " ms" << std::endl << std::endl;

        // VO大小计算
        extractor.calculateXORVOSize();
        std::cout << std::endl;
    }

    return 0;
}