#include <vector>
#include <iostream>

using namespace std;

void foo(vector<vector<int> >& matrix)
{
  cout << "matrix[0][0]: "<< matrix[0][0] << endl;
}

int main() {

  int m = 5;

  vector<vector<int> > matrix(m);
  vector<vector<int> >* p_matrix;

  p_matrix = &matrix;

  matrix[0].push_back(10);
  matrix[0].push_back(11);
  matrix[0].push_back(12);

  cout << "first: " << (*p_matrix)[0].size() << endl;
  cout << "first: " << (*p_matrix)[0][0] << endl;

  cout << "m[0].size: " << matrix[0].size() << endl;
  cout << "m[0][0]: " << matrix[0][0] << endl;
  cout << "m[0][0]: " << matrix[0][1] << endl;

  foo(matrix);
  
  // read vector from the end
  for (vector<int>::reverse_iterator i = matrix[0].rbegin(); i != matrix[0].rend(); ++i ) { 
    cout << "hodnota: " << *i << endl;
  }

  //matrix[0][0] = 32;
  //matrix[7][0] = 32;
}
