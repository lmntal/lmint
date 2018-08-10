#include <bits/stdc++.h>
using namespace std;
#define INF 1e9
using Functor = pair<string,int>;
// using Functor = int;

Functor a2={"a",2},b2={"b",2},_={"",0};
// Functor a2=1,b2=2,_=0;

class Atom {
  public:
    Functor functor; // <name,arity>
    // vector<shared_ptr<Atom>> link;
    vector<Atom*> link;
    vector<int> link_pos;
    // list<shared_ptr<Atom>>::iterator itr;
    list<Atom*>::iterator itr;

    Atom(){}

    Atom(Functor _functor){

        functor = _functor;
        int arity = functor.second;
        // int arity = 2;
        // link = vector<shared_ptr<Atom>>(arity);
        link = vector<Atom*>(arity);
        link_pos = vector<int>(arity);
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
    Temp(   Functor _functor, 
            vector<Functor> _link_functor,
            vector<int> _link_pos,
            vector<int> _link_reg)
    {
        functor = _functor;
        link_functor = _link_functor;
        link_pos = _link_pos;
        link_reg = _link_reg;
    }

};

class Guard{
  public:
    string function;
    vector<string> variable;
};

// Matching 時に使う変数的役割
class Register {
  public:
    // vector<shared_ptr<Atom>> head;
    // vector<shared_ptr<Atom>> body;
    // vector<shared_ptr<Atom>> freelink;
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


// map<Functor,list<shared_ptr<Atom>>> atomlist;
map<Functor,list<Atom*>> atomlist;
// list<shared_ptr<Atom>> atomlist[3];
vector<Rule> rulelist;


// bool set_atom_to_reg(Rule &rule, Register &reg, shared_ptr<Atom> atom, int hi){
bool set_atom_to_reg(Rule &rule, Register &reg, Atom* atom, int hi){
    Functor functor = atom->functor;
    reg.head[hi] = atom;

    // 他でreg登録済みのアトムならだめ

    /* リンクが合っているか */
    for (int i = 0; i < functor.second; i++){  
    // for (int i = 0; i < 2; i++){      
        // shared_ptr<Atom> dst_atom = atom->link[i];
        Atom* dst_atom = atom->link[i];
        // 自由リンク
        if(rule.head[hi].link_functor[i] == _){
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
    for (int i = 0; i < functor.second; i++){
    // for (int i = 0; i < 2; i++){      
        // shared_ptr<Atom> dst_atom = atom->link[i];
        Atom* dst_atom = atom->link[i];
        int link_reg = rule.head[hi].link_reg[i];

        if(rule.head[hi].link_functor[i] == _){
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

// void remove_atom_from_reg(Rule &rule, Register &reg, shared_ptr<Atom> atom, int hi){
void remove_atom_from_reg(Rule &rule, Register &reg, Atom* atom, int hi){
    Functor functor = atom->functor;
    reg.head[hi] = NULL;

    for (int i = 0; i < functor.second; i++){
    // for (int i = 0; i < 2; i++){      

        // shared_ptr<Atom> dst_atom = atom->link[i];
        Atom* dst_atom = atom->link[i];
        int link_reg = rule.head[hi].link_reg[i];

        if(rule.head[hi].link_functor[i] == _){
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
    for(auto &atom : reg.head){
        if(atom==NULL){
            cout<<"head NULL"<<endl;
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
        // reg.body[i] = make_shared<Atom>(rule.body[i].functor);
        reg.body[i] = new Atom(rule.body[i].functor);
        atomlist[rule.body[i].functor].push_back(reg.body[i]);
        reg.body[i]->itr = --(atomlist[rule.body[i].functor].end());
    }

    // 2. 自由リンクの先からこちらを向いてるものを新しいアトムに指す
    for (int i = 0; i < (int)rule.body.size(); i++) {
        for (int j = 0; j < (int)rule.body[i].link_functor.size(); j++) {
            // bodyのi番目のアトムのj番目のリンクについて

            if (rule.body[i].link_functor[j] == _) {
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

            if (rule.body[i].link_functor[j] == _) {
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
    }

}

long try_rule_cnt=0;
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


void make_graph(int n){
    // shared_ptr<Atom> front = make_shared<Atom>(b2);
    // shared_ptr<Atom> pre = front;
    Atom* front = new Atom(b2);
    Atom* pre = front;
    atomlist[b2].push_back(front);
    front->itr = --(atomlist[b2].end());
    for (int i = 0; i < n-1; i++) {
        Functor functor;
        if(rand()%10 == 0){
            functor = b2;
        }else{
            functor = a2;
        }

        // shared_ptr<Atom> cur = make_shared<Atom>(functor);
        Atom* cur = new Atom(functor);
        atomlist[functor].push_back(cur);
        cur->itr = --(atomlist[functor].end());

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

int main(void){

    // 初期グラフの生成
    make_graph(40000);
    cout<<"a2 atomlist size : "<<atomlist[a2].size()<<endl;
    cout<<"b2 atomlist size : "<<atomlist[b2].size()<<endl;
    


    Rule aba;
    // Temp(_functor, {_link_functor}, {_link_pos}, {_link_reg})
    
    aba.head.resize(3);
    aba.head[0] = Temp(a2, {b2,_ }, {1,-1}, {1,0});
    aba.head[1] = Temp(b2, {a2,a2}, {1, 0}, {2,0});
    aba.head[2] = Temp(a2, {_ ,b2}, {-1,0}, {1,1});

    aba.freelink_num = 2;

    aba.body.resize(3);
    aba.body[0] = Temp(b2, {a2,_ }, {1,-1}, {1,0});
    aba.body[1] = Temp(a2, {b2,b2}, {1, 0}, {2,0});
    aba.body[2] = Temp(b2, {_ ,a2}, {-1,0}, {1,1});

    rulelist.push_back(aba);

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

    cout<<"a2 atomlist size : "<<atomlist[a2].size()<<endl;
    cout<<"b2 atomlist size : "<<atomlist[b2].size()<<endl;
    cout<<"cnt : "<<try_rule_cnt<<endl;
    
    return 0;
}
