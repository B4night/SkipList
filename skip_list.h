#ifndef _SKIP_LIST_H_
#define _SKIP_LIST_H_

#include "skip_list_node.h"
#include <cstdlib>
#include <iostream>
#include <mutex>

template <typename K, typename V>
class skip_list {
private:
    int max_level;
    int current_level;
    skip_list_node<K, V>* header;
    int cnt;
public:
    skip_list(int l = 8);
    ~skip_list();
    int get_random_level();
    skip_list_node<K, V>* create_node(K, V, int);
    int insert_node(K, V);
    bool search_node(K);
    void delete_node(K);
    int size();
    void show_list();
private:
    std::mutex lk;
};

template <typename K, typename V>
void skip_list<K, V>::show_list() {
    for (int i = max_level; i >= 0; i--) {
        std::cout << "level " << i << "  :";
        skip_list_node<K, V>* current = header->forward[i];
        while (current != nullptr) {
            std::cout << "  " << current->get_key() << ":" << current->get_value();
            current = current->forward[i];
        }
        std::cout << std::endl;
    }
}

template <typename K, typename V>
int skip_list<K, V>::size() {
    return this->cnt;
}

template <typename K, typename V>
skip_list<K, V>::skip_list(int max_level) : max_level(max_level), current_level(0), cnt(0) {
    K k;
    V v;
    this->header = new skip_list_node<K, V>(k, v, max_level);
}

template <typename K, typename V>
skip_list<K, V>::~skip_list() {
    delete header;
}

template <typename K, typename V>
int skip_list<K, V>::get_random_level() {
    int k = 0;
    while (rand() % 2) k++;
    return k < max_level ? k : max_level;
}

template <typename K, typename V>
skip_list_node<K, V>* skip_list<K, V>::create_node(K k, V v, int l) {
    return new skip_list_node<K, V>(k, v, l);
}

template <typename K, typename V>
int skip_list<K, V>::insert_node(K k, V v) {
    std::lock_guard<std::mutex> lg(lk);
    std::cout << "Start insert node, key = " << k << ", value = " << v << std::endl;
    skip_list_node<K, V>* current = this->header;
    skip_list_node<K, V>* update[max_level + 1];    // 记录每一层经过的值最大(最右边)的结点
    for (int i = 0; i < max_level + 1; i++)
        update[i] = nullptr;
    
    for (int i = current_level; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->get_key() < k)
            current = current->forward[i];
        update[i] = current;
    }
    current = current->forward[0];

    if (current != NULL && current->get_key() == k) {
        std::cout << "Already have key:" << k << std::endl;
        return -1;
    }
    if (current == NULL || current->get_key() != k) {
        int random_level = get_random_level();

        if (random_level > current_level) {
            for (int i = current_level + 1; i < random_level + 1; i++)
                update[i] = header;
            current_level = random_level;
        }
        skip_list_node<K, V>* tmp = create_node(k, v, random_level);
        
        for (int i = 0; i < random_level + 1; i++) {
            tmp->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = tmp;
        }
        std::cout << "Successfully insert node, key=" << k << ",value=" << v << std::endl;
        cnt++;
    }
    return 0;
}

template <typename K, typename V>
bool skip_list<K, V>::search_node(K k) {
    std::cout << "Start find node, key = " << k << std::endl;
    skip_list_node<K, V>* current = this->header;
    for (int i = current_level; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->get_key() < k)
            current = current->forward[i];      // 在同一level中向后移动
    }
    // 由于current->forward[i]的key值为小于k的node中最大的,所以要往右进一格
    current = current->forward[0];
    if (current && current->get_key() == k) {
        std::cout << "Find Key:  " << k << ", value = " << current->get_value() << std::endl;
        // std::cout.flush();
        return true;
    }
    std::cout << "Not find key" << std::endl;
    // std::cout.flush();
}

template <typename K, typename V>
void skip_list<K, V>::delete_node(K k) {

}

#endif