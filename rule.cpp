#include "rule.hpp"
#include "parser.hpp"

map<Functor,list<Atom*>> atomlist;
vector<Rule> rulelist;
map<Functor, map<int, unordered_map<string, list<Atom*>>>> unary_indexed_atomlist;
// unary_indexed_atomlist[dst_functor][dst_pos][unary_name] = {*dst_atom, ...}

unordered_map<Atom*, list<Atom*>::iterator> unary_indexed_atom_itr;
// unary_indexed_atom_itr[*unary_atom] = unary_indexed_atomlist::iterator<*dst_atom>

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


Link::Link(): atom(NULL), pos(0) {}
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

// atom1のpos1にatom2のpos2をつなぐ
void connect_links(Atom *atom1, int pos1, Atom *atom2, int pos2) {
    // atom1のpos1にあったLinkがunary
    Atom *dst_atom = atom1->link[pos1].atom;
    if (dst_atom != NULL && dst_atom->functor.arity == 1) {
        unary_indexed_atomlist[atom1->functor][pos1][dst_atom->functor.name].erase(
            unary_indexed_atom_itr[dst_atom]
        );
    }

    // atom1のpos1にatom2のpos2をつなぐ
    atom1->link[pos1].atom = atom2;
    atom1->link[pos1].pos = pos2;

    // atom2がunary
    if (atom2->functor.arity == 1) {
        unary_indexed_atomlist[atom1->functor][pos1][atom2->functor.name].emplace_front(atom1);
        unary_indexed_atom_itr[atom2]
            = unary_indexed_atomlist[atom1->functor][pos1][atom2->functor.name].begin();
    }
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
    return type_checks.empty() && compares.empty();
}


Rule::Rule(): freelink_num(0) {}
Rule::~Rule() {}

void Rule::show() {
    cout << "----------- head -----------" << endl;
    for (int i = 0; i < (int)head_atoms.size(); i++) {
        cout << head_atoms[i]->functor << " [" << head_atoms[i] << "] (";
        int arity = (int)head_atoms[i]->functor.arity;
        for (int j = 0; j < arity; j++) {
            cout << ((head_atoms[i]->link[j].is_freelink()) ? "FL:" : "LL:")
                 << head_atoms[i]->link[j].atom << "(" << head_atoms[i]->link[j].pos << ")"
                 << (j + 1 == arity ? "" : ", ");
        }
        cout << ")" << endl;
    }
    cout << "----------- body -----------" << endl;
    for (int i = 0; i < (int)body_atoms.size(); i++) {
        cout << body_atoms[i]->functor << " [" << body_atoms[i] << "] (";
        int arity = (int)body_atoms[i]->functor.arity;
        for (int j = 0; j < arity; j++) {
            cout << ((body_atoms[i]->link[j].is_freelink()) ? "FL:" : "LL:")
                 << body_atoms[i]->link[j].atom << "(" << body_atoms[i]->link[j].pos << ")"
                 << (j + 1 == arity ? "" : ", ");
        }
        cout << ")" << endl;
    }
    for (auto &p : connectors) {
        cout << p.first << " = " << p.second << endl;
    }
}


Register::Register() {}
Register::~Register() {}

