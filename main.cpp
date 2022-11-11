#include "skip_list.h"
#include <string>

int main() {
    skip_list<std::string, std::string> sl("skip_list_dump");
    // for (int i = 0; i < 10; i++)
    //     sl.insert_node(i, i);
    sl.show_list();
}