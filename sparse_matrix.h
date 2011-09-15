#ifndef matrix_csr_h
#define matrix_csr_h

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_sparse.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>


using namespace std;
using namespace boost;
using namespace boost::numeric::ublas;

typedef mapped_matrix<double, column_major> msparse;

/**
 * reads a file in matrix market format
 * matrix must be symmetric.
 * Uses boost to store.
 */
class SMatrix {
public:
	SMatrix(string file_name);
	virtual ~SMatrix();
	void GetRow(int row, map<int, double>& mtx);
	void Dump(void);
protected:
	void ReadFromMMFile(string file_name);

private:
	int N;
	int no_entries;
	msparse* smatrix;
};

#endif
