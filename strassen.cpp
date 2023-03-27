#include <stdio.h>
#include <stdlib.h>
#include <Eigen/Dense>
#include <pthread.h>
#include <iostream>
#include <fstream>

using namespace Eigen;

MatrixXi regMult(const MatrixXi &A, const MatrixXi &B,int n) {
    // Perform matrix multiplication using Eigen's .row(), .col() and .dot() methods
    MatrixXi C(n, n);
    C.setZero();

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            C(i, j) = A.row(i).dot(B.col(j));
        }
    }

    return C;
}

MatrixXi strassMult(const MatrixXi &A, const MatrixXi &B, int n) {
    if (n <= 64) { // Base case: use standard matrix multiplication for small matrices
        return regMult(A, B, n);
    }

    int half = n / 2;

    MatrixXi A11 = A.topLeftCorner(half, half);
    MatrixXi A12 = A.topRightCorner(half, half);
    MatrixXi A21 = A.bottomLeftCorner(half, half);
    MatrixXi A22 = A.bottomRightCorner(half, half);

    MatrixXi B11 = B.topLeftCorner(half, half);
    MatrixXi B12 = B.topRightCorner(half, half);
    MatrixXi B21 = B.bottomLeftCorner(half, half);
    MatrixXi B22 = B.bottomRightCorner(half, half);

    MatrixXi P1 = strassMult(A11 + A22, B11 + B22, half);
    MatrixXi P2 = strassMult(A21 + A22, B11, half);
    MatrixXi P3 = strassMult(A11, B12 - B22, half);
    MatrixXi P4 = strassMult(A22, B21 - B11, half);
    MatrixXi P5 = strassMult(A11 + A12, B22, half);
    MatrixXi P6 = strassMult(A21 - A11, B11 + B12, half);
    MatrixXi P7 = strassMult(A12 - A22, B21 + B22, half);

    MatrixXi C11 = P1 + P4 - P5 + P7;
    MatrixXi C12 = P3 + P5;
    MatrixXi C21 = P2 + P4;
    MatrixXi C22 = P1 - P2 + P3 + P6;

    MatrixXi C(n,n);

    C << C11, C12,
         C21, C22;

    return C;
}


int main(int argc, char *argv[]) {
    // Usage: ./strassen <mode> <matrix size> inputfile 
    // mode = 0: autograder
    int mode = atoi(argv[1]);
    // read in the matrix size (n)
    int n = atoi(argv[2]);

    // create a matrix of size n x n
    MatrixXi A(n,n);
    MatrixXi B(n,n);

    // read in input from input ASCII file (2n^2 entries, one number per line)
    // first n^2 entries are for A, last n^2 entries are for B
    std::ifstream inputFile(argv[3]);

    if (!inputFile.is_open()) {
        std::cerr << "Failed to open the file." << std::endl;
        return 1;
    }

    // Fill A, converting ASCII to integer
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            inputFile >> A(i, j);
        }
    }

    // Fill B
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            inputFile >> B(i, j);
        }
    }

    inputFile.close();

    // Correct result matrix
    MatrixXi correct(n,n);
    correct = A * B;

    // Initialize the result matrix
    MatrixXi C(n, n);
    C.setZero();

    // Multiply A and B using Strassen's algorithm
    C = strassMult(A, B, n);

    // Print the result matrix
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) {
                std::cout << C(i, j) << std::endl;
            }
        }
    }
    std::cout << std::endl;
    
    // Confirm that result is correct
    if (C.isApprox(correct)) {
        std::cout << "Correct!" << std::endl;
    } else {
        std::cout << "Incorrect!" << std::endl;
    }

    return 0;
}
