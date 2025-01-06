#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <list>

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

    public:
        localExtractor();
        ~localExtractor();

        VertexID getLiIndexTop();

        void liIndexPop();

        void initLiIndex(const VertexID& queryV);

        void insertToLiIndex(const Graph& graph, const VertexID& vid, std::unordered_map<VertexID, bool>& visted, const uint& k);

        bool isLiIndexEmpty() const;

        void candidateGeneration(const Graph& graph, const VertexID& queryV, const uint& k);

        void globalExtract(const VertexID& queryV, const uint& k);

        Graph kcoreExtract(const Graph& graph, const VertexID& queryV, const uint& k);

        const Graph& getCandGraph() const;

        void printLiIndex() const;
};