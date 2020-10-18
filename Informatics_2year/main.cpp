#include <iostream>
#include <thread>
#include <functional>
#include <mutex>
#include <random>
#include <ctime>
#include "tree.cpp"

using namespace std;

void thread_func (Tree* tree, int* max_var, mutex* lock) {
    int max = tree->max();
    if (*max_var < max) {
        lock->lock();
        *max_var = max;
        lock->unlock();
    }
}

int main() {

    int tree_max;


    mt19937 rng;
    rng.seed(random_device()());
    uniform_int_distribution<mt19937::result_type > dist6(0, 99999);

    // creating tree
    Tree my_tree;
    for(int i = 1; i < 10000; i++)
        my_tree.insert((int) dist6(rng));

    clock_t start = clock();
    tree_max = my_tree.max();
    clock_t stop = clock();

    double elapsed_time = (double) (stop - start) / CLOCKS_PER_SEC;

    cout << "Tree m: " << tree_max << "; 1 thread time: " << elapsed_time << endl;

    // initializing values for threads
    tree_max = 0;
    int thread_num;
    mutex sum_lock;
    cout << "Enter a number of threads from 1 to " << my_tree.root->children.size() << ": ";
    cin >> thread_num;

    // managing threads
    vector<thread> ths;
    auto clocks = 0;
    for (int i = 0; i < thread_num-1; i++){
        auto th_tree = new Tree(my_tree.root->children[i]);
        start = clock();
        ths.emplace_back(thread_func, th_tree, &tree_max, &sum_lock);
        stop = clock();
        clocks += stop - start;
    }

    start = clock();

    for (auto i = thread_num - 1; i < my_tree.root->children.size(); i++) {
        Tree tr(my_tree.root->children[i]);
        int max = tr.max();
        if (tree_max < max) {
            sum_lock.lock();
            tree_max = max;
            sum_lock.unlock();
        }
    }

    if (tree_max < my_tree.root->key) {
        sum_lock.lock();
        tree_max = my_tree.root->key;
        sum_lock.unlock();
    }

    for (auto& th : ths) {
        th.join();
    }
    stop = clock();
    clocks += stop - start;

    elapsed_time = (double) clocks / CLOCKS_PER_SEC;
    cout << "Tree sum = " << tree_max << "; " << thread_num << " threads time: " << elapsed_time << endl;

    return 0;
}