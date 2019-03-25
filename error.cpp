#include <iostream>
#include "error.h"

using namespace std;

void error(const string &msg) {
	cerr << msg << endl;
	exit(-1);
}
