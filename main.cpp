#include "skip_list.h"
#include <string>

int main() {
    skip_list<int, int> sl;
    for (int i = 0; i < 10; i++)
        sl.insert_node(i, i);
    sl.show_list();
}