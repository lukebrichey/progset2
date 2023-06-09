#include <Eigen/Dense>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <memory>
#include <chrono>
#include <random>

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

MatrixXi padMatrix(const MatrixXi &M) {
    int rows = M.rows();
    int cols = M.cols();
    int padded_rows = 1;
    int padded_cols = 1;

    while (padded_rows < rows) {
        padded_rows *= 2;
    }

    while (padded_cols < cols) {
        padded_cols *= 2;
    }

    MatrixXi padded(padded_rows, padded_cols);
    padded.setZero();
    padded.topLeftCorner(rows, cols) = M;
    return padded;
}

MatrixXi unpadMatrix(const MatrixXi &M) {
    return M.topLeftCorner(M.rows() - 1, M.cols() - 1);
}


MatrixXi strassMult(const MatrixXi &A, const MatrixXi &B, int n) {
    if (n <= 64 || (n != (n & -n))) { // Base case: use standard matrix multiplication for small matrices or non-power of 2 sizes
        return regMult(A, B, n);
    }
    
    bool pad_required = false;
    if (n % 2 != 0) {
        n += 1;
        pad_required = true;
    }

    MatrixXi padded_A = A, padded_B = B;
    if (pad_required) {
        padded_A = padMatrix(A);
        padded_B = padMatrix(B);
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

    if (pad_required) {
        return unpadMatrix(C);
    }

    return C;
}

MatrixXi generate_random_graph(double p) {
    MatrixXi adjacency_matrix(1024, 1024);
    adjacency_matrix.setZero();

    // Use a random number generator with a seed based on the current time.
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::uniform_real_distribution<double> distribution(0.0, 1.0);

    for (int i = 0; i < 1024; i++) {
        for (int j = 0; j < i; j++) {
            double rand_val = distribution(generator);
            if (rand_val < p) {
                adjacency_matrix(i, j) = 1;
                adjacency_matrix(j, i) = 1;
            }
        }
    }

    return adjacency_matrix;

}

int main(int argc, char *argv[]) {
    // Usage: ./strassen <mode> <matrix size> inputfile 
    // mode = 0: autograder
    // mode = 1: debugger
    // mode = 2: triangles

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
        case 2:
        {
            std::vector<double> probabilities = {0.01, 0.02, 0.03, 0.04, 0.05};

            for (const double p : probabilities) {
                double avg = 0;
                for (int i = 0; i < 10; i++) {
                    std::cout << "Generating graph for p = " << p << std::endl;
                    MatrixXi graph = generate_random_graph(p);
                    MatrixXi A2 = strassMult(graph, graph, 1024);
                    MatrixXi A3 = strassMult(A2, graph, 1024);

                    // Calculate the number of triangles in the graph
                    double num_triangles = 0;
                    int diag_sum = 0;
                    // Add diagonal entries
                    for (int i = 0; i < 1024; i++) {
                        for (int j = 0; j < 1024; j++) {
                            if (i == j) {
                                diag_sum += A3(i, j);
                            }
                        }
                    }

                    std::cout << "Diagonal sum: " << diag_sum << std::endl;

                    num_triangles = diag_sum / 6;
                    avg += num_triangles;
                }
                avg /= 10;
                std::cout << "There are " << avg << "in the graph with p = " << p << std::endl;
            }

            break;
        }
        default:
            std::cerr << "Invalid mode." << std::endl;
            return 1;
    }

    return 0;
}
