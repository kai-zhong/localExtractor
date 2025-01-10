#include "localExtractor/localExtractor.h"

localExtractor::localExtractor()
{
    answerExists = false;
}

localExtractor::~localExtractor()
{}

void localExtractor::mclInit()
{
    mcl::aggs::AGGS::init();
}

VertexID localExtractor::getLiIndexTop()
{
    std::map<uint, std::list<VertexID>>::const_reverse_iterator it = liIndex.rbegin();
    if(it == liIndex.rend())
    {
        std::cerr << "LiIndex is empty." << std::endl;
        throw std::out_of_range("LiIndex is empty.");
    }
    return it->second.front();
}

void localExtractor::liIndexPop()
{
    std::map<uint, std::list<VertexID>>::reverse_iterator it = liIndex.rbegin();
    if(it == liIndex.rend())
    {
        std::cerr << "LiIndex is empty." << std::endl;
        throw std::out_of_range("LiIndex is empty.");
    }
    liIndexExists.erase(it->second.front());
    it->second.pop_front();
    if(it->second.empty())
    {
        liIndex.erase(it->first);
    }
}

void localExtractor::initLiIndex(const VertexID& queryV)
{
    liIndex.clear();
    liIndex[0].push_back(queryV);
    liIndexExists[queryV] = 0;
}

void localExtractor::insertToLiIndex(const Graph& graph, const VertexID& vid, std::unordered_map<VertexID, bool>& visited, const uint& k)
{
    std::vector<VertexID> neighbors;
    for(const VertexID& neighbor : graph.getVertexNeighbors(vid))
    {
        if(!visited[neighbor] && graph.getVertexDegree(neighbor) >= k)
        {
            neighbors.emplace_back(neighbor);
        }
    }

    if(neighbors.empty())
    {
        return;
    }

    for(const VertexID& neighbor : neighbors)
    { 
        if(liIndexExists.find(neighbor) == liIndexExists.end()) // 未加入LiIndex
        {
            liIndex[1].push_back(neighbor);
            liIndexExists[neighbor] = 1;
        }
        else // 已加入LiIndex
        {
            uint li = liIndexExists[neighbor];
            liIndex[li].remove(neighbor);
            if(liIndex[li].empty())
            {
                liIndex.erase(li);
            }
            liIndex[li + 1].push_back(neighbor);
            liIndexExists[neighbor] = li + 1;
        }
    }
}

bool localExtractor::isLiIndexEmpty() const
{
    return liIndex.empty();
}

void localExtractor::candidateGeneration(const Graph& graph, const VertexID& queryV, const uint& k)
{
    if(!candVertices.empty())
    {
        candVertices.clear();
    }
    std::unordered_map<VertexID, bool> visited;

    for(const std::pair<VertexID, Vertex>& nodePair : graph.getNodes())
    {
        visited[nodePair.first] = false;
    }

    initLiIndex(queryV);

    while(!isLiIndexEmpty())
    {
        VertexID currentV = getLiIndexTop();
        visited[currentV] = true;
        liIndexPop();

        candGraph.addVertex(currentV, true, false);
        for(const VertexID& neighbor : graph.getVertexNeighbors(currentV))
        {
            if(visited[neighbor] == true)
            {
                candGraph.addEdge(currentV, neighbor, true, false);
            }
        }
        candVertices.emplace_back(currentV);

        // uint minDegree = candGraph.getMinDegreeWithtraversal();
        uint minDegree = candGraph.getVertexDegree(candGraph.getMinDegreeVertexID());
        if(minDegree >= k)
        {
            std::cout << "Answer exists." << std::endl;
            answerExists = true;
            break;
        }

        insertToLiIndex(graph, currentV, visited, k);
    }
}

void localExtractor::globalExtract(const VertexID& queryV, const uint& k)
{
    VertexID minDegreeV;
    uint deg;
    bool satisfyKcore = false;
    Graph kcoreG;
    while(true)
    {
        minDegreeV = candGraph.getMinDegreeVertexID();
        deg = candGraph.getVertexDegree(minDegreeV);
        if(deg < k)
        {
            if(minDegreeV == queryV)
            {
                break;
            }
            candGraph.removeVertex(minDegreeV, true, false);
        }
        else
        {
            satisfyKcore = true;
            break;
        }
    }
    if(!satisfyKcore)
    {
        std::cout << "The graph does not satisfy k-core property." << std::endl;
    }
}

void localExtractor::constructVO(const Graph& graph, const Graph& kcoreGraph)
{
    size_t n = kcoreGraph.getVertexNum();
    vo.clear();
    vo.reserve(n);
    for(const std::pair<VertexID, Vertex>& nodePair : kcoreGraph.getNodes())
    {
        VertexID vid = nodePair.first;
        Vertex v = graph.getVertex(vid);
        vo.emplace_back(serializeVertexInfo(v));
    }
}

Graph localExtractor::kcoreExtract(const Graph& graph, const VertexID& queryV, const uint& k)
{
    if(!answerExists)
    {
        answerExists = false;
    }

    candidateGeneration(graph, queryV, k);

    if(!answerExists)
    {
        std::cout << "Start global extraction." << std::endl;
        candGraph.buildInvertedIndex();
        globalExtract(queryV, k);
    }

    constructVO(graph, candGraph);

    return candGraph;
}

