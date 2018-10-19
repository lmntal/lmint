#include "myslim.hpp"

bool find_atom(Rule &rule, Register &reg);
bool set_atom_to_reg(Rule &rule, Register &reg, Atom* atom, int hi);
void remove_atom_from_reg(Rule &rule, Register &reg, Atom* atom, int hi);
void rewrite(Rule &rule, Register &reg);

/*
    ルール内の未決定のアトムを1つ選び、
    それにマッチするアトムをグラフの中から1つ選びレジスタに格納する
*/
bool find_atom(Rule &rule, Register &reg) {
    // 未決定アトムの中で最小のアトムリストのものを選ぶ
    int min_atomlist_size = INF, hi = -1;
    for (int i = 0; i < (int)reg.head.size(); i++) {
        if (reg.head[i] != NULL) continue;
        int atomlist_size = atomlist[rule.head[i]->functor].size();
        if (min_atomlist_size > atomlist_size) {
            min_atomlist_size = atomlist_size;
            hi = i;
        }
    }
    if (hi == -1) {
        return true;
    } else {
        Functor functor = rule.head[hi]->functor;
        for (auto &atom : atomlist[functor]) {
            bool ok = set_atom_to_reg(rule, reg, atom, hi);
            if (ok) {
                if (find_atom(rule, reg)) return true;
                else remove_atom_from_reg(rule, reg, atom, hi);
            }
        }
        return false;
    }
}


/*
    reg.head[hi] に atom をいれて問題がないか検査
    問題がなければ reg にいれて true を返す
    問題があれば reg から外して false を返す
    再帰的に連結グラフのアトムを検査
*/
bool set_atom_to_reg(Rule &rule, Register &reg, Atom* atom, int hi) {

    // 他でreg登録済みのアトム
    for (Atom *registered_atom : reg.head) {
        if (atom == registered_atom) return false;
    }

    reg.head[hi] = atom;
    RuleAtom *rule_atom = rule.head[hi];

    /* リンクが合っているか */
    int arity = atom->functor.arity;
    for (int i = 0; i < arity; i++) {

        // 自由リンク
        if (rule_atom->link[i].is_freelink()) {
            continue;
        }

        Atom* dst_atom = atom->link[i].atom;

        // リンク先のファンクタが違う
        if (dst_atom->functor != rule_atom->link[i].atom->functor) {
            reg.head[hi] = NULL;
            return false;
        }

        // 接続場所が合っているか
        int dst_pos = rule_atom->link[i].pos;
        if (dst_atom->link[dst_pos].atom != atom ||
            dst_atom->link[dst_pos].pos != i) {
            reg.head[hi] = NULL;
            return false;
        }
    }

    /* リンクをレジスタに登録 */
    for (int i = 0; i < arity; i++) {

        // 自由リンク
        if (rule_atom->link[i].is_freelink()) {
            reg.freelink[rule_atom->link[i].freelinkID()] = atom->link[i];
            continue;
        }

        Atom* dst_atom = atom->link[i].atom;
        int dst_id = rule_atom->link[i].atom->id; // ルールでは来るはずのid

        // 自由リンクでない かつ レジスタ[dst_id]は未登録
        if (reg.head[dst_id] == NULL) {
            bool ok = set_atom_to_reg(rule, reg, dst_atom, dst_id);
            if (!ok) {
                reg.head[hi] = NULL;
                return false;
            }
        }
        // 自由リンクでない かつ レジスタ[dst_id]は登録済み
        else if (reg.head[dst_id] != NULL) {
            if (reg.head[dst_id]->link[rule_atom->link[i].pos].atom != atom) {
                reg.head[hi] = NULL;
                return false;
            }
        }
    }

    return true;
}