Register::Register(Rule &rule) {
    head_atoms.resize(rule.head_atoms.size());
    body_atoms.resize(rule.body_atoms.size());
    freelinks.resize(rule.freelink_num);
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

list<Atom*>* get_unary_indexed_atomlist(
    Functor &dst_functor, int dst_pos, string &unary_name)
{
    if (unary_indexed_atomlist[dst_functor][dst_pos].count(unary_name) == 0) {
        return NULL;
    }
    return &unary_indexed_atomlist[dst_functor][dst_pos][unary_name];
}

//   [a_0, ... , a_k-1, a_k (itr), ... , a_n-1]
// ->[a_k (itr), ... , a_n-1, a_0, ... , a_k-1]
void splice(list<Atom*> &a, list<Atom*>::iterator &itr) {
    a.splice(a.end(), a, a.begin(), itr);
}

long long back_track = 0;

bool find_atom(Rule &rule, Register &reg) {
    // 未決定アトムの中で最小のアトムリストのものを選ぶ
    int head_id = -1;
    list<Atom*> *start_point_list = NULL;
    if (!guard_check(rule, reg)) return false;

    for (int i = 0; i < (int)reg.head_atoms.size(); i++) {
        if (reg.head_atoms[i] != NULL) continue;

        list<Atom*> *atomlist_i = &atomlist[rule.head_atoms[i]->functor];
        if (start_point_list == NULL ||
            start_point_list->size() > atomlist_i->size())
        {
            start_point_list = atomlist_i;
            head_id = i;
        }

        int arity = rule.head_atoms[i]->functor.arity;
        for (int j = 0; j < arity; j++) {
            if (!rule.head_atoms[i]->link[j].is_freelink()) continue;
            int fid = rule.head_atoms[i]->link[j].freelinkID();
            if (reg.expected_unary.count(fid)) {
                atomlist_i = get_unary_indexed_atomlist(rule.head_atoms[i]->functor, j, reg.expected_unary[fid]);
                if (atomlist_i == NULL) {
                    back_track++;
                    return false;
                }
                if (start_point_list->size() > atomlist_i->size()) {
                    start_point_list = atomlist_i;
                    head_id = i;
                }
            }
        }
    }


    if (start_point_list == NULL) {
        return true;
    } else {
        for (auto itr = start_point_list->begin(); itr != start_point_list->end(); ++itr) {
            Atom* atom = *itr;
            bool ok = set_atom_to_reg(rule, reg, atom, head_id);
            if (ok) {
                if (find_atom(rule, reg)) {
                    splice(*start_point_list, itr);
                    return true;
                } else {
                    remove_atom_from_reg(rule, reg, atom, head_id);
                }
            }
            back_track++;
        }
        return false;
    }
}


/*
    reg.head_atoms[head_id] に atom をいれて問題がないか検査
    問題がなければ reg にいれて true を返す
    問題があれば reg から外して false を返す
    再帰的に連結グラフのアトムを検査
*/

bool set_atom_to_reg(Rule &rule, Register &reg, Atom* atom, int head_id) {

    // 他でreg登録済みのアトム
    for (Atom *registered_atom : reg.head_atoms) {
        if (atom == registered_atom) return false;
    }

    reg.head_atoms[head_id] = atom;
    RuleAtom *rule_atom = rule.head_atoms[head_id];
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
            reg.head_atoms[head_id] = NULL;
            return false;
        }
        // 接続場所が合っているか
        int dst_pos = rule_atom->link[i].pos;
        if (dst_atom->link[dst_pos].atom != atom ||
            dst_atom->link[dst_pos].pos != i) {
            reg.head_atoms[head_id] = NULL;
            return false;
        }
    }
    /* リンクをレジスタに登録 */
    for (int i = 0; i < arity; i++) {
        // 自由リンク
        if (rule_atom->link[i].is_freelink()) {
            reg.freelinks[rule_atom->link[i].freelinkID()] = atom->link[i];
            continue;
        }

        Atom* dst_atom = atom->link[i].atom;
        int dst_id = rule_atom->link[i].atom->id; // ルールでは来るはずのid
        // 自由リンクでない かつ レジスタ[dst_id]は未登録
        if (reg.head_atoms[dst_id] == NULL) {
            bool ok = set_atom_to_reg(rule, reg, dst_atom, dst_id);
            if (!ok) {
                reg.head_atoms[head_id] = NULL;
                return false;
            }
        }
        // 自由リンクでない かつ レジスタ[dst_id]は登録済み
        else if (reg.head_atoms[dst_id] != NULL) {
            if (reg.head_atoms[dst_id]->link[rule_atom->link[i].pos].atom != atom) {
                reg.head_atoms[head_id] = NULL;
                return false;
            }
        }
    }

    return true;
}