const Graph& localExtractor::getCandGraph() const
{
    return candGraph;
}

std::string localExtractor::serializeVertexInfo(const Vertex& v)
{
    unsigned char splitter = '/';
    std::ostringstream oss = std::ostringstream();

    VertexID vid = v.getVid();
    const std::vector<VertexID>& neighbors = v.getNeighbors();

    std::string vidStr;

    oss << vid;
    for(const VertexID& neighbor : neighbors)
    {
        oss << splitter << neighbor;
    }
    
    vidStr = oss.str();
    return vidStr;
}

VertexID localExtractor::getSerializedVertexID(const std::string& serializedInfo)
{
    std::istringstream iss(serializedInfo);
    std::string vidStr;
    char splitter = '/';
    std::getline(iss, vidStr, splitter); // 按 '/' 分隔，读取第一个部分
    return static_cast<unsigned int>(std::stoul(vidStr));
}

void localExtractor::signGraph(const Graph& graph)
{
    mcl::aggs::AGGS::init();

    const size_t n = graph.getVertexNum();
    vertexSKeys.clear();
    vertexPKeys.clear();
    vertexSignatures.clear();
    vertexSKeys.reserve(n);
    vertexPKeys.reserve(n);
    vertexSignatures.reserve(n);

    for(const std::pair<VertexID, Vertex>& nodePair : graph.getNodes())
    {
        VertexID vid = nodePair.first;
        mcl::aggs::SecretKey skey;
        mcl::aggs::PublicKey pkey;
        mcl::aggs::Signature sig;

        skey.init();
        skey.getPublicKey(pkey);
        skey.sign(sig, serializeVertexInfo(nodePair.second));

        vertexSKeys[vid] = skey;
        vertexPKeys[vid] = pkey;
        vertexSignatures[vid] = sig;
    }
}

void localExtractor::signVertex(const Vertex& v)
{
    VertexID vid = v.getVid();
    mcl::aggs::SecretKey skey;
    mcl::aggs::PublicKey pkey;
    mcl::aggs::Signature sig;
    skey.init();
    skey.getPublicKey(pkey);
    skey.sign(sig, serializeVertexInfo(v));

    vertexSKeys[vid] = skey;
    vertexPKeys[vid] = pkey;
    vertexSignatures[vid] = sig;
}

void localExtractor::signAddUpdate(const Vertex& src, const Vertex& dst)
{
    signVertex(src);
    signVertex(dst);
}

void localExtractor::signDeleteEdgeUpdate(const Vertex& v)
{
    signVertex(v);
}

void localExtractor::signDeleteVertexUpdate(const VertexID& vid)
{
    vertexSKeys.erase(vid);
    vertexPKeys.erase(vid);
    vertexSignatures.erase(vid);
}

void localExtractor::updateVertexSignature(const Vertex& v)
{
    VertexID vid = v.getVid();
    if(vertexSignatures.find(vid) == vertexSignatures.end())
    {
        mcl::aggs::SecretKey skey;
        mcl::aggs::PublicKey pkey;
        mcl::aggs::Signature sig;

        skey.init();
        skey.getPublicKey(pkey);
        skey.sign(sig, serializeVertexInfo(v));

        vertexSKeys[vid] = skey;
        vertexPKeys[vid] = pkey;
        vertexSignatures[vid] = sig;
    }
    else
    {
        mcl::aggs::Signature sig;
        vertexSKeys[vid].sign(sig, serializeVertexInfo(v));
        vertexSignatures[vid] = sig;
    }
}

void localExtractor::verifyKcoreGraph(const std::vector<std::string>& VO)
{
    size_t n = VO.size();
    std::vector<mcl::aggs::Signature> voSigs(n);
    std::vector<mcl::aggs::PublicKey> voPKeys(n);
    mcl::aggs::Signature aggSig;

    for(size_t i = 0; i < n; i++)
    {
        VertexID vid = getSerializedVertexID(VO[i]);
        voSigs[i] = vertexSignatures[vid];
        voPKeys[i] = vertexPKeys[vid];
    }
    aggSig.aggregate(voSigs);

    bool ok = aggSig.verify(VO, voPKeys);
    std::cout << "Verification result: " << (ok ? "OK" : "NG") << std::endl;
}

const std::vector<std::string>& localExtractor::getVO() const
{
    return vo;
}

void localExtractor::calculateVOSize() const
{
    size_t totalSize = sizeof(vo); // vector 本身的大小
    for (const auto& str : vo) 
    {
        totalSize += sizeof(str);       // 每个 std::string 对象的固定大小
        totalSize += str.capacity();   // 加上字符串实际分配的容量
    }

    // 输出结果
    std::cout << "Size of VO:" << std::endl;
    std::cout << "  Bytes: " << totalSize << " bytes" << std::endl;
    std::cout << "  Kilobytes: " << totalSize / 1024.0 << " KB" << std::endl;
    std::cout << "  Megabytes: " << totalSize / (1024.0 * 1024.0) << " MB" << std::endl;
}

void localExtractor::printLiIndex() const
{
    if(liIndex.empty())
    {
        std::cout << "LiIndex is empty." << std::endl;
        return;
    }
    for (const auto& pair : liIndex)
    {
        std::cout << "Key: " << pair.first << ", Values: ";
        for (const auto& value : pair.second)
        {
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}
