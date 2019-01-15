#include "rule.hpp"
#include "parser.hpp"

map<Functor,list<Atom*>> atomlist;
vector<Rule> rulelist;

Functor::Functor() {}

Functor::~Functor() {}

Functor::Functor(int arity_, string name_): arity(arity_), name(name_) {}

bool Functor::operator==(const Functor &rhs) const {
    return arity == rhs.arity && name == rhs.name;
}

bool Functor::operator!=(const Functor &rhs) const {
    return arity != rhs.arity || name != rhs.name;
}

bool Functor::is_int() const {
    return isdigit(name[0]);
}

bool Functor::operator<(const Functor &rhs) const {
    return name != rhs.name ? name < rhs.name : arity < rhs.arity;
}

ostream& operator<<(ostream& ost, const Functor &rhs) {
    ost << "<" << rhs.name << "," << rhs.arity << ">";
    return ost;
}


Link::Link() {}

Link::~Link() {}

Link::Link(Atom *atom_, int pos_): atom(atom_), pos(pos_) {}

bool Link::operator<(const Link &rhs) const {
    return atom != rhs.atom ? atom < rhs.atom : pos < rhs.pos;
}


Atom::Atom() {}

Atom::~Atom() {
    assert(*itr == this);
    atomlist[functor].erase(itr);
}

Atom::Atom(Functor functor_): functor(functor_) {
    link.resize(functor.arity);
    atomlist[functor].push_front(this);
    itr = atomlist[functor].begin();
}

bool Atom::is_int() {
    return functor.arity == 1 && isdigit(functor.name[0]);
}


RuleLink::RuleLink() {}

RuleLink::~RuleLink() {}

RuleLink::RuleLink(int pos_): atom(NULL), pos(pos_) {}

RuleLink::RuleLink(RuleAtom *atom_, int pos_): atom(atom_), pos(pos_) {}

bool RuleLink::is_freelink() {
    return atom == NULL;
}

int RuleLink::freelinkID() {
    assert(is_freelink());
    return pos;
}


RuleAtom::RuleAtom() {}

RuleAtom::~RuleAtom() {}

RuleAtom::RuleAtom(Functor functor_, int id_): functor(functor_), id(id_) {
    link.resize(functor.arity);
}

RuleAtom::RuleAtom(Functor functor_, int id_, vector<RuleLink> link_):
    functor(functor_), id(id_), link(link_) {}


Guard::Compare::Compare() {}

Guard::Compare::~Compare() {}

Guard::Compare::Compare(vector<string> &left_exp_, string &op_, vector<string> &right_exp_):
    left_exp(left_exp_), op(op_), right_exp(right_exp_) {}

bool Guard::Compare::is_null() {
    return op == "";
}


Guard::TypeCheck::TypeCheck() {}

Guard::TypeCheck::~TypeCheck() {}

Guard::TypeCheck::TypeCheck(string link_, string type_): link(link_), type(type_) {}

bool Guard::TypeCheck::is_null() {
    return type == "";
}


Guard::Assign::Assign() {}

Guard::Assign::~Assign() {}


Guard::Guard() {}

Guard::~Guard() {}

bool Guard::is_null() {
    return type_check.empty() && compare.empty();
}


Rule::Rule(): freelink_num(0) {}

Rule::~Rule() {}

void Rule::show() {
    cout << "----------- head -----------" << endl;
    for (int i = 0; i < (int)head.size(); i++) {
        cout << head[i]->functor << " [" << head[i] << "] (";
        int arity = (int)head[i]->functor.arity;
        for (int j = 0; j < arity; j++) {
            cout << ((head[i]->link[j].is_freelink()) ? "FL:" : "LL:")
                 << head[i]->link[j].atom << "(" << head[i]->link[j].pos << ")"
                 << (j + 1 == arity ? "" : ", ");
        }
        cout << ")" << endl;
    }
    cout << "----------- body -----------" << endl;
    for (int i = 0; i < (int)body.size(); i++) {
        cout << body[i]->functor << " [" << body[i] << "] (";
        int arity = (int)body[i]->functor.arity;
        for (int j = 0; j < arity; j++) {
            cout << ((body[i]->link[j].is_freelink()) ? "FL:" : "LL:")
                 << body[i]->link[j].atom << "(" << body[i]->link[j].pos << ")"
                 << (j + 1 == arity ? "" : ", ");
        }
        cout << ")" << endl;
    }
    for (auto &p : connector) {
        cout << p.first << " = " << p.second << endl;
    }
}


