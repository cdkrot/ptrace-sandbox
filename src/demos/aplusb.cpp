#include <fstream>

using namespace std;

int main() {
    ifstream in("res/aplusb.in");
    ofstream out("res/aplusb.out");

    int a, b;
    in >> a >> b;
    out << a + b << "\n";
    return 0;
}