void remove_atom_from_reg(Rule &rule, Register &reg, Atom* atom, int hi) {
    Functor functor = atom->functor;
    reg.head[hi] = NULL;

    int arity = functor.arity;
    for (int i = 0; i < arity; i++) {

        // 自由リンク
        if (rule.head[hi]->link[i].is_freelink()) {
            int fi = rule.head[hi]->link[i].freelinkID();
            reg.freelink[fi] = Link(NULL, 0);
            continue;
        }
        
        Atom* dst_atom = atom->link[i].atom;
        int dst_id = rule.head[hi]->link[i].atom->id;
        // 自由リンクでない かつ レジスタ登録済み
        if (reg.head[dst_id] != NULL) {
            remove_atom_from_reg(rule, reg, dst_atom, dst_id);
        }
    }
}

void rewrite(Rule &rule, Register &reg) {
    for (Atom* &atom : reg.head) {
        assert(atom != NULL);
    }
/*
    freelinkがheadを指しているとき、
    そのfreelinkを新しいほうにつなぎ変えてからもとのheadを消す必要がある
*/

    const int rule_body_size = rule.body.size();

    // 1. 新しいアトムを生成する
    for (int i = 0; i < rule_body_size; i++) {
        reg.body[i] = new Atom(rule.body[i]->functor);
    }

    // 2. 自由リンクの先からこちらを向いてるものを新しいアトムに指す
    for (int i = 0; i < rule_body_size; i++) {
        const int arity = rule.body[i]->functor.arity;
        for (int j = 0; j < arity; j++) {
            // bodyのi番目のアトムのj番目のリンクについて

            if (rule.body[i]->link[j].is_freelink()) {
                int fi = rule.body[i]->link[j].freelinkID();
                int dst_pos = reg.freelink[fi].pos;
                reg.freelink[fi].atom->link[dst_pos].atom = reg.body[i];
                reg.freelink[fi].atom->link[dst_pos].pos = j;
            }
        }
    }

    // 3. bodyatomから自由リンクを指す & 局所リンクを繋ぐ
    for (int i = 0; i < rule_body_size; i++) {
        const int arity = rule.body[i]->functor.arity;
        for (int j = 0; j < arity; j++) {
            // bodyのi番目のアトムのj番目のリンクについて
            if (rule.body[i]->link[j].is_freelink()) {
                int fi = rule.body[i]->link[j].freelinkID();
                reg.body[i]->link[j] = reg.freelink[fi];
            } else {
                int li = rule.body[i]->link[j].atom->id;
                reg.body[i]->link[j].atom = reg.body[li];
                reg.body[i]->link[j].pos = rule.body[i]->link[j].pos;
            }
        }
    }

    // 4. 自由リンク同士の接続
    for (pair<int,int> &p : rule.connect) {
        int u = p.first;
        int v = p.second;
        reg.freelink[u].atom->link[reg.freelink[u].pos] = reg.freelink[v];
        reg.freelink[v].atom->link[reg.freelink[v].pos] = reg.freelink[u];
    }

    // 5. headatomを消す
    for (auto &atom : reg.head) {
        delete atom;
    }

}

long long num_rules_success = 0;
bool try_rule(Rule &rule) {
    Register reg(rule);
    if (find_atom(rule,reg)) {
        rewrite(rule,reg);
        num_rules_success++;
        return true;
    } else {
        return false;
    }
}


/* ---------------------------------------------------------- */

const Functor A2 = {2, "a"};
const Functor A0 = {0, "a"};
const Functor B2 = {2, "b"};
const Functor B0 = {0, "b"};

// X = a(a(a(b(a(... (X))))))
void make_XabaaaaabaaaX_graph(int n) {
    srand(time(NULL));
    Atom *front = new Atom(B2);
    Atom *pre = front;
    for (int i = 0; i < n-1; i++) {
        Functor functor;
        if (rand()%10 == 0) {
            functor = B2;
        } else {
            functor = A2;
        }

        Atom *cur = new Atom(functor);

        pre->link[0].atom = cur;
        pre->link[0].pos = 1;
        cur->link[1].atom = pre;
        cur->link[1].pos = 0;
        pre = cur;
    }

    pre->link[0].atom = front;
    pre->link[0].pos = 1;
    front->link[1].atom = pre;
    front->link[1].pos = 0;
}