Register::Register() {}

Register::~Register() {}

Register::Register(Rule &rule) {
    head.resize(rule.head.size());
    body.resize(rule.body.size());
    freelink.resize(rule.freelink_num);
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

/*
    ルール内の未決定のアトムを1つ選び、
    それにマッチするアトムをグラフの中から1つ選びレジスタに格納する
*/

bool find_atom(Rule &rule, Register &reg) {
    // 未決定アトムの中で最小のアトムリストのものを選ぶ
    int min_atomlist_size = -1, hi = -1;
    for (int i = 0; i < (int)reg.head.size(); i++) {
        if (reg.head[i] != NULL) continue;
        int atomlist_size = atomlist[rule.head[i]->functor].size();
        if (min_atomlist_size > atomlist_size || hi == -1) {
            min_atomlist_size = atomlist_size;
            hi = i;
        }
    }
    if (hi == -1) {
        return guard_check(rule, reg);
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


/*
    TOP := '+' | '-'
    FOP := '*' | '/' | 'mod'
    Exp := Term { TOP Term }
    Term := Factor { FOP Factor }
    Factor := Link | Num | '(' Exp ')' | TOP Factor
    Num := [0-9]+
*/

int eval_factor(Rule &rule, Register &reg, vector<string> &tokens, int &i) {
    if (tokens[i][0] == '#') {
        int free_link_id = std::stoi(tokens[i].substr(1));
        i++;
        return std::stoi(reg.freelink[free_link_id].atom->functor.name);
    }

    if (isdigit(tokens[i][0])) {
        int result = std::stoi(tokens[i]);
        i++;
        return result;
    }

    if (tokens[i] == "(") {
        i++;
        int exp = eval_exp(rule, reg, tokens, i);
        assert(tokens[i] == ")");
        i++;
        return exp;
    }

    if (tokens[i] == "+") {
        i++;
        return eval_factor(rule, reg, tokens, i);
    }

    if (tokens[i] == "-") {
        i++;
        return - eval_factor(rule, reg, tokens, i);
    }

    if (tokens[i] == "rand") {
        i++;
        assert(tokens[i] == "(");
        i++;
        int rand_max = std::stoi(tokens[i]);
        assert(tokens[i] == ")");
        i++;

        static std::random_device seed_gen;
        static std::mt19937 mt(seed_gen());
        std::uniform_int_distribution<> rand_int(0, rand_max-1);
        return rand_int(mt);
    }

    assert(false);
}

int eval_term(Rule &rule, Register &reg, vector<string> &tokens, int &i) {
    int result = eval_factor(rule, reg, tokens, i);
    while (true) {
        if ((int)tokens.size() == i) break;
        if (tokens[i] != "*" && tokens[i] != "/") break;
        string op = tokens[i];
        i++;
        int term = eval_factor(rule, reg, tokens, i);
        if (op == "*") {
            result *= term;
        } else if (op == "/") {
            result /= term;
        } else {
            assert(false);
        }
    }
    return result;
}

int eval_exp(Rule &rule, Register &reg, vector<string> &tokens, int &i) {
    int result = eval_term(rule, reg, tokens, i);
    while (true) {
        if ((int)tokens.size() == i) break;
        if (tokens[i] != "+" && tokens[i] != "-") break;
        string &op = tokens[i];
        i++;
        int term = eval_term(rule, reg, tokens, i);
        if (op == "+") {
            result += term;
        } else if (op == "-") {
            result -= term;
        } else {
            assert(false);
        }
    }
    return result;
}

bool eval_compare(int left_exp, string &op, int right_exp) {
    if (op == "=:=") return left_exp == right_exp;
    if (op == "=\\=") return left_exp != right_exp;
    if (op == "=<") return left_exp <= right_exp;
    if (op == "<") return left_exp < right_exp;
    if (op == ">=") return left_exp >= right_exp;
    if (op == ">") return left_exp > right_exp;
    assert(false);
}

bool guard_check(Rule &rule, Register &reg) {
    // for (auto &assign : rule.guard.assign) {

    // }

    for (auto &c : rule.guard.compare) {
        
        // TODO: リンクがすべてintであることを確認

        int i = 0;
        int left_exp = eval_exp(rule, reg, c.left_exp, i);

        i = 0;
        int right_exp = eval_exp(rule, reg, c.right_exp, i);

        if (!eval_compare(left_exp, c.op, right_exp)) {
            return false;
        }
    }
    return true;
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
    for (pair<int,int> &p : rule.connector) {
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

/* ----------------------------- dump ----------------------------- */

void show_graph() {
    map<pair<Atom*, int>, string> mp;
    for (auto al : atomlist) {
        Functor functor = al.first;
        auto &list = al.second;
        cout << functor << " size = " << list.size() << endl;
    }
}

void nest_dump(Atom* atom, int depth, unordered_set<Atom*> &dumped_atoms, map<Link, int> &locallink_id) {
    
    dumped_atoms.insert(atom);
    Functor &functor = atom->functor;
    cout << functor.name;
    int args = functor.arity + (depth > 0 ? -1 : 0);
    if (args == 0) return;
    cout << "(";
    for (int i = 0; i < args; i++) {
        if (locallink_id.find(atom->link[i]) != locallink_id.end()) {
            cout << "L" << locallink_id[atom->link[i]];
        } else {
            Atom *dst_atom = atom->link[i].atom;
            int dst_pos = atom->link[i].pos;
            if (dst_atom->functor.arity == dst_pos + 1 && 
                depth < 10000 &&
                dumped_atoms.find(dst_atom) == dumped_atoms.end())
            {
                nest_dump(dst_atom, depth+1, dumped_atoms, locallink_id);
            } else {
                int id = locallink_id.size();
                locallink_id[Link(atom, i)] = id;
                cout << "L" << id;
            }
        }
        if (i < args-1) cout << ",";
    }
    cout << ")";
}


void dump() {
    vector<pair<int,Functor>> size_functor_pairs;
    for (auto &itr : atomlist) {
        size_functor_pairs.push_back({itr.second.size(), itr.first});
    }
    sort(size_functor_pairs.begin(), size_functor_pairs.end());
    unordered_set<Atom*> dumped_atoms;
    map<Link, int> locallink_id;
    for (pair<int,Functor> &p : size_functor_pairs) {
        Functor &functor = p.second;
        if (functor.is_int()) continue;
        for (Atom* atom : atomlist[functor]) {
            if (dumped_atoms.find(atom) == dumped_atoms.end()) {
                nest_dump(atom, 0, dumped_atoms, locallink_id);
                cout << ". ";
            }
        }
    }
    cout << endl;
}

void show_atomlist_size() {
    cout << "------------ atom list size ------------" << endl;
    for (auto &al : atomlist) {
        cout << al.first << " size = " << al.second.size() << endl;
    }
}

void show_rules() {
    int R = rulelist.size();
    for (int i = 0; i < R; i++) {
        printf("------------ Rule %d ------------\n", i);
        rulelist[i].show();
    }
}

int main(void) {
    Parser parser;
    parser.parse();
    load(parser); // vm.load(parser);
    // show_rules();
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
    // debug(num_rules_success);
    dump();
    return 0;
}

