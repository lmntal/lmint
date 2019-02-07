#include <bits/stdc++.h>
using namespace std;
#define rep(i,n) for(int i=0;i<int(n);++i)

#define debug(x) cerr<<(#x)<<": "<<(x)<<endl

int main(void){
    int n;
    cin >> n;
    vector<set<int>> g(n);
    std::random_device seed_gen;
    std::mt19937 mt(seed_gen());
    std::uniform_int_distribution<> rand_int(0, n-1);

    // 大規模
    for (int i = 0; i < 10; i++) {
        rep(j, n/10) {
            int r = rand_int(mt);
            g[i].insert(r);
            g[r].insert(i);
        }
    }
    // 小規模
    for (int i = 10; i < n; i++) {
        for (int j = 0; j < 10; j++) {
            int r = rand_int(mt);
            g[i].insert(r);
        }
    }

    int cnt = 0;
    for (int i = 0; i < n; i++) {
        for (int to : g[i]) {
            printf("follow(%d,%d).\n", i, to);
        }
    }
    return 0;
}
