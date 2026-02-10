#include <iostream>
#include <vector>

void DFS(int node, std::vector<std::vector<int>>& adj, std::vector<bool>& visited)
{
	visited[node] = true;
	std::cout << node << std::endl;

	for (int neigh : adj[node])
	{
		if (!visited[neigh])
		{
			DFS(neigh, adj, visited);
		}
	}
}

int main()
{
	int n = 100;
	std::vector<std::vector<int>> adj(n);

	adj[0].push_back(1);
	adj[0].push_back(2);
	adj[0].push_back(3);
	adj[0].push_back(4);
	adj[0].push_back(5);
	

	std::vector<bool> visited(n, false);

	DFS(0, adj, visited);

	return;
}