// X = a(b(a(Y))) :- X = b(a(b(Y))).
Rule make_aba_rule() {
    Rule rule;
    rule.head = {
        new RuleAtom(A2, 0, {}),
        new RuleAtom(B2, 1, {}),
        new RuleAtom(A2, 2, {}),
    };
    rule.head[0]->link = {RuleLink(rule.head[1], 1), RuleLink(NULL, 0)};
    rule.head[1]->link = {RuleLink(rule.head[2], 1), RuleLink(rule.head[0], 0)};
    rule.head[2]->link = {RuleLink(NULL, 1), RuleLink(rule.head[1], 0)};
    rule.freelink_num = 2;
    rule.body = {
        new RuleAtom(B2, 0, {}),
        new RuleAtom(A2, 1, {}),
        new RuleAtom(B2, 2, {}),
    };
    rule.body[0]->link = {RuleLink(rule.body[1], 1), RuleLink(NULL, 0)};
    rule.body[1]->link = {RuleLink(rule.body[2], 1), RuleLink(rule.body[0], 0)};
    rule.body[2]->link = {RuleLink(NULL, 1), RuleLink(rule.body[1], 0)};
    return rule;
}


/* ---------------------------------------------------------- */

// X = b(a(a(a(a(... (X))))))
void make_baaab_graph(int n) {
    srand(time(NULL));
    Atom *front = new Atom(B2);
    Atom *pre = front;
    for (int i = 0; i < n; i++) {
        Atom *cur = new Atom(A2);

        pre->link[0].atom = cur;
        pre->link[0].pos = 1;
        cur->link[1].atom = pre;
        cur->link[1].pos = 0;
        pre = cur;
    }

    pre->link[0].atom = front;
    pre->link[0].pos = 1;
    front->link[1].atom = pre;
    front->link[1].pos = 0;
}

// X = a(a(Y)) :- X = a(Y)
Rule make_aa_to_a_rule() {
    Rule rule;
    rule.head = {
        new RuleAtom(A2, 0, {}),
        new RuleAtom(A2, 1, {}),
    };
    rule.head[0]->link = {RuleLink(rule.head[1], 1), RuleLink(NULL, 0)};
    rule.head[1]->link = {RuleLink(NULL, 1), RuleLink(rule.head[0], 0)};
    rule.freelink_num = 2;
    rule.body = {
        new RuleAtom(A2, 0, {}),
    };
    rule.body[0]->link = {RuleLink(NULL, 1), RuleLink(NULL, 0)};
    return rule;
}

// X = a(Y) :- X = Y
Rule make_a_to_XY_rule() {
    Rule rule;
    rule.head = {
        new RuleAtom(A2, 0, {}),
    };
    rule.head[0]->link = {RuleLink(NULL, 1), RuleLink(NULL, 0)};
    rule.freelink_num = 2;
    rule.connect = {{0,1}};
    return rule;
}
/* ---------------------------------------------------------- */

// a,a,a,...,a.
void make_many_a(int n) {
    for (int i = 0; i < n; i++) {
        new Atom(A0);
    }
}

// a,a :- b.
Rule make_a_a_to_b_rule() {
    Rule rule;
    rule.head = {
        new RuleAtom(A0, 0, {}),
        new RuleAtom(A0, 1, {}),
    };
    rule.freelink_num = 0;
    rule.body = {
        new RuleAtom(B0, 0, {}),
    };
    return rule;
}

/* ---------------------------------------------------------- */



int main(void) {
    // init

    // make_XabaaaaabaaaX_graph(100000);
    // rulelist = { make_aba_rule() };

    make_baaab_graph(10000000);
    // rulelist = { make_aa_to_a_rule() };
    rulelist = { make_a_to_XY_rule() };

    // make_many_a(10000000);
    // rulelist = { make_a_a_to_b_rule() };

    for (auto al : atomlist) {
        cout << al.first << " size = " << al.second.size() << endl;
    }

    // execute rule
    while (true) {
        bool success = false;
        for (Rule &rule : rulelist) {
            if (try_rule(rule)) {
                success = true;
                break;
            }
        }
        if (!success) break;
    }
    print(num_rules_success);

    for (auto al : atomlist) {
        cout << al.first << " size = " << al.second.size() << endl;
    }

    return 0;
}

