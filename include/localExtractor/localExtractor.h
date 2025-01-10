#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <list>
#include <string>

#include <mcl/aggregate_sig.hpp>

#include "../graph/graph.h"
#include "../graph/vertex.h"
#include "../util/common.h"
#include "../configuration/types.h"

class Vertex;
class Graph;

class localExtractor
{
    private:
        std::vector<VertexID> candVertices;
        Graph candGraph;

        std::map<uint, std::list<VertexID>> liIndex;
        std::unordered_map<VertexID, uint> liIndexExists;
        bool answerExists;

        std::unordered_map<VertexID, mcl::aggs::SecretKey> vertexSKeys;
        std::unordered_map<VertexID, mcl::aggs::PublicKey> vertexPKeys;
        std::unordered_map<VertexID, mcl::aggs::Signature> vertexSignatures;

        std::vector<std::string> vo;
    public:
        localExtractor();
        ~localExtractor();

        void mclInit();

        VertexID getLiIndexTop();

        void liIndexPop();

        void initLiIndex(const VertexID& queryV);

        void insertToLiIndex(const Graph& graph, const VertexID& vid, std::unordered_map<VertexID, bool>& visted, const uint& k);

        bool isLiIndexEmpty() const;

        void candidateGeneration(const Graph& graph, const VertexID& queryV, const uint& k);

        void globalExtract(const VertexID& queryV, const uint& k);

        void constructVO(const Graph& graph, const Graph& kcoreGraph);

        Graph kcoreExtract(const Graph& graph, const VertexID& queryV, const uint& k);

        const Graph& getCandGraph() const;

        std::string serializeVertexInfo(const Vertex& v);

        VertexID getSerializedVertexID(const std::string& serializedInfo);

        void signGraph(const Graph& graph);

        void signVertex(const Vertex& v);

        void signAddUpdate(const Vertex& src, const Vertex& dst);

        void signDeleteEdgeUpdate(const Vertex& v); // 节点未被删除，但是节点信息发生改变，需要更新节点的签名

        void signDeleteVertexUpdate(const VertexID& vid); // 节点被删除，需要删除节点的签名

        void updateVertexSignature(const Vertex& v);

        void verifyKcoreGraph(const std::vector<std::string>& VO);

        const std::vector<std::string>& getVO() const;

        void calculateVOSize() const;

        void printLiIndex() const;
};