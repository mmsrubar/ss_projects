/*
 * Input:
 *  Matrix A (m x n)
 *  Matrix B (n x k)
 * Output
 *  Matrix C (m x k)
 */
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <mpi.h>

#define FILE_MATRIX1 "mat1"
#define FILE_MATRIX2 "mat2"

int m;
int n;
int k;

using namespace std;

int readMatrix(ifstream &infile, vector<vector<int> > &matrix, bool matA)
{
  std::string line;
  string c;
  int i = 0;

  // read the matrix
  while (std::getline(infile, line))  // this does the checking!
  {
    std::istringstream iss(line);

    // read all value from a line
    while (iss >> c) {
      matrix[i].push_back(atoi(c.c_str()));
      if (!matA) i++;
    }

    if (matA) i++; else i = 0;
  }
}

int main(int argc, char *argv[])
{
  std::string line;
  int first_value;    // first value of a file
  int numprocs;       // how many CPU?
  int myid;           // my rank

  // MPI INIT
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  // CPU with rank 0 will read the matrices from files mat1 and mat2
  if (myid == 0) {
    // open file with matrices
    ifstream infile1(FILE_MATRIX1);
    ifstream infile2(FILE_MATRIX2);

    // read number of rows for matrix A
    getline(infile1, line);
    istringstream iss1(line);
    iss1 >> m;
    vector<vector<int> > matA(m);

    cerr << "file: " << FILE_MATRIX1 << endl;
    cerr << m << endl;
    readMatrix(infile1, matA, true);
    for (int i = 0; i < matA.size(); i++) {
      for (int j = 0; j < matA[i].size(); j++) {
        cerr << matA[i][j] << " ";
      }
      cerr << endl;
    }

    // read number of rows for matrix B 
    getline(infile2, line);
    istringstream iss2(line);
    iss2 >> k;
    vector<vector<int> > matB(k);

    cerr << "file: " << FILE_MATRIX2 << endl;
    cerr << k << endl;
    readMatrix(infile2, matB, false);
    for (int j = 0; j < matB[0].size(); j++) {
      for (int i = 0; i < matB.size(); i++) {
        cerr << matB[i][j] << " ";
      }
      cerr << endl;
    }
  }

}
