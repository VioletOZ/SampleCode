#include <iostream>
#include <vector>
#include <queue>


class Node
{
public:
    Node(int n_, int m_, int v_)
    {
        N = n_;
        M = m_;
        V = v_;

    }

    // ±í
    void DFS(int x)
    {
        visited[x] = true;
        std::cout << x << std::endl;

        for (int i = 0; i < graph[x].size(); i++)
        {
            int y = graph[x][i];
            if (!visited[y])
                DFS(y);
        }

    };

    // ³Ð
    void BFS(int x)
    {
        memset(visited, 0, sizeof(visited));

        visited[x] = true;
        que.push(x);
        
        while (!que.empty())
        {
            std::cout << que.front() << std::endl;
            que.pop();
            for (int i = 0; i < graph[x].size(); i++)
            {
                int y = graph[x][i];
                if (!visited[y])
                {
                    visited[y] = true;
                    que.push(y);
                }
            }
        }

    }

    void SetGraph(int a, int b)
    {
        graph[a].push_back(b);
    }

    void GetGraph()
    {
        for (int i = 0; i < 10; i++)
        {
            for (std::vector<int>::iterator it = graph[i].begin(); it < graph[i].end(); it++)
            {
                std::cout << *it;
            }
            std::cout << "" << std::endl;
        }
    }

private:
    int N, M, V = { 0, };

    bool visited[9] = { 0, };        // ¹æ¹®ÇÑ À§Ä¡ 
    std::vector<int> graph[10];      // ±×·¡ÇÁ
    std::queue<int> que;

};
