#include <vector>
#include <string>
#include <iostream>
#include <functional>

using namespace std;

void modifyVec(vector<int> &vec, int val, function<int(int, int)> op)
{
    for (int &v : vec)
    {
        v = op(v, val);
    }
}

template <typename T>
void printVec(const string name, vector<T> vec)
{
    cout << name << " : ";
    size_t length = vec.size();
    for (size_t i = 0; i < length; i++)
    {
        cout << vec.at(i) << " ";
    }
    cout << endl;
}

int main(int argc, char *argv[])
{
    string opStr = string(argv[1]);
    int val = atoi(argv[2]);
    vector<int> vec = {1, 2, 3, 4, 5, 10, 100, 1000};
    printVec("Original", vec);
    cout << "Performing " << opStr << " on vector with value " << val << endl;
    if (opStr == "add")
        modifyVec(vec, val, [](int x, int y)
                  { return x + y; });
    else if (opStr == "sub")
        modifyVec(vec, val, [](int x, int y)
                  { return x - y; });
    printVec("Result", vec);
    return 0;
}