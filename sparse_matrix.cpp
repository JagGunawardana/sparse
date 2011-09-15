#include <boost/numeric/ublas/io.hpp>
#include <boost/algorithm/string.hpp>

#include <fstream>

#include "exception.h"
#include "sparse_matrix.h"

SMatrix::SMatrix(string file_name) {
	smatrix = NULL;
	ReadFromMMFile(file_name);
}

SMatrix::~SMatrix() {
	if (smatrix != NULL)
		delete(smatrix);
}

void SMatrix::ReadFromMMFile(string file_name) {
	ifstream in_file(file_name.c_str());
	string line;
	bool in_data = false;
	std::vector<string> items;
	cout.precision(10);
	int cur_entries =0;
	while(getline(in_file, line)) {
		if (line.substr(0,1) == "%") // ignore comment lines
			continue;
		split(items, line,	is_any_of(" \t"), token_compress_on);
		assert(items.size() == 3);
		if (!in_data) { //line must be dimensions
			N = atoi(items[0].c_str());
			no_entries = atoi(items[2].c_str());
			assert(N == atoi(items[1].c_str()));
			smatrix = new msparse(N, N, no_entries);
			in_data = true;
		}
		else { // parse the data
			int i = atoi(items[0].c_str());
			int j = atoi(items[1].c_str());
			double val = strtod(items[2].c_str(), NULL);
			(*smatrix)(i-1, j-1) = val; // this matrix is 0 based
			cur_entries++;
		}
	}
	assert(cur_entries == no_entries);
}

void SMatrix::GetRow(int row, map<int, double>& mtx) {
}


void SMatrix::Dump(void) {
    typedef msparse::iterator1 I1;
    typedef msparse::iterator2 I2;
    for(I1 i1 = smatrix->begin1() ; i1 != smatrix->end1(); ++i1)
        for(I2 i2 = i1.begin() ; i2 != i1.end(); ++i2)
            cout << "(" << i2.index1() << "," << i2.index2() << ")=" <<
                *i2 << endl;
}
