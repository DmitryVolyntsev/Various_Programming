#include <iostream>
#include <vector>
#include <random>

using namespace std;

class Node {
public:

    int key;
    vector<Node*> children;

    explicit Node (int item) {
        key = item;
    }

    void add_child (int item) {
        children.push_back(new Node(item));
    }
};

class Tree {
public:
    Node* root = nullptr;

    Tree () {
        root = nullptr;
    }

    explicit Tree (Node* r) {
        root = r;
    }

    void insert (int item) {

        if (root == nullptr) {
            root = new Node(item);
            return;
        }

        Node* current = root;
        mt19937 rng;
        rng.seed(random_device()());
        uniform_int_distribution<mt19937::result_type > dist6(0, 100);

        while (!current->children.empty() && (dist6(rng) % 4)) {
            current = current->children[dist6(rng) % current->children.size()];
        }

        current->add_child(item);
    }

    int sum () {
        int sum = root->key;
        if (!root->children.empty()) {
            for (Node* child : root->children) {
                auto new_tree = new Tree(child);
                sum += new_tree->sum();
                delete new_tree;
            }
        }

        return sum;
    }

    int max () {
        int max = root->key;
        if (!root->children.empty()) {
            for (auto child : root->children) {
                auto new_tree = new Tree(child);
                int new_tree_max = new_tree->max();
                if (max < new_tree_max)
                    max = new_tree_max;
            }
        }

        return max;
    }
};

