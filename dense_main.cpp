#include <iostream>
#include <boost/filesystem.hpp>

#include "sparse_matrix.h"

int main(int argc, char* argv[]) {
	if (argc != 2) {
		cout << "Usage: "<<argv[0]<<" matrix_file_name" << endl;
		exit(1);
	}
	string file_name = argv[1];
	cout << "Reading matrix file: "<< file_name<<endl;
	filesystem::path p(file_name);
	if (!filesystem::is_regular_file(p)  || filesystem::file_size(p) == 0) {
		cout << "Usage: "<<argv[0]<<" matrix_file_name" << endl;
		exit(1);
	}
	SMatrix matrix(file_name);
	matrix.Dump();
	return(0);
}
