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


    // bool found = false;
    // VertexID topVid;
    // if(isLiIndexEmpty())
    // {
    //     std::cerr << "LiIndex is empty." << std::endl;
    //     throw std::out_of_range("LiIndex is empty.");
    // }
    // for(std::map<uint, std::list<VertexID>>::reverse_iterator it = liIndex.rbegin(); it!= liIndex.rend();)
    // {
    //     uint li = it->first;
    //     std::list<VertexID>& vidList = it->second;
    //     // std::cout << "li: " << li << std::endl;
    //     while(!vidList.empty())
    //     {
    //         // std::cout << "vidList.front(): " << vidList.front() << std::endl;
    //         VertexID vid = vidList.front();
    //         if(liIndexExists.find(vid) == liIndexExists.end() || liIndexExists.at(vid) != li)
    //         {
    //             vidList.pop_front();
    //         }
    //         else
    //         {
    //             found = true;
    //             topVid = vid;
    //             break;
    //         }
    //     }
    //     if(found)
    //     {
    //         break;
    //     }
    //     else
    //     {
    //         if(vidList.empty())
    //         {
    //             it = std::map<uint, std::list<VertexID>>::reverse_iterator(liIndex.erase(std::next(it).base()));
    //         }
    //         else
    //         {
    //             it++;
    //         }
    //     }
    // }
    // return topVid;
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

    return candGraph;
}

const Graph& localExtractor::getCandGraph() const
{
    return candGraph;
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
