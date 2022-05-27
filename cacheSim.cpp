/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

class Entry {       // The class
public:             // Access specifier
    unsigned long int tag;
    bool valid = false;
    bool dirty = false;
    unsigned LRUrank;
};

int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

    unsigned L1BlockNum = L1Size - BSize;
    unsigned L2BlockNum = L2Size - BSize;

    unsigned L1SetsNum = L1BlockNum - L1Assoc;
    unsigned L2SetsNum = L2BlockNum - L2Assoc;

    unsigned int L1Misses = 0;
    unsigned int L2Misses = 0;
    unsigned int L1AcceseeNum = 0;
    unsigned int L2AcceseeNum = 0;

    // Build cache

    // Build L1
    Entry L1[(int)pow(2,L1Assoc)][(int)pow(2,L1BlockNum-L1Assoc)];
//    unsigned int L1Counts[(int)pow(2,L1Assoc)];
    for (int i = 0; i < (int)pow(2,L1Assoc); ++i) {
        for (int j = 0; j < (int)pow(2,L1BlockNum-L1Assoc); ++j) {
            L1[i][j].LRUrank = i;
        }
    }

    // Build L2
    Entry L2[(int)pow(2,L2Assoc)][(int)pow(2,L2BlockNum-L2Assoc)];
//    unsigned int L2Counts[(int)pow(2,L2Assoc)];
    for (int i = 0; i < (int)pow(2,L2Assoc); ++i) {
        for (int j = 0; j < (int)pow(2,L2BlockNum-L2Assoc); ++j) {
            L2[i][j].LRUrank = i;
        }
    }

	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
