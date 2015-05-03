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

#define FILE_MATRIX1 "mat1"
#define FILE_MATRIX2 "mat2"


int m;
int n;
int k;

using namespace std;

int readMatrix(const char *fname)
{
  cout << "file: " << fname << endl;
  std::string line;
  string name(fname);
  std::ifstream infile(fname);

  // read first line which represents the number of rows in mat1 or number of
  // columbs in case of mat2
  std::getline(infile, line);
  std::istringstream iss(line);

  if (name.compare("mat1") == 0)
    iss >> m;
  else
    iss >> k;
  cout << line << endl;

  // read the matrix
  while (std::getline(infile, line))  // this does the checking!
  {
    std::istringstream iss(line);

    string c;
    int num;

    // read all value from a line
    while (iss >> c)
    {
      cout << c << " ";
      num = atoi( c.c_str() );  // convert string to int
    }

    cout << endl;
  }
}

int main() {
  readMatrix("mat1");
  readMatrix("mat2");


  cout << "matA has " << m << " rows" << endl;
  vector<vector<int> > matA(m);
}
