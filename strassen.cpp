#include <Eigen/Dense>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <memory>
#include <chrono>

using namespace Eigen;

struct ThreadArgs {
    MatrixXi A;
    MatrixXi B;
    MatrixXi *result;
    int n;
};

MatrixXi regMult(const MatrixXi &A, const MatrixXi &B, int n) {
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

MatrixXi strassMult(const MatrixXi &A, const MatrixXi &B, int n);

void *threadedStrassMult(void *args) {
    ThreadArgs *arguments = (ThreadArgs *)args;
    int n = arguments->n;
    *(arguments->result) = strassMult(arguments->A, arguments->B, n);
    return NULL;
}

MatrixXi strassMult(const MatrixXi &A, const MatrixXi &B, int n) {
    if (n <= 64) { // Base case: use standard matrix multiplication for small matrices
        return regMult(A, B, n);
    }

    int half = n / 2;

    // Allocate submatrices on the heap
    auto A11 = std::make_unique<MatrixXi>(A.topLeftCorner(half, half));
    auto A12 = std::make_unique<MatrixXi>(A.topRightCorner(half, half));
    auto A21 = std::make_unique<MatrixXi>(A.bottomLeftCorner(half, half));
    auto A22 = std::make_unique<MatrixXi>(A.bottomRightCorner(half, half));

    auto B11 = std::make_unique<MatrixXi>(B.topLeftCorner(half, half));
    auto B12 = std::make_unique<MatrixXi>(B.topRightCorner(half, half));
    auto B21 = std::make_unique<MatrixXi>(B.bottomLeftCorner(half, half));
    auto B22 = std::make_unique<MatrixXi>(B.bottomRightCorner(half, half));

    // Allocate result matrices on the heap
    auto P1 = std::make_unique<MatrixXi>(half, half);
    auto P2 = std::make_unique<MatrixXi>(half, half);
    auto P3 = std::make_unique<MatrixXi>(half, half);
    auto P4 = std::make_unique<MatrixXi>(half, half);
    auto P5 = std::make_unique<MatrixXi>(half, half);
    auto P6 = std::make_unique<MatrixXi>(half, half);
    auto P7 = std::make_unique<MatrixXi>(half, half);

    pthread_t threads[7];
    ThreadArgs threadArgs[7];

    for (int i = 0; i < 7; ++i) {
        threadArgs[i].n = half;
    }

    threadArgs[0] = {(*A11.get()) + (*A22.get()), (*B11.get()) + (*B22.get()), P1.get(), half};
    pthread_create(&threads[0], NULL, threadedStrassMult, (void *)&threadArgs[0]);

    threadArgs[1] = {(*A21.get()) + (*A22.get()), *B11.get(), P2.get(), half};
    pthread_create(&threads[1], NULL, threadedStrassMult, (void *)&threadArgs[1]);

    threadArgs[2] = {*A11.get(), (*B12.get()) - (*B22.get()), P3.get(), half};
    pthread_create(&threads[2], NULL, threadedStrassMult, (void *)&threadArgs[2]);

    threadArgs[3] = {*A22.get(), (*B21.get()) - (*B11.get()), P4.get(), half};
    pthread_create(&threads[3], NULL, threadedStrassMult, (void *)&threadArgs[3]);

    threadArgs[4] = {(*A11.get()) + (*A12.get()), *B22.get(), P5.get(), half};
    pthread_create(&threads[4], NULL, threadedStrassMult, (void *)&threadArgs[4]);

    threadArgs[5] = {(*A21.get()) - (*A11.get()), (*B11.get()) + (*B12.get()), P6.get(), half};
    pthread_create(&threads[5], NULL, threadedStrassMult, (void *)&threadArgs[5]);

    threadArgs[6] = {(*A12.get()) - (*A22.get()), (*B21.get()) + (*B22.get()), P7.get(), half};
    pthread_create(&threads[6], NULL, threadedStrassMult, (void *)&threadArgs[6]);

    for (int i = 0; i < 7; i++) {
        pthread_join(threads[i], NULL);
    }

    MatrixXi C11 = *P1 + *P4 - *P5 + *P7;
    MatrixXi C12 = *P3 + *P5;
    MatrixXi C21 = *P2 + *P4;
    MatrixXi C22 = *P1 - *P2 + *P3 + *P6;

    MatrixXi C(n, n);

    C << C11, C12,
         C21, C22;

    return C;
}


int main(int argc, char *argv[]) {
    // Usage: ./strassen <mode> <matrix size> inputfile 
    // mode = 0: autograder
    // mode = 1: debugger

    if (argc != 4) {
        std::cerr << "Usage: ./strassen <mode> <matrix size> inputfile" << std::endl;
        return 1;
    }

    int mode = atoi(argv[1]);
    // Read in the matrix size (n)
    int n = atoi(argv[2]);

    // Create a matrix of size n x n
    MatrixXi A(n,n);
    MatrixXi B(n,n);

    // Read in the input file
    std::ifstream inputFile(argv[3]);

    if (!inputFile.is_open()) {
        std::cerr << "Failed to open the file." << std::endl;
        return 1;
    }

    // Fill A
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

    // Initialize the result matrix
    MatrixXi C(n, n);
    C.setZero();

    switch (mode) {
        case 0:
        {
            // Run the algorithm
            C = strassMult(A, B, n);

            // Print the diagonal elements
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    if (i == j) {
                        std::cout << C(i, j) << std::endl;
                    }
                }
            }
            std::cout << std::endl;    
            break;
        }
        case 1:
        {
            auto start = std::chrono::high_resolution_clock::now();

            // Run the algorithm
            C = strassMult(A, B, n);

            auto end = std::chrono::high_resolution_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            std::cout << "Time taken by function: " << duration.count() << " microseconds" << std::endl;
            
            // Check the result        
            MatrixXi correct(n, n);
            correct = A * B;
            if (C.isApprox(correct)) {
                std::cout << "Correct!" << std::endl;
            } else {
                std::cout << "Incorrect!" << std::endl;
            }
            break;
        }
        default:
            std::cerr << "Invalid mode." << std::endl;
            return 1;
    }

    return 0;
}
