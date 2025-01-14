#include "localExtractor/localExtractor.h"

localExtractor::localExtractor()
{
    answerExists = false;
}

localExtractor::~localExtractor()
{}

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

        candGraph.addVertex(currentV, true, true);
        for(const VertexID& neighbor : graph.getVertexNeighbors(currentV))
        {
            if(visited[neighbor] == true)
            {
                candGraph.addEdge(currentV, neighbor, true, true);
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
            candGraph.removeVertex(minDegreeV, true, true);
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

void localExtractor::constructXORVO(const Graph& graph, const Graph& kcoreGraph)
{
    size_t n = kcoreGraph.getVertexNum();
    std::array<unsigned char, SHA256_DIGEST_LENGTH> gd;
    std::vector<std::string> originGraphVertexInfo;
    gd = graphDigest;
    for(const std::pair<VertexID, Vertex>& nodePair : kcoreGraph.getNodes())
    {
        VertexID vid = nodePair.first;
        Vertex v = graph.getVertex(vid);
        std::string vertexInfo = serializeVertexInfo(v);
        std::array<unsigned char, SHA256_DIGEST_LENGTH> vertexDigestArr = v.getDigest();
        for(size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        {
            gd[i] ^= v.getDigest()[i];
        }
        originGraphVertexInfo.emplace_back(vertexInfo);
    }
    xorVO = std::make_pair(gd, originGraphVertexInfo);
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
        // candGraph.buildInvertedIndex();
        globalExtract(queryV, k);
    }

    constructXORVO(graph, candGraph);
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

void localExtractor::generateGraphDigest(const Graph& graph)
{
    bool start = true;
    for(const std::pair<VertexID, Vertex>& nodePair : graph.getNodes())
    {
        if(start == true)
        {
            start = false;
            graphDigest = nodePair.second.getDigest();
            continue;
        }
        Vertex v = nodePair.second;
        std::array<unsigned char, SHA256_DIGEST_LENGTH> vertexDigestArr = v.getDigest();
        for(size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        {
            graphDigest[i] ^= vertexDigestArr[i];
        }
    }
}

void localExtractor::updateGraphDigest(const unsigned char* oldVDigest, const unsigned char* newVDigest)
{
    if(oldVDigest == nullptr) // 新增顶点
    {
        for(size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        {
            graphDigest[i] ^= newVDigest[i];
        }
        return ;
    }
    if(newVDigest == nullptr) // 删除顶点
    {
        for(size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        {
            graphDigest[i] ^= oldVDigest[i];
        }
        return ;
    }
    for(size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    {
        graphDigest[i] ^= oldVDigest[i] ^ newVDigest[i];
    }
}

void localExtractor::verifyKcoreGraphXOR(const std::pair<std::array<unsigned char, SHA256_DIGEST_LENGTH>, std::vector<std::string>>& VO)
{
    std::array<unsigned char, SHA256_DIGEST_LENGTH> gd = xorVO.first;

    for(const std::string& str : VO.second)
    {
        unsigned char vertexDigest[SHA256_DIGEST_LENGTH];
        SHA256_CTX ctx;
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, (unsigned char*)str.c_str(), str.size());
        SHA256_Final(vertexDigest, &ctx);
        for(size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        {
            gd[i] ^= vertexDigest[i];
        }
    }

    bool ok = gd == graphDigest;
    std::cout << "Verification result: " << (ok ? "OK" : "NG") << std::endl;
}

const std::pair<std::array<unsigned char, SHA256_DIGEST_LENGTH>, std::vector<std::string>>& localExtractor::getXORVO() const
{
    return xorVO;
}

void localExtractor::calculateXORVOSize() const
{
    size_t totalSize = sizeof(std::array<unsigned char, SHA256_DIGEST_LENGTH>) + SHA256_DIGEST_LENGTH;
    totalSize += sizeof(std::vector<std::string>);
    for(const auto& str : xorVO.second)
    {
        totalSize += sizeof(str);
        totalSize += str.capacity();
    }

    std::cout << "Size of XORVO:" << std::endl;
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
