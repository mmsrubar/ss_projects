/*
 * Project:     Implementation of Pipeline Merge Sort algorithm
 * Seminar:     Parallel and Distributed Algorithms
 * Author:      Michal Srubar, xsruba03@stud.fit.vutbr.cz
 * Date:        Sat May  2 23:11:20 CEST 2015
 */


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
#include <unistd.h>

#define FILE_MATRIX1 "mat1"
#define FILE_MATRIX2 "mat2"

#define is_cpu_on_left_edge(num)    ((num % k) == 0)
#define is_cpu_on_right_edge(num)   (((num+1) % k) == 0)
#define is_cpu_on_top_edge(num)     (num < k)
#define is_cpu_on_bottom_edge(num)  (num >= (m-1) * k)
#define is_cpu_last(num)            (num == m*k-1)
#define is_cpu_first(num)           (num == 0)

using namespace std;

int m;
int n;
int k;

void readMatrixParams(int *n, int *m, int *k)
{
  string line;
  ifstream infile1(FILE_MATRIX1);
  ifstream infile2(FILE_MATRIX2);

  // read number of rows for matrix A
  getline(infile1, line);
  istringstream iss1(line);
  iss1 >> *m;

  // read number of cols for matrix B
  getline(infile2, line);
  istringstream iss2(line);
  iss2 >> *k;

  *n = 0;
  string c;
  getline(infile1, line);
  istringstream iss3(line);
  while (iss3 >> c)
    (*n)++;
}

int sendMatrixAVals(int k)
{
  std::string line;
  string c;
  int i = 0;
  int cpu_idx = 0;
  int line_idx = 0;
  int mat_idx;
  MPI_Request req;
  vector<int> values;

  // open file with matrices
  ifstream infile(FILE_MATRIX1);
  cerr << "file: " << FILE_MATRIX1 << endl;

  // skip first line
  getline(infile, line);

  // read the matrix
  while (std::getline(infile, line))  // this does the checking!
  {
    istringstream iss(line);
    vector<int> values;
    int num_vals = 0;

    // read all values from a line
    while (iss >> c) {
      values.push_back(atoi(c.c_str()));
    }

    // read vector from the end to the begining
    vector<int>::reverse_iterator i;
    for (i = values.rbegin(); i != values.rend(); ++i) { 
      int value = *i;
      cerr << "send: "<< value <<" to cpu: " << cpu_idx << endl;
      MPI_Isend(&value, 1, MPI_INT, cpu_idx, 0, MPI_COMM_WORLD, &req); 
    }

    line_idx++;
    cpu_idx += k;
  }
}

int sendMatrixBVals(int k)
{
  string line;
  string c;
  int i = 0;
  int cpu_idx = 0;
  int line_idx = 0;
  int matrix_idx;
  int mat_idx;
  MPI_Request req;

  // open file with matrices
  ifstream infile(FILE_MATRIX2);
  cerr << "file: " << FILE_MATRIX2 << endl;

  // skip first line
  getline(infile, line);

  vector<vector<int> > matrix(k);
  // read the matrix
  while (std::getline(infile, line))
  {
    istringstream iss(line);
    matrix_idx = 0;

    // read all values from a line
    while (iss >> c) {
      matrix[matrix_idx].push_back(atoi(c.c_str()));
      matrix_idx++;
    }
  }

  // read vector from the end to the begining 
  for (int j = 0; j < matrix.size(); j++) {
    vector<int>::reverse_iterator i;
    for (i = matrix[j].rbegin(); i != matrix[j].rend(); ++i ) { 
      int value = *i;
      cerr << "send: "<< value <<" to cpu: " << cpu_idx << endl;
      MPI_Isend(&value, 1, MPI_INT, cpu_idx, 0, MPI_COMM_WORLD, &req); 
    }
    cpu_idx++;
  }
}

int recv_val_a(int proc_num, int idx, vector<int>& input)
{
  int val;
  MPI_Status stat;

  if (is_cpu_first(proc_num) || is_cpu_on_left_edge(proc_num)) {
    val = input[idx];
    cerr << "CPU" << proc_num << " received val a " << val << " from buf["<< idx<< endl;
  }
  else {
    // such CPU has always his left neighbour
    MPI_Recv(&val, 1, MPI_INT, proc_num-1, 0, MPI_COMM_WORLD, &stat);
    cerr << "CPU" << proc_num << " received " << val << " from "<< proc_num-1 << endl;
  }

  return val;
}

int recv_val_b(int proc_num, int idx, vector<int>& input)
{
  int val;
  MPI_Status stat;

  if (is_cpu_first(proc_num)) {
    // CPU 0 always reads values of matrix B from MPI
    MPI_Recv(&val, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);
    cerr << "CPU" << proc_num << " received " << val << " from 0"<< endl;
  }
  else if (is_cpu_on_top_edge(proc_num)) {
    val = input[idx];
    cerr << "CPU" << proc_num << " received val b " << val << " from buf["<< idx<< endl;
  }
  else {
    // such CPU has always neighbour under it
    MPI_Recv(&val, 1, MPI_INT, proc_num-k, 0, MPI_COMM_WORLD, &stat);
    cerr << "CPU" << proc_num << " received " << val << " from "<< proc_num-k << endl;
  }

  return val;
}

