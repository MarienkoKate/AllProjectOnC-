#define _CRT_SECURE_NO_WARNINGS 
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
using namespace std;
void print_(queue<int> v) {
	queue<int> v1 = v;
	while (!v1.empty())
	{
		int val = v1.front();
		v1.pop();
		cout << val << " ";
	}
	cout << endl;
}
void print__(vector<int> v) {
	for (auto it = v.begin(); it != v.end(); ++it) cout << *it << " ";
}
std::vector<int> from_string(std::string str) {
	std::vector<int> list;

	size_t size = str.length() + 1;
	char* buff = new char[size];

	strcpy(buff, str.c_str());
	buff[str.length()] = '\0';

	char* pch = strtok(buff, " ,.-		");

	while (pch != NULL)
	{

		int Int = pch[0] - '0';
		for (int i = 1; pch[i] != '\0'; i++) {
			if (pch[i + 1] == '\0') break;
			Int = Int * 10 + pch[i] - '0';
		}

		list.push_back(Int);

		pch = strtok(NULL, " ,.-");

	}

	return list;
}

std::vector<std::vector<int>> file(string file_name, int* vertex) {
	std::vector<std::vector<int>> v;
	std::ifstream file(file_name);
	std::string str, name;
	int j = 0;
	vector<int> vv;

	if (!file.is_open())
		std::cout << "error\n";
	else {
		while (true) {

			if (!file.eof()) {

				std::getline(file, str);

				if (j == 0) {
					name = str;
					*vertex = stoi(name);

				}
				else if (str == "-") {
					v.resize(j);
				}
				else {
					v.push_back(from_string(str));
				}
				j++;
				str.clear();
			}
			else break;
		}

		file.close();

		return v;
	}
}

int n = 5;
void bfs(int s, std::vector<std::vector<int>> list) {

	std::queue<int> q;
	q.push(s);
	std::vector<bool> used(n);
	std::vector<int> d(n), p(n);
	used[s] = true;
	p[s] = -1;
	while (!q.empty()) {
		int v = q.front();
		q.pop();
		for (size_t i = 0; i < list[v].size(); ++i) {
			int to = list[v][i];

			if (!used[to]) {
				used[to] = true;
				q.push(to);
				d[to] = d[v] + 1;
				p[to] = v;
			}
		}
	}
	for (int to = 0; to != n; to++) {
		if (!used[to])
			std::cout << "No path!";
		else {
			std::vector<int> path;
			for (int v = to; v != -1; v = p[v])
				path.push_back(v);
			reverse(path.begin(), path.end());
			std::cout << "Path to " << to << ": ";
			for (size_t i = 0; i < path.size(); ++i)
				std::cout << path[i] << " ";

		}
		std::cout << std::endl;
	}
}

int main() {

	int vertex = 0;
	std::vector<std::vector<int>> vv = file("C://lab18.txt", &vertex);
	/*for (int i = 0; i != 6; i++) {
		print__(vv[i]);
		cout << endl;
	}*/

	bfs(vertex, vv);
}