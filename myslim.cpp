#include <bits/stdc++.h>
using namespace std;
#define print(x) cerr << (#x) << ": " << (x) << endl;
const int INF = 1e9;

class Functor;
class Atom;
class Temp;
class Guard;
class Register;
class Rule;

map<Functor,list<Atom*>> atomlist;
vector<Rule> rulelist;

class Functor {
  public:
    int arity;
    string name;
    Functor(){}
    ~Functor(){
        // cerr << "delete Functor" << endl;
    }
    Functor(int arity_, string name_): arity(arity_),name(name_) {}

    bool operator==(const Functor r) const {
        return arity == r.arity && name == r.name;
    }
    bool operator!=(const Functor r) const {
        return arity != r.arity || name != r.name;
    }
    bool operator<(const Functor r) const {
        return arity != r.arity ? arity < r.arity : name < r.name;
    }
};

const Functor A2 = {2, "a"};
const Functor A0 = {0, "a"};
const Functor B2 = {2, "b"};
const Functor B0 = {0, "b"};
const Functor FL = {0, ""}; 


class Link {
  public:
    Atom *dst_atom;
    int dst_pos;
    Link(){}
    Link(Atom *da, int dp): dst_atom(da), dst_pos(dp) {}
    ~Link(){}
};

class Atom {
  public:
    Functor functor;

    vector<Atom*> link;
    vector<int> link_pos;

    list<Atom*>::iterator itr;

    Atom(){}
    ~Atom(){
        // cerr << "deleted Atom" << endl;
    }
    Atom(Functor functor_){
        functor = functor_;
        int arity = functor.arity;
        link = vector<Atom*>(arity);
        link_pos = vector<int>(arity);
    }
    void add_to_atomlist(){
        atomlist[functor].push_front(this);
        itr = atomlist[functor].begin();
    }

    // isSymbol
    // isInt
};

// bodyのX=Yに対応していない

class Temp{
  public:
    Functor functor;
    
    vector<Functor> link_functor;
    // gen(0) :- -> <100,0>
    // gen(N) :- N>0 | -> <int,0>
    
    vector<int> link_pos;
    // 非負整数 -> 接続位置
    // -1      -> 自由リンク

    vector<int> link_reg;
    // registerID(link_posでatomかfreelinkか振り分け)

    Temp(){}
    Temp(Functor f, vector<Functor> lf, vector<int> lp, vector<int> lr):  
        functor(f), link_functor(lf), link_pos(lp), link_reg(lr){}
};

class Guard{
  public:
    string function;
    vector<string> variable;
};

// Matching 時に使う変数的役割
class Register {
  public:
    vector<Atom*> head;
    vector<Atom*> body;
    vector<Atom*> freelink;
    vector<int> freelink_pos;
};

class Rule {
  public:
    int freelink_num;
    vector<Temp> head;
    vector<Guard> gurad;
    vector<Temp> body;
};

// ----------------------------------------------------------------------

bool set_atom_to_reg(Rule &rule, Register &reg, Atom* atom, int hi){
    Functor functor = atom->functor;
    reg.head[hi] = atom;

    // 他でreg登録済みのアトムならだめ

    /* リンクが合っているか */
    int arity = functor.arity;
    for (int i = 0; i < arity; i++){  
        Atom* dst_atom = atom->link[i];

        // 自由リンク
        if(rule.head[hi].link_functor[i] == FL){
            continue;
        }

        // リンク先のファンクタが違う
        if (dst_atom->functor != rule.head[hi].link_functor[i]){
            reg.head[hi] = NULL;
            return false;
        }

        // 接続場所が合っているか
        int dst_pos = rule.head[hi].link_pos[i];
        if(!(dst_atom->link[dst_pos] == atom &&
             dst_atom->link_pos[dst_pos] == i)){
            reg.head[hi] = NULL;
            return false;
        }
    }



    /* リンクをレジスタに登録 */
    for (int i = 0; i < arity; i++){
        Atom* dst_atom = atom->link[i];
        int link_reg = rule.head[hi].link_reg[i];

        if(rule.head[hi].link_functor[i] == FL){
            // 自由リンク
            reg.freelink[link_reg] = dst_atom;
            reg.freelink_pos[link_reg] = atom->link_pos[i];
        }else if(reg.head[link_reg] == NULL){
            // 自由リンクでない かつ レジスタ未登録
            bool ok = set_atom_to_reg(rule, reg, dst_atom, link_reg);
            if (!ok){
                reg.head[hi] = NULL;
                return false;
            }
        }
    }

    return true;
}

