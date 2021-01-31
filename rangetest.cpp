#include <iostream>
#include "range.hpp"

using namespace std;

int main(int ac, char** av){
#ifdef TEST_FOR
	if(ac > 1){
		const int e = atoi(av[1]);
		for(int i = 0; i < e; i ++){ cout << i << " "; } cout << endl;
	}
#endif
#ifdef TEST_RANGE
	if(ac > 1){
		const int e = atoi(av[1]);
		for(int i: range(e)){ cout << i << " "; } cout << endl;
	}
#endif
#ifndef TEST_FOR
#ifndef TEST_RANGE
	for(int i = 0; i < 10; i ++){ cout << i << " "; } cout << endl;
	for(auto i: range(10)){ cout << i << " "; } cout << endl;
	for(auto i: range(3, 10)){ cout << i << " "; } cout << endl;
	for(auto i: range(7, 10, 2)){ cout << i << " "; } cout << endl;
	for(auto i: range(6, 10, 2)){ cout << i << " "; } cout << endl;
	for(auto i: range(7, 11, 2)){ cout << i << " "; } cout << endl;
	for(auto i: range(6, 11, 2)){ cout << i << " "; } cout << endl;
	for(auto i: range(10 - 1, -1, -1)){ cout << i << " "; } cout << endl;
	for(auto i: range(10, 7, -2)){ cout << i << " "; } cout << endl;
	for(auto i: range(10, 6, -2)){ cout << i << " "; } cout << endl;
	for(auto i: range(11, 7, -2)){ cout << i << " "; } cout << endl;
	for(auto i: range(11, 6, -2)){ cout << i << " "; } cout << endl;
#endif
#endif
	return 0;
}