void remove_atom_from_reg(Rule &rule, Register &reg, Atom* atom, int head_id) {
    Functor functor = atom->functor;
    reg.head_atoms[head_id] = NULL;

    int arity = functor.arity;
    for (int i = 0; i < arity; i++) {

        // 自由リンク
        if (rule.head_atoms[head_id]->link[i].is_freelink()) {
            int fi = rule.head_atoms[head_id]->link[i].freelinkID();
            reg.freelinks[fi] = Link(NULL, 0);
            continue;
        }
        
        Atom* dst_atom = atom->link[i].atom;
        int dst_id = rule.head_atoms[head_id]->link[i].atom->id;
        // 自由リンクでない かつ レジスタ登録済み
        if (reg.head_atoms[dst_id] != NULL) {
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
        int freelink_id = std::stoi(tokens[i].substr(1));
        i++;
        return std::stoi(reg.freelinks[freelink_id].atom->functor.name);
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

bool vars_in_exp_are_completed(Register &reg, vector<string> &tokens) {
    for (string &token : tokens) {
        if (token[0] == '#') {
            int freelink_id = std::stoi(token.substr(1));
            if (reg.freelinks[freelink_id].atom == NULL) {
                return false;
            }
        }
    }
    return true;
}



// そこまでで評価ができそうなGuard文の検査
// expected_unary のチェックも行う
bool guard_check(Rule &rule, Register &reg) {

    for (auto &compare : rule.guard.compares) {
        // TODO: リンクがすべてintであることを確認

        bool left_exp_is_completed = vars_in_exp_are_completed(reg, compare.left_exp);
        bool right_exp_is_completed = vars_in_exp_are_completed(reg, compare.right_exp);
        if (!left_exp_is_completed && !right_exp_is_completed) {
            continue;
        }
        int i, left_exp, right_exp;
        if (left_exp_is_completed) {
            i = 0;
            left_exp = eval_exp(rule, reg, compare.left_exp, i);
        }
        if (right_exp_is_completed) {
            i = 0;
            right_exp = eval_exp(rule, reg, compare.right_exp, i);
        }
        if (left_exp_is_completed && !right_exp_is_completed &&
            compare.right_exp.size() == 1 && compare.right_exp[0][0] == '#')
        {
            int freelink_id = std::stoi(compare.right_exp[0].substr(1));
            reg.expected_unary[freelink_id] = to_string(left_exp);
        }
        if (!left_exp_is_completed && right_exp_is_completed &&
            compare.left_exp.size() == 1 && compare.left_exp[0][0] == '#')
        {
            int freelink_id = std::stoi(compare.left_exp[0].substr(1));
            reg.expected_unary[freelink_id] = to_string(right_exp);
        }
        if (left_exp_is_completed && right_exp_is_completed &&
            !eval_compare(left_exp, compare.op, right_exp))
        {
            return false;
        }
    }
    return true;
}


void rewrite(Rule &rule, Register &reg) {
    for (Atom* &atom : reg.head_atoms) {
        assert(atom != NULL);
    }
/*
    freelinkがhead_atomsを指しているとき、
    そのfreelinkを新しいほうにつなぎ変えてからもとのhead_atomsを消す必要がある
*/

    vector<bool> in_guard(rule.freelink_num);
    for (auto &compare : rule.guard.compares) {
        for (string &token : compare.left_exp) {
            if (token[0] == '#') {
                int freelink_id = std::stoi(token.substr(1));
                in_guard[freelink_id] = true;
            }
        }
        for (string &token : compare.right_exp) {
            if (token[0] == '#') {
                int freelink_id = std::stoi(token.substr(1));
                in_guard[freelink_id] = true;
            }
        }
    }
    map<pair<int,int>, Atom*> typed_atoms; // <body_atoms_id, freelink_id>


    const int rule_body_size = rule.body_atoms.size();

    // 1. 新しいアトムを生成する
    for (int i = 0; i < rule_body_size; i++) {
        reg.body_atoms[i] = new Atom(rule.body_atoms[i]->functor);
    }

    // 2. 自由リンクの先からこちらを向いてるものを新しいアトムに指す
    for (int i = 0; i < rule_body_size; i++) {
        const int arity = rule.body_atoms[i]->functor.arity;
        for (int j = 0; j < arity; j++) {
            // bodyのi番目のアトムのj番目のリンクについて

            if (rule.body_atoms[i]->link[j].is_freelink()) {
                int fi = rule.body_atoms[i]->link[j].freelinkID();
                if (in_guard[fi]) {
                    typed_atoms[{i, fi}] = new Atom(reg.freelinks[fi].atom->functor);
                    connect_links(typed_atoms[{i, fi}], 0, reg.body_atoms[i], j);
                } else {
                    // int dst_pos = reg.freelinks[fi].pos;
                    // reg.freelinks[fi].atom->link[dst_pos].atom = reg.body_atoms[i];
                    // reg.freelinks[fi].atom->link[dst_pos].pos = j;
                    connect_links(reg.freelinks[fi].atom, reg.freelinks[fi].pos, reg.body_atoms[i], j);
                }
            }
        }
    }

    // 3. bodyatomから自由リンクを指す & 局所リンクを繋ぐ
    for (int i = 0; i < rule_body_size; i++) {
        const int arity = rule.body_atoms[i]->functor.arity;
        for (int j = 0; j < arity; j++) {
            // bodyのi番目のアトムのj番目のリンクについて
            if (rule.body_atoms[i]->link[j].is_freelink()) {
                int fi = rule.body_atoms[i]->link[j].freelinkID();
                if (in_guard[fi]) {
                    connect_links(reg.body_atoms[i], j, typed_atoms[{i, fi}], 0);
                } else {
                    connect_links(reg.body_atoms[i], j, reg.freelinks[fi].atom, reg.freelinks[fi].pos);
                }
                // reg.body_atoms[i]->link[j] = reg.freelinks[fi];
            } else {
                int li = rule.body_atoms[i]->link[j].atom->id;
                // reg.body_atoms[i]->link[j].atom = reg.body_atoms[li];
                // reg.body_atoms[i]->link[j].pos = rule.body_atoms[i]->link[j].pos;
                connect_links(reg.body_atoms[i], j, reg.body_atoms[li], rule.body_atoms[i]->link[j].pos);
            }
        }
    }

    // 4. 自由リンク同士の接続
    for (pair<int,int> &p : rule.connectors) {
        int u = p.first;
        int v = p.second;
        // reg.freelinks[u].atom->link[reg.freelinks[u].pos] = reg.freelinks[v];
        connect_links(reg.freelinks[u].atom, reg.freelinks[u].pos, reg.freelinks[v].atom, reg.freelinks[v].pos);
        // reg.freelinks[v].atom->link[reg.freelinks[v].pos] = reg.freelinks[u];
        connect_links(reg.freelinks[v].atom, reg.freelinks[v].pos, reg.freelinks[u].atom, reg.freelinks[u].pos);
    }

    for (int i = 0; i < rule.freelink_num; i++) {
        if (in_guard[i]) {
            Atom *dst_unary_atom = reg.freelinks[i].atom;
            Atom *h_atom = dst_unary_atom->link[0].atom;
            unary_indexed_atomlist[h_atom->functor][dst_unary_atom->link[0].pos][dst_unary_atom->functor.name].erase(
                unary_indexed_atom_itr[dst_unary_atom]
            );
            delete dst_unary_atom;
        }
    }

    // 5. headatomを消す
    const int rule_head_size = rule.head_atoms.size();
    for (int i = 0; i < rule_head_size; i++) {
        if (reg.head_atoms[i]->functor.arity == 1 && !rule.head_atoms[i]->link[0].is_freelink()) {
            int dst_id = rule.head_atoms[i]->link[0].atom->id;
            int dst_pos = rule.head_atoms[i]->link[0].pos;
            unary_indexed_atomlist[rule.head_atoms[dst_id]->functor][dst_pos][rule.head_atoms[i]->functor.name].erase(
                unary_indexed_atom_itr[reg.head_atoms[i]]
            );
        }
        delete reg.head_atoms[i];
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
    debug(back_track);
    return 0;
}