void remove_atom_from_reg(Rule &rule, Register &reg, Atom* atom, int hi){
    Functor functor = atom->functor;
    reg.head[hi] = NULL;

    int arity = functor.arity;
    for (int i = 0; i < arity; i++){
        Atom* dst_atom = atom->link[i];
        int link_reg = rule.head[hi].link_reg[i];

        if(rule.head[hi].link_functor[i] == FL){
            // 自由リンク
            reg.freelink[link_reg] =  NULL;
            reg.freelink_pos[link_reg] = 0;
        }else if(reg.head[link_reg] != NULL){
            // 自由リンクでない かつ レジスタ登録済み
            remove_atom_from_reg(rule, reg, dst_atom, link_reg);
        }
    }
}

bool find_atom(Rule &rule, Register &reg){
    // 未決定アトムの中で最小のアトムリストのものを選ぶ
    int min_atomlist_size = INF, hi = -1;
    for(int i = 0; i < (int)reg.head.size(); i++) {
        if(reg.head[i] != NULL) continue;
        int atomlist_size = atomlist[rule.head[i].functor].size();
        if( min_atomlist_size > atomlist_size) {
            min_atomlist_size = atomlist_size;
            hi = i;
        }
    }
    if(hi == -1){
        // 全て決定している
        return true;
    }else{
        Functor functor = rule.head[hi].functor;
        for(auto &atom : atomlist[functor]){
            bool ok = set_atom_to_reg(rule,reg,atom,hi);
            if (ok){
                if(find_atom(rule,reg)) return true;
                else remove_atom_from_reg(rule,reg,atom,hi);
            }
        }
        return false;
    }
}

void commit(Rule &rule, Register &reg) {
    // print(__func__);
    // cout << reg.head[0] << endl << reg.head[1] << endl;
    for(auto &atom : reg.head){
        if(atom == NULL){
            cerr << "head NULL" << endl;
            return;
        }
    }
/*
    freelinkがheadを指しているとき、
    そのfreelinkを新しいほうにつなぎ変えてからもとのheadを消す必要がある
    (X=Yについてはまだかんがえていない)
*/

    // 1. 新しいアトムを生成する
    for(int i = 0; i < (int)rule.body.size(); i++){
        reg.body[i] = new Atom(rule.body[i].functor);
        reg.body[i]->add_to_atomlist();
    }

    // 2. 自由リンクの先からこちらを向いてるものを新しいアトムに指す
    for (int i = 0; i < (int)rule.body.size(); i++) {
        for (int j = 0; j < (int)rule.body[i].link_functor.size(); j++) {
            // bodyのi番目のアトムのj番目のリンクについて

            if (rule.body[i].link_functor[j] == FL) {
                int fi = rule.body[i].link_reg[j];
                reg.freelink[fi]->link[reg.freelink_pos[fi]] = reg.body[i];
                reg.freelink[fi]->link_pos[reg.freelink_pos[fi]] = j;
            }
        }
    }

    // 3. bodyatomから自由リンクを指す & 局所リンクを繋ぐ
    for (int i = 0; i < (int)rule.body.size(); i++) {
        for (int j = 0; j < (int)rule.body[i].link_functor.size(); j++) {
            // bodyのi番目のアトムのj番目のリンクについて
            int li = rule.body[i].link_reg[j];
            // li : bodyのi番目のアトムのj番目のリンク先のレジスタ番号

            if (rule.body[i].link_functor[j] == FL) {
                reg.body[i]->link[j] = reg.freelink[li];
                reg.body[i]->link_pos[j] = reg.freelink_pos[li];
            }else{
                reg.body[i]->link[j] = reg.body[li];
                reg.body[i]->link_pos[j] = rule.body[i].link_pos[j];
            }
        }
    }

    // 4. headatomを消す
    for (auto atom : reg.head) {
        atomlist[atom->functor].erase(atom->itr);
        delete atom;
    }

}

long long try_rule_cnt = 0;
bool try_rule(Rule &rule){
    try_rule_cnt++;
    Register reg;
    reg.head.resize(rule.head.size());
    reg.body.resize(rule.body.size());
    reg.freelink.resize(rule.freelink_num);
    reg.freelink_pos.resize(rule.freelink_num);
    if(find_atom(rule,reg)){
        commit(rule,reg);
        return true;
    }else{
        return false;
    }
}