//		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
//		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);
//        num = num >> 2; // Ignore2 LSB (2 right bits).

        unsigned int L1tag = num >> (BSize + L1SetsNum);
        unsigned int L1set = (num << (32 - (BSize + L1SetsNum)));
        L1set = L1set >>  (31 - L1SetsNum);
        L1set = L1set >>  1;

        bool found = false;

        if(operation == 'w'){
            if(WrAlloc){
                L1AcceseeNum += 1;
                for (int i = 0; i < (int)pow(2,L1Assoc); ++i) {
                    if(L1[i][L1set].valid && (L1[i][L1set].tag == L1tag)){
                        L1[i][L1set].dirty = true;
                        found = true;
                        unsigned prevL1Count = L1[i][L1set].LRUrank;
                        L1[i][L1set].LRUrank =  pow(2, L1Assoc) - 1;
                        for (int j = 0; j < (int)pow(2,L1Assoc); ++j) {
                            if((j != i) && (L1[j][L1set].LRUrank >= prevL1Count)){
                                L1[j][L1set].LRUrank--;
                            }
                        }
                    }
                }

                if(!found){
                    // No hit on L1.
                    L1Misses+=1;
                    // Accessee L2
                    L2AcceseeNum += 1;
                    unsigned int L2tag = num >> (BSize + L2SetsNum);
                    unsigned int L2set = (num << 32 - (BSize + L2SetsNum));
                    L2set = L2set >> 31 - L2SetsNum;
                    L2set = L2set >> 1;
                    for (int i = 0; i < (int)pow(2,L2Assoc); ++i) {
                        if(L2[i][L2set].valid && (L2[i][L2set].tag == L2tag)){
                            found = true;
                            L2[i][L2set].dirty = true;
                            unsigned prevL2Rank = L2[i][L2set].LRUrank;
                            L2[i][L2set].LRUrank =  pow(2, L2Assoc) - 1;
                            for (int j = 0; j < (int)pow(2,L2Assoc); ++j) {
                                if((j != i) && (L2[j][L2set].LRUrank >= prevL2Rank)){
                                    L2[j][L2set].LRUrank--;
                                }
                            }
                            // Update L1
                            for (int j = 0; j < (int)pow(2,L1Assoc); ++j) {
                                if(L1[j][L1set].LRUrank == 0){
                                    if(L1[j][L1set].dirty){
                                        // Recalc L2 net tag.
                                        unsigned newNum =  ((L1[j][L1set].tag << L1SetsNum) + L1set);
                                        newNum = newNum << BSize;
                                        unsigned NewL2tag = newNum >> (BSize + L2SetsNum);
                                        unsigned NewL2set = (newNum << 32 - (BSize + L2SetsNum));
                                        NewL2set = NewL2set >> 31 - L2SetsNum;
                                        NewL2set = NewL2set >> 1;
                                        for (int k = 0; k < (int)pow(2,L2Assoc); ++k) {
                                            if(L2[k][NewL2set].tag == NewL2tag){
                                                L2[k][NewL2set].dirty = true;
                                                L2[k][NewL2set].valid = true;
                                                unsigned prevL2Count = L2[k][L2set].LRUrank;
                                                L2[k][L2set].LRUrank =  pow(2, L2Assoc) - 1;
                                                for (int i = 0; i <  (int)pow(2,L2Assoc); ++i) {
                                                    if((L2[i][L2set].LRUrank >= prevL2Count) && (i != k)){
                                                        L2[i][L2set].LRUrank--;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    L1[j][L1set].dirty = true;
                                    L1[j][L1set].tag = L1tag;
                                    L1[j][L1set].valid = true;
                                    L1[j][L1set].LRUrank = pow(2, L1Assoc) - 1;
                                }else{
                                    L1[j][L1set].LRUrank--;
                                }
                            }
                        }
                    }
                    if(!found){
                        // L2 Miss
                        L2Misses++;

                        // Update L2
                        for (int j = 0; j < (int)pow(2,L2Assoc); ++j) {
                            if(L2[j][L2set].LRUrank == 0){

                                // Recalc L1 set tag.
                                unsigned newNum =  ((L2[j][L2set].tag << L2SetsNum) + L2set);
                                newNum = newNum << BSize;
                                unsigned NewL1tag = newNum >> (BSize + L1SetsNum);
                                unsigned NewL1set = (newNum << 32 - (BSize + L1SetsNum));
                                NewL1set = NewL1set >> 31 - L1SetsNum;
                                NewL1set = NewL1set >> 1;

                                for (int k = 0; k < (int)pow(2,L1Assoc); ++k) {
                                    if ((L1[k][NewL1set].tag == NewL1tag) && L1[k][NewL1set].valid) {
                                        L1[k][NewL1set].valid = false;
                                        L1[k][NewL1set].dirty = false;
                                        unsigned prevL1Count = L1[k][NewL1set].LRUrank;
                                        L1[k][NewL1set].LRUrank = pow(2, L1Assoc) - 1;
                                        for (int i = 0; i < (int) pow(2, L1Assoc); ++i) {
                                            if ((L1[i][NewL1set].LRUrank >= prevL1Count) && (i != k)) {
                                                L1[i][NewL1set].LRUrank--;
                                            }
                                        }
                                    }
                                }

                                L2[j][L2set].dirty = false;
                                L2[j][L2set].tag = L2tag;
                                L2[j][L2set].valid = true;
                                L2[j][L2set].LRUrank = pow(2, L2Assoc) - 1;
                            }else{
                                L2[j][L2set].LRUrank--;
                            }
                        }

                        // Update L1
                        for (int j = 0; j < (int)pow(2,L1Assoc); ++j) {
                            if(L1[j][L1set].LRUrank == 0){
                                if(L1[j][L1set].dirty){
                                    // Recalc L2 net tag.
                                    unsigned newNum =  ((L1[j][L1set].tag << L1SetsNum) + L1set);
                                    newNum = newNum << BSize;
                                    unsigned NewL2tag = newNum >> (BSize + L2SetsNum);
                                    unsigned NewL2set = (newNum << 32 - (BSize + L2SetsNum));
                                    NewL2set = NewL2set >> 31 - L2SetsNum;
                                    NewL2set = NewL2set >> 1;

                                    for (int k = 0; k < (int)pow(2,L2Assoc); ++k) {
                                        if(L2[k][NewL2set].tag == NewL2tag){
                                            L2[k][NewL2set].dirty = true;
                                            unsigned prevL2Count = L2[k][NewL2set].LRUrank;
                                            L2[k][NewL2set].LRUrank =  pow(2, L2Assoc) - 1;
                                            for (int i = 0; i <  (int)pow(2,L2Assoc); ++i) {
                                                if((L2[i][NewL2set].LRUrank >= prevL2Count) && (i != k)){
                                                    L2[i][NewL2set].LRUrank--;
                                                }
                                            }
                                        }
                                    }
                                }
                                L1[j][L1set].dirty = true;
                                L1[j][L1set].tag = L1tag;
                                L1[j][L1set].valid = true;
                                L1[j][L1set].LRUrank = pow(2, L1Assoc) - 1;
                            }else{
                                L1[j][L1set].LRUrank--;
                            }
                        }
                    }
                }
            }else{
                // write no allocate
                L1AcceseeNum += 1;
                for (int i = 0; i < (int)pow(2,L1Assoc); ++i) {
                    if(L1[i][L1set].valid && (L1[i][L1set].tag == L1tag)){
                        L1[i][L1set].dirty = true;
                        found = true;
                        unsigned prevL1Count = L1[i][L1set].LRUrank;
                        L1[i][L1set].LRUrank =  pow(2, L1Assoc) - 1;
                        for (int j = 0; j < (int)pow(2,L1Assoc); ++j) {
                            if((j != i) && (L1[j][L1set].LRUrank >= prevL1Count)){
                                L1[j][L1set].LRUrank--;
                            }
                        }
                    }
                }

                if(!found){
                    // No hit on L1.
                    L1Misses+=1;
                    // Accessee L2
                    L2AcceseeNum += 1;
                    unsigned int L2tag = num >> (BSize + L2SetsNum);
                    unsigned int L2set = (num << 32 - (BSize + L2SetsNum));
                    L2set = L2set >> 31 - L2SetsNum;
                    L2set = L2set >> 1;
                    for (int i = 0; i < (int)pow(2,L2Assoc); ++i) {
                        if(L2[i][L2set].valid && (L2[i][L2set].tag == L2tag)){
                            found = true;
                            L2[i][L2set].dirty = true;
                            unsigned prevL2Count = L2[i][L2set].LRUrank;
                            L2[i][L2set].LRUrank =  pow(2, L2Assoc) - 1;
                            for (int j = 0; j <  (int)pow(2, L2Assoc); ++j) {
                                if((j != i) && (L2[j][L2set].LRUrank >= prevL2Count)){
                                    L2[j][L2set].LRUrank--;
                                }
                            }
                        }
                    }
                    if(!found){
                        // L2 Miss
                        L2Misses++;

//                        // Update L2
//                        for (int j = 0; j < (int)pow(2,L2Assoc); ++j) {
//                            if(L2[j][L2set].LRUrank == 0){
//
//                                // Go to L1 and remove the previous L2 val
//                                if(L2[j][L2set].valid){
//                                    // Recalc L1 set tag.
//                                    unsigned newNum =  ((L2[j][L2set].tag << L2SetsNum) + L2set);
//                                    unsigned NewL1tag = newNum >> (BSize + L1SetsNum);
//                                    unsigned NewL1set = (newNum << 32 - (BSize + L1SetsNum)) >>  32 - L1SetsNum;
//                                    NewL1set = NewL1set >> 1;
//
//                                    for (int k = 0; k < (int)pow(2,L1Assoc); ++k) {
//                                        if(L1[k][NewL1set].tag == NewL1tag &&  L1[k][NewL1set].valid){
//                                            L1[k][NewL1set].valid = false;
//                                            L1[k][NewL1set].dirty = false;
//                                            unsigned prevL1Count = L1[k][NewL1set].LRUrank;
//                                            L1[k][NewL1set].LRUrank =  pow(2, L2Assoc) - 1;
//                                            for (int i = 0; i <  (int)pow(2,L1Assoc); ++i) {
//                                                if(L1[i][L1set].LRUrank >= k && i != k){
//                                                    L1[i][L1set].LRUrank--;
//                                                }
//                                            }
//                                        }
//                                    }
//
//                                }
//
//                                L2[j][L2set].dirty = false;
//                                L2[j][L2set].tag = L2tag;
//                                L2[j][L2set].valid = true;
//                                L2[j][L2set].LRUrank = pow(2, L2Assoc) - 1;
//                            }else{
//                                L2[j][L2set].LRUrank--;
//                            }
//                        }
//
//                        // Update L1
//                        for (int j = 0; j < (int)pow(2,L1Assoc); ++j) {
//                            if(L1[j][L1set].LRUrank == 0){
//                                if(L1[j][L1set].dirty){
//                                    // Recalc L2 net tag.
//                                    unsigned newNum =  ((L1[j][L1set].tag << L1SetsNum) + L1set);
//                                    unsigned NewL2tag = newNum >> (BSize + L2SetsNum);
//                                    unsigned NewL2set = (newNum << 32 - (BSize + L2SetsNum));
//                                    NewL2set = NewL2set >> 31 - L2SetsNum;
//                                    NewL2set = NewL2set >> 1;
//
//                                    for (int k = 0; k < (int)pow(2,L2Assoc); ++k) {
//                                        if(L2[k][NewL2set].tag == NewL2tag){
//                                            L2[k][NewL2set].dirty = true;
//                                            unsigned prevL2Count = L2[k][NewL2set].LRUrank;
//                                            L2[k][NewL2set].LRUrank =  pow(2, L2Assoc) - 1;
//                                            for (int i = 0; i <  (int)pow(2,L2Assoc); ++i) {
//                                                if(L2[i][NewL2set].LRUrank >= k && i != k){
//                                                    L2[i][NewL2set].LRUrank--;
//                                                }
//                                            }
//                                        }
//                                    }
//                                }
//                                L1[j][L1set].dirty = false;
//                                L1[j][L1set].tag = L1tag;
//                                L1[j][L1set].valid = true;
//                                L1[j][L1set].LRUrank = pow(2, L1Assoc) - 1;
//                            }else{
//                                L1[j][L1set].LRUrank--;
//                            }
//
//                        }
                    }
                }
            }
        }else{
            // Read command
            L1AcceseeNum += 1;
            for (int i = 0; i < (int)pow(2,L1Assoc); ++i) {
                if(L1[i][L1set].valid && (L1[i][L1set].tag == L1tag)){
                    found = true;
                    unsigned prevL1Count = L1[i][L1set].LRUrank;
                    L1[i][L1set].LRUrank =  pow(2, L1Assoc) - 1;
                    for (int j = 0; j < (int)pow(2,L1Assoc); ++j) {
                        if((j != i) && (L1[j][L1set].LRUrank >= prevL1Count)){
                            L1[j][L1set].LRUrank--;
                        }
                    }
                }
            }

            if(!found){
                // No hit on L1.
                L1Misses+=1;
                // Accessee L2
                L2AcceseeNum += 1;
                unsigned int L2tag = num >> (BSize + L2SetsNum);
                unsigned int L2set = (num << (32 - (BSize + L2SetsNum)));
                L2set = L2set >> (31 - L2SetsNum);
                L2set = L2set >> 1;
                for (int i = 0; i < (int)pow(2,L2Assoc); ++i) {
                    if(L2[i][L2set].valid && (L2[i][L2set].tag == L2tag)){
                        found = true;
                        bool previousL2DirtyBit = L2[i][L2set].dirty;
//                        L2[i][L2set].dirty = false;
                        unsigned prevL2Count = L2[i][L2set].LRUrank;
                        L2[i][L2set].LRUrank =  pow(2, L2Assoc) - 1;
                        for (int j = 0; j < (int)pow(2,L2Assoc); ++j) {
                            if((j != i) && (L2[j][L2set].LRUrank >= prevL2Count)){
                                L2[j][L2set].LRUrank--;
                            }
                        }
                        // Update L1
                        for (int j = 0; j < (int)pow(2,L1Assoc); ++j) {
                            if(L1[j][L1set].LRUrank == 0){
                                if(L1[j][L1set].dirty){
                                    // Recalc L2 net tag.
                                    unsigned newNum =  ((L1[j][L1set].tag << L1SetsNum) + L1set);
                                    newNum = newNum << BSize;
                                    unsigned NewL2tag = newNum >> (BSize + L2SetsNum);
                                    unsigned NewL2set = (newNum << 32 - (BSize + L2SetsNum));
                                    NewL2set = NewL2set >> 31 - L2SetsNum;
                                    NewL2set = NewL2set >> 1;

                                    for (int k = 0; k < (int)pow(2,L2Assoc); ++k) {
                                        if(L2[k][NewL2set].tag == NewL2tag){
                                            L2[k][NewL2set].dirty = true;
                                            unsigned prevL2Count = L2[k][NewL2set].LRUrank;
                                            L2[k][NewL2set].LRUrank =  pow(2, L2Assoc) - 1;
                                            for (int i = 0; i <  (int)pow(2,L2Assoc); ++i) {
                                                if((L2[i][NewL2set].LRUrank >= prevL2Count) && (i != k)){
                                                    L2[i][NewL2set].LRUrank--;
                                                }
                                            }
                                        }
                                    }
                                }
                                L1[j][L1set].dirty = previousL2DirtyBit;
                                L1[j][L1set].tag = L1tag;
                                L1[j][L1set].valid = true;
                                L1[j][L1set].LRUrank = pow(2, L1Assoc) - 1;
                            }else{
                                L1[j][L1set].LRUrank--;
                            }
                        }
                    }
                }
                if(!found){
                    // L2 Miss
                    L2Misses++;

                    // Update L2
                    for (int j = 0; j < (int)pow(2,L2Assoc); ++j) {
                        if(L2[j][L2set].LRUrank == 0){

                            // Go to L1 and remove the previous L2 val
                            if(L2[j][L2set].valid){
                                // Recalc L1 set tag.
                                unsigned newNum =  ((L2[j][L2set].tag << L2SetsNum) + L2set);
                                newNum = newNum << BSize;
                                unsigned NewL1tag = newNum >> (BSize + L1SetsNum);
                                unsigned NewL1set = (newNum << 32 - (BSize + L1SetsNum));
                                NewL1set = NewL1set >> 31 - L1SetsNum;
                                NewL1set = NewL1set >> 1;

                                for (int k = 0; k < (int)pow(2,L1Assoc); ++k) {
                                    if((L1[k][NewL1set].tag == NewL1tag) &&  L1[k][NewL1set].valid){
                                        L1[k][NewL1set].valid = false;
                                        L1[k][NewL1set].dirty = false;
                                        unsigned prevL1Count = L1[k][NewL1set].LRUrank;
                                        L1[k][NewL1set].LRUrank =  pow(2, L1Assoc) - 1;
                                        for (int i = 0; i <  (int)pow(2,L1Assoc); ++i) {
                                            if((L1[i][L1set].LRUrank >= prevL1Count) && (i != k)){
                                                L1[i][L1set].LRUrank--;
                                            }
                                        }
                                    }
                                }

                            }

                            L2[j][L2set].dirty = false;
                            L2[j][L2set].tag = L2tag;
                            L2[j][L2set].valid = true;
                            L2[j][L2set].LRUrank = pow(2, L2Assoc) - 1;
                        }else{
                            L2[j][L2set].LRUrank--;
                        }
                    }

                    // Update L1
                    for (int j = 0; j < (int)pow(2,L1Assoc); ++j) {
                        if(L1[j][L1set].LRUrank == 0){
                            if(L1[j][L1set].valid && L1[j][L1set].dirty){
                                // Recalc L2 net tag.
                                unsigned newNum =  ((L1[j][L1set].tag << L1SetsNum) + L1set);
                                newNum = newNum << BSize;
                                unsigned NewL2tag = newNum >> (BSize + L2SetsNum);
                                unsigned NewL2set = (newNum << 32 - (BSize + L2SetsNum));
                                NewL2set = NewL2set >> 31 - L2SetsNum;
                                NewL2set = NewL2set >> 1;

                                for (int k = 0; k < (int)pow(2,L2Assoc); ++k) {
                                    if(L2[k][NewL2set].tag == NewL2tag){
                                        L2[k][NewL2set].dirty = true;
                                        unsigned prevL2Count = L2[k][NewL2set].LRUrank;
                                        L2[k][NewL2set].LRUrank =  pow(2, L2Assoc) - 1;
                                        for (int i = 0; i <  (int)pow(2,L2Assoc); ++i) {
                                            if((L2[i][NewL2set].LRUrank >= prevL2Count) && (i != k)){
                                                L2[i][NewL2set].LRUrank--;
                                            }
                                        }
                                    }
                                }
                            }
                            L1[j][L1set].dirty = false;
                            L1[j][L1set].tag = L1tag;
                            L1[j][L1set].valid = true;
                            L1[j][L1set].LRUrank = pow(2, L1Assoc) - 1;
                        }else{
                            L1[j][L1set].LRUrank--;
                        }
                    }
                }
            }
        }
	}

	double L1MissRate =  (double)L1Misses/(double)L1AcceseeNum;
	double L2MissRate =  (double)L2Misses/ (double)L2AcceseeNum;
	double avgAccTime =  (double)(L1AcceseeNum*L1Cyc + L2AcceseeNum*L2Cyc + L2Misses*MemCyc)/(double)L1AcceseeNum;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