void send_vals_ab(int proc_num, int a, int b)
{
  if (is_cpu_last(proc_num))
    return;   // last CPU don't has no neighbours
  else if (is_cpu_on_right_edge(proc_num)) {
    cerr << "CPU" << proc_num << " sending " <<b<< " to "<< proc_num+k << endl;
    MPI_Send(&b, 1, MPI_INT, proc_num+k, 0, MPI_COMM_WORLD);
  }
  else if (is_cpu_on_bottom_edge(proc_num)) {
    cerr << "CPU" << proc_num << " sending " <<a<< " to "<< proc_num+1 << endl;
    MPI_Send(&a, 1, MPI_INT, proc_num+1, 0, MPI_COMM_WORLD);
  }
  else {
    cerr << "CPU" << proc_num << " sending " <<a<< " to "<< proc_num+1 << endl;
    MPI_Send(&a, 1, MPI_INT, proc_num+1, 0, MPI_COMM_WORLD);
    cerr << "CPU" << proc_num << " sending " <<b<< " to "<< proc_num+k << endl;
    MPI_Send(&b, 1, MPI_INT, proc_num+k, 0, MPI_COMM_WORLD);
  }
}

/**
 * example code from: 
 * http://www.guyrutenberg.com/2007/09/22/profiling-code-using-clock_gettime/
 */
void diff(struct timespec *start, struct timespec *end, struct timespec *temp) {
  if ((end->tv_nsec - start->tv_nsec) < 0) {
    temp->tv_sec = end->tv_sec - start->tv_sec - 1;
    temp->tv_nsec = 1000000000 + end->tv_nsec - start->tv_nsec;
  } else {
    temp->tv_sec = end->tv_sec - start->tv_sec;
    temp->tv_nsec = end->tv_nsec - start->tv_nsec;
  }
}


int main(int argc, char *argv[])
{
  std::string line;
  int first_value;    // first value of a file
  int numprocs;       // how many CPU?
  int myid;           // my rank
  int number;
  int value;
  MPI_Status stat;
  bool measure = false;
  struct timespec time1, time2, d;

  int a;  // value from matrix A
  int b;  // value from matrix B
  int c = 0;  // result of multiplication

  // CPU 0 has values from matrix A in input buffer and values from matrix B
  // will read from MPI_Recv on the fly
  vector<int> input(n);
  int mat_idx = 0;

  if (argc == 2 && (strcmp(argv[1], "-m") == 0)) {
    measure = true;
  }


  // MPI INIT
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);


  // CPU with rank 0 will read the matrices from files mat1 and mat2
  if (myid == 0) {
    readMatrixParams(&n, &m, &k);
    cerr << "CPU" << myid << " m: " << m << endl;
    cerr << "CPU" << myid << " n: " << n << endl;
    cerr << "CPU" << myid << " k: " << k << endl;

    for (int i = 0; i < (m*k); i++) {
      MPI_Send(&m, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      MPI_Send(&n, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      MPI_Send(&k, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }

    sendMatrixAVals(k);     // distribute values from matrix A
    sendMatrixBVals(k);     // distribute values from matrix B
  }

  MPI_Recv(&m, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);
  MPI_Recv(&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);
  MPI_Recv(&k, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);

  /* fill in input buffer with values from CPU 0 */
  if (is_cpu_on_left_edge(myid) || is_cpu_on_top_edge(myid)) {
    for (int i = 0; i < n; i++) {
      MPI_Recv(&value, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);
      input.push_back(value);
    }
  }

  if (is_cpu_first(myid) && measure) {
    clock_gettime(CLOCK_REALTIME, &time1);
  }

  // MESH MULTIPLICATION ALGORITHM
  // Every CPU will execute n-multiplications and some will n-times send some
  // value to its neighbour.
  for (int i = 0; i < n; i++) {

    a = recv_val_a(myid, mat_idx, input);
    b = recv_val_b(myid, mat_idx, input);

    c += a * b;

    send_vals_ab(myid, a, b);
    mat_idx++;
  }

  if (is_cpu_last(myid) && measure) {
    clock_gettime(CLOCK_REALTIME, &time2);
  }

  if (!is_cpu_last(myid)) {
    // Every CPU, except the last one, will send the c value to the last CPU
    // which will print the results.
    MPI_Send(&c, 1, MPI_INT, m*k-1, 0, MPI_COMM_WORLD);
  } 
  else {
    cout << m << ":" << k << endl;

    int i = 0;
    while (i < (m*k-1)) {

      for (int j = 0; j < k; j++) {
        MPI_Recv(&value, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &stat);
        cout << value << " ";

        if (++i >= (m*k-1))
          break;
      }
      
      (i < (m*k-1)) ? cout << endl : cout;
    }

    cout << c << endl;
  }

  if (is_cpu_first(myid) && measure) {
    /* wait until the last process end and ask for start time */
    MPI_Recv(&value, 1, MPI_INT, (m*k-1), 0, MPI_COMM_WORLD, &stat);
    MPI_Send(&(time1.tv_sec), 1, MPI_LONG_INT, (m*k-1), 0, MPI_COMM_WORLD);
    MPI_Send(&(time1.tv_nsec), 1, MPI_LONG_INT, (m*k-1), 0, MPI_COMM_WORLD);
  }

  if (is_cpu_last(myid) && measure) {
      MPI_Send(&value, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
      MPI_Recv(&(time1.tv_sec), 1, MPI_LONG_INT, 0, 0, MPI_COMM_WORLD, &stat);
      MPI_Recv(&(time1.tv_nsec), 1, MPI_LONG_INT, 0, 0, MPI_COMM_WORLD, &stat);
      diff(&time1, &time2, &d);
      printf("time: %ld.%ld\n", d.tv_sec, d.tv_nsec);
  }
 

  MPI_Finalize(); 
}
