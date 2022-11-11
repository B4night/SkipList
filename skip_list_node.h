#ifndef _SKIP_LIST_NODE_H_
#define _SKIP_LIST_NODE_H_ 

template <typename K, typename V>
class skip_list;

template <typename K, typename V>    // key and value
class skip_list_node {
    friend class skip_list<K, V>;
private:
    K key;
    V value;
    int level;
    skip_list_node<K, V>** forward;
public:
    skip_list_node() {}
    skip_list_node(K k, V v, int level);
    ~skip_list_node();
    K get_key() const;
    V get_value() const;
};


template<typename K, typename V>    // key and value
skip_list_node<K, V>::skip_list_node(K k, V v, int level) : key(k), value(v), level(level) {
    forward = new skip_list_node<K, V>* [level + 1];    //0 ~ level
    for (int i = 0; i < level + 1; i++)
        forward[i] = nullptr;
}

template<typename K, typename V>    // key and value
skip_list_node<K, V>::~skip_list_node() {
    delete[] forward;
}

template<typename K, typename V>    // key and value
K skip_list_node<K, V>::get_key() const {
    return key;
}

template<typename K, typename V>    // key and value
V skip_list_node<K, V>::get_value() const {
    return value;
}

#endif