/* ---------------------------------------------------------- */

// X = a(a(a(b(a(... (X)))))) 
void make_XabaaaaabaaaX_graph(int n){
    srand(time(NULL));
    Atom *front = new Atom(B2);
    front->add_to_atomlist();
    Atom *pre = front;
    for (int i = 0; i < n-1; i++) {
        Functor functor;
        if(rand()%10 == 0){
            functor = B2;
        }else{
            functor = A2;
        }

        Atom *cur = new Atom(functor);
        cur->add_to_atomlist();

        pre->link[0] = cur;
        pre->link_pos[0] = 1;
        cur->link[1] = pre;
        cur->link_pos[1] = 0;
        pre = cur;
    }

    pre->link[0] = front;
    pre->link_pos[0] = 1;
    front->link[1] = pre;
    front->link_pos[1] = 0;
}

// X = a(b(a(Y))) :- X = b(a(b(Y))).
Rule make_aba_rule(){
    Rule rule;
    rule.head = {
        Temp(A2, {B2,FL}, {1,-1}, {1,0}),
        Temp(B2, {A2,A2}, {1, 0}, {2,0}),
        Temp(A2, {FL,B2}, {-1,0}, {1,1})
    };
    rule.freelink_num = 2;
    rule.body = {
        Temp(B2, {A2,FL}, {1,-1}, {1,0}),
        Temp(A2, {B2,B2}, {1, 0}, {2,0}),
        Temp(B2, {FL,A2}, {-1,0}, {1,1})
    };
    return rule;
}


/* ---------------------------------------------------------- */

// X = b(a(a(a(a(... (X))))))
void make_baaab_graph(int n){
    srand(time(NULL));
    Atom *front = new Atom(B2);
    front->add_to_atomlist();
    Atom *pre = front;
    for (int i = 0; i < n; i++) {
        Atom *cur = new Atom(A2);
        cur->add_to_atomlist();

        pre->link[0] = cur;
        pre->link_pos[0] = 1;
        cur->link[1] = pre;
        cur->link_pos[1] = 0;
        pre = cur;
    }

    pre->link[0] = front;
    pre->link_pos[0] = 1;
    front->link[1] = pre;
    front->link_pos[1] = 0;
}

// X = a(a(Y)) :- X = a(Y)
Rule make_aa_to_a_rule(){
    Rule rule;
    rule.head = {
        Temp(A2, {A2,FL}, {1,-1}, {1,0}),
        Temp(A2, {FL,A2}, {-1,0}, {1,1})
    };
    rule.freelink_num = 2;
    rule.body = {
        Temp(A2, {FL,FL}, {-1,-1}, {1,0})
    };
    return rule;
}

/* ---------------------------------------------------------- */

// a,a,a,...,a.
void make_many_a_graph(int n){
    
    for (int i = 0; i < n; i++) {
        Atom *cur = new Atom(A0);
        cur->add_to_atomlist();
    }
}

// a,a :- b.
Rule make_a_a_none_rule(){
    Rule rule;
    rule.head = {
        Temp(A0, {}, {}, {}),
        Temp(A0, {}, {}, {})
    };
    rule.freelink_num = 0;
    rule.body = {
        Temp(B0, {}, {}, {})
    };
    return rule;
}

/* ---------------------------------------------------------- */



int main(void){
    // init

    // make_XabaaaaabaaaX_graph(4000);
    // rulelist = { make_aba_rule() };

    make_baaab_graph(50000);
    rulelist = { make_aa_to_a_rule() };

    // make_many_a_graph(11);
    // rulelist = { make_a_a_none_rule() };

    print(atomlist[A0].size());
    print(atomlist[B0].size());
    print(atomlist[A2].size());
    print(atomlist[B2].size());

    // execute rule
    while(true){
        bool success = false;
        for(Rule &rule : rulelist){
            if(try_rule(rule)){
                success = true;
                break;
            }
        }
        if(!success) break;
    }
    print(try_rule_cnt);

    print(atomlist[A0].size());
    print(atomlist[B0].size());
    print(atomlist[A2].size());
    print(atomlist[B2].size());
    
    return 0;
}
