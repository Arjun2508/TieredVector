#include <bits/stdc++.h>
#include "tiered_vector.hpp"

using namespace std;
using namespace cppx;
#define endl '\n';

int main() {
    tiered_vector<int> tv;

    cout << "-----------START OF TEST-----------" << endl;
    cout << endl;

    //Push Back:
    for (int i = 0; i < 5000; ++i) {
        tv.push_back(3 * i);
    }

    cout << "Size: " << tv.size() << endl;
    cout << "Capacity: " << tv.capacity() << endl;

    cout << endl;

    //Random Access:
    cout << "1001th element: " << tv[1000] << endl;
    cout << "4001th element: " << tv[4000] << endl;

    cout << endl;

    //Copy Constructor Check:
    tiered_vector<int> copy_tv = tv;
    cout << "Copy size: " << copy_tv.size() << endl;
    cout << "Copy element check: " << copy_tv[1234] << endl;

    cout << endl;

    //Move Constructor Check:
    tiered_vector<int> move_tv = move(tv);
    cout << "Move size: " << move_tv.size() << endl;
    cout << "Original after move size(should be 0): " << tv.size() << endl;

    cout << endl;

    //Resize Smaller
    move_tv.resize(2000);
    cout << "After resize(2000): " << move_tv.size() << endl;

    cout << endl;

    //Resize Larger
    move_tv.resize(8000);
    cout << "After resize(8000): " << move_tv.size() << endl;

    cout << endl;

    //Assignment Operator test
    tiered_vector<int> assigned;
    assigned = copy_tv;
    cout << "Assigned size: " << assigned.size() << endl;

    cout << endl;

    //Pop Back hysterisis
    cout << "Before pop_back x1000: " << assigned.size() << endl;
    for (int i = 0; i < 1000; ++i) {
        assigned.pop_back();
    }
    cout << "After pop_back x1000: " << assigned.size() << endl;


    cout << endl;


    cout << "-----------END OF TEST-----------" << endl;

    return 0;
}
