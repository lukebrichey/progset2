#include <iostream>
#include <fstream>
#include <random>

using namespace std;

int main() {
    int d;
    string prefix;
    cout << "Enter dimension of square matrices: ";
    cin >> d;
    cout << "Enter prefix for file: ";
    cin >> prefix;

    // Generate random integers between 0 and 9
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 9);

    // Open output file with prefix
    ofstream outfile(prefix + "_" + to_string(d) + ".txt");

    // Fill file with 2d^2 entries, alternating between matrices A and B
    for (int i = 0; i < d * d * 2; i++) {
        outfile << dis(gen) << endl;
    }

    outfile.close();
    cout << "File created." << endl;

    return 0;
}

