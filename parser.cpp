#include "parser.hpp"

int TopSet::orig_local_link_num = 0;


TopSet::Link::Link() {}

TopSet::Link::~Link() {}

TopSet::Link::Link(bool is_connector_, int id_, int pos_):
    is_connector(is_connector_), id(id_), pos(pos_) {}

bool TopSet::Link::operator==(const Link rhs) const {
    return is_connector == rhs.is_connector && id == rhs.id && pos == rhs.pos;
}

ostream& operator<<(ostream& ost, const TopSet::Link &rhs) {
    ost << "<" << (rhs.is_connector ? "( = ) " : "(...) ")
        << rhs.id << "," << rhs.pos << ">";
    return ost;
}


TopSet::TopSet() {}

TopSet::~TopSet() {}

int TopSet::add_atom(string atom) {
    atom_name.push_back(atom);
    int atom_id = atom_args.size();
    atom_args.resize(atom_id + 1);
    return atom_id;
}

string TopSet::make_orig_local_link_num() {
    return "#" + to_string(orig_local_link_num++);
}

void TopSet::show() {
    for (int i = 0; i < (int)atom_name.size(); i++) {
        cout << atom_name[i] << "(";
        int arity = (int)atom_args[i].size();
        for (int j = 0; j < arity; j++) {
            cout << atom_args[i][j] 
                 << (j + 1 == arity ? "" : ", ");
        }
        cout << ")" << endl;
    }
    for (auto &p : connect) {
        cout << p.first << " = " << p.second << endl;
    }
}


PrsGuard::Compare::Compare() {}

PrsGuard::Compare::~Compare() {}

PrsGuard::Compare::Compare(
    vector<string> &left_exp_, string &op_, vector<string> &right_exp_):
    left_exp(left_exp_), op(op_), right_exp(right_exp_) {}

bool PrsGuard::Compare::is_null() {
    return op == "";
}


PrsGuard::TypeCheck::TypeCheck() {}

PrsGuard::TypeCheck::~TypeCheck() {}

PrsGuard::TypeCheck::TypeCheck(string link_, string type_): link(link_), type(type_) {}

bool PrsGuard::TypeCheck::is_null() {
    return type == "";
}


PrsGuard::Assign::Assign() {}

PrsGuard::Assign::~Assign() {}


PrsGuard::PrsGuard() {}

PrsGuard::~PrsGuard() {}

bool PrsGuard::is_null() {
    return type_check.empty() && compare.empty();
}

void PrsGuard::show() {
    for (TypeCheck &tc : type_check) {
        cout << tc.type << ": " << tc.link << endl;
    }

    for (Compare &c : compare) {
        for (string &token : c.left_exp) {
            cout << token << " ";
        }
        cout << c.op << " ";
        for (string &token : c.right_exp) {
            cout << token << " ";
        }
        cout << endl;
    }

    for (Assign &a : assign) {
        cout << a.new_var << " = ";
        for (string &token : a.exp) {
            cout << token << " ";
        }
        cout << endl;
    }
}


PrsRule::PrsRule() {}

PrsRule::~PrsRule() {}

PrsRule::PrsRule(TopSet head_, TopSet body_): head(head_), body(body_) {}

PrsRule::PrsRule(TopSet head_, PrsGuard guard_, TopSet body_):
    head(head_), body(body_), guard(guard_) {}

void PrsRule::show() {
    printf(". [Head] .\n");
    head.show();
    printf(". [Guard] .\n");
    guard.show();
    printf(". [Body] .\n");
    body.show();
}


Parser::Parser(): y(0), x(0) {}

Parser::~Parser() {}

void Parser::read_istream() {
    string s;
    while (getline(cin, s)) {
        raw_inputs.push_back(s);
    }
}

void Parser::show() {
    printf("================ [Graph] ================\n");
    graph.show();

    printf("================ [Rule] =================\n");
    int R = rule.size();
    for (int i = 0; i < R; i++) {
        printf("* ------- [Rule %d] -------- *\n", i);
        rule[i].show();
    }
    printf("=========================================\n");
}


Result_of_nest::Result_of_nest() {}

Result_of_nest::~Result_of_nest() {}

Result_of_nest::Result_of_nest(string link_name_):
    is_link(true), link_name(link_name_) {}

Result_of_nest::Result_of_nest(int atom_id_):
    is_link(false), atom_id(atom_id_) {}


void Parser::syntax_error(string message) {
    cerr << "Syntax Error: " << message
         << " at line " << y+1 << " column " << x+1 << endl;
    int l = max(x-30, 0);
    cerr << raw_inputs[y].substr(l, 70) << endl;
    cerr << string(x-l, ' ') << "^" << endl;
    exit(1);
}


// https://stackoverflow.com/questions/6698039/nested-comments-in-c-c
void Parser::read_ignore() {
    while (y < (int)raw_inputs.size()) {
        if (x == (int)raw_inputs[y].size()) {
            y++, x = 0;
        } else if (raw_inputs[y][x] == ' ' || raw_inputs[y][x] == '\t') {
            x++;
        } else if (raw_inputs[y].substr(x, 2) == "//") {
            y++, x = 0;
        } else if (raw_inputs[y].substr(x, 2) == "/*") {
            int begin_y = y, begin_x = x;
            x += 2;
            while (raw_inputs[y].substr(x, 2) != "*/") {
                if (x == (int)raw_inputs[y].size()) {
                    y++, x = 0;
                } else {
                    x++;
                }
                if (y == (int)raw_inputs.size()) {
                    y = begin_y, x = begin_x;
                    syntax_error("unterminated comment");
                } 
            }
            x += 2;
        } else {
            return;
        }
    }
}


bool Parser::read_token(string token) {
    int length = token.size();
    if (raw_inputs[y].substr(x, length) == token) {
        x += length;
        read_ignore();
        return true;
    } else {
        return false;
    }
}


// Program := {(Rule | Graph) '.'}
// Rule := TopSet ':-' TopSet
// Graph := TopSet
void Parser::read_sentences() {
    read_ignore();
    while (y < (int)raw_inputs.size()) {
        TopSet topset = read_topset();

        // マクロにスコープはない？！ -> lambda式
        #define append(a, b) a.insert(a.end(), b.begin(), b.end())
        // Graph
        if (read_token(".")) {
            append(graph.atom_name, topset.atom_name);
            append(graph.atom_args, topset.atom_args);
            append(graph.connect, topset.connect);
        }
        // Rule
        else if (read_token(":-")) {
            PrsGuard guard;
            int preY = y, preX = x;
            if (read_guard(guard)) {
                rule.push_back(PrsRule(topset, guard, read_topset()));
            } else {
                y = preY, x = preX;
                rule.push_back(PrsRule(topset, read_topset()));
            }
            if (!read_token(".")) {
                syntax_error("Unexpected Token");
            }
        } else {
            syntax_error("Unexpected Token");
        }
    }
}


/*
    Link = atom, atom = Link ならatom側へ付け足し
    Link = Link ならそのまま pair<string, string> と等価なもので登録
    atom = atom なら局所リンクを発行し,それぞれ付け足し
*/
// TopSet := (Nest | Nest '=' Nest) {',' (Nest | Nest '=' Nest)}
TopSet Parser::read_topset() {
    TopSet topset;
    do {
        int tokenX = x, tokenY = y;
        Result_of_nest result1 = read_nest(topset);
        if (read_token("=")) {
            Result_of_nest result2 = read_nest(topset);
            if (result1.is_link) {
                // Link = Link
                if (result2.is_link) {
                    topset.connect.push_back({result1.link_name, result2.link_name});
                }
                // Link = Atom(...)
                else {
                    int atom_id = result2.atom_id;
                    topset.atom_args[atom_id].push_back(result1.link_name);
                }
            } else {
                // Atom(...) = Link
                if (result2.is_link) {
                    int atom_id = result1.atom_id;
                    topset.atom_args[atom_id].push_back(result2.link_name);
                }
                // Atom(...) = Atom(...)
                else {
                    int atom_id1 = result1.atom_id;
                    int atom_id2 = result2.atom_id;
                    string orig_local_link = TopSet::make_orig_local_link_num();
                    topset.atom_args[atom_id1].push_back(orig_local_link);
                    topset.atom_args[atom_id2].push_back(orig_local_link);
                }
            }
        }
        // Nest
        else {
            if (result1.is_link) {
                y = tokenY, x = tokenX;
                syntax_error(
                    "Top-level variable occurrence: " + result1.link_name
                );
            }
        }
    } while (read_token(","));
    return topset;
}


// Nest := Link | Atom | Atom '(' ')' | Atom '(' Nest {',' Nest} ')'
// 親子のNestなら子は親へidをかえし局所リンクを発行し、子のatomへ付け足し、親はその新規リンクを登録
Result_of_nest Parser::read_nest(TopSet &topset) {

    // Link
    if (is_link_initial(raw_inputs[y][x])) {
        string cur_link = read_name();
        read_ignore();
        return Result_of_nest(cur_link);
    }

    // Atom | Atom '(' ')' | Atom '(' Nest {',' Nest} ')'
    else if (is_atom_initial(raw_inputs[y][x]) || isdigit(raw_inputs[y][x])) {
        int atom_id = topset.add_atom(read_name());

        // Atom '(' ')' | Atom '(' Nest {',' Nest} ')'
        if (read_token("(")) {

            // Atom '(' ')'
            if (read_token(")")) {
                return Result_of_nest(atom_id);
            }

            // Atom '(' Nest {',' Nest} ')'
            else {
                do {
                    Result_of_nest result = read_nest(topset);
                    if (result.is_link) {
                        topset.atom_args[atom_id].push_back(result.link_name);
                    } else {
                        int child_id = result.atom_id;
                        string orig_local_link = TopSet::make_orig_local_link_num();
                        topset.atom_args[atom_id].push_back(orig_local_link);
                        topset.atom_args[child_id].push_back(orig_local_link);
                    }
                } while (read_token(","));

                if (read_token(")")) {
                    return Result_of_nest(atom_id);
                } else {
                    syntax_error("Unexpected Token");
                }
            }
        }

        // Atom
        else {
            return Result_of_nest(atom_id);
        }

    } else {
        syntax_error("Unexpected Token");
    }
    assert(false);
}

// ------------------------- Guard -------------------------

// Gurad := (TypeCheck | Compare) {',' (TypeCheck | Compare)}
// TypeCheck := Type '(' Link ')'
// Type := int | unary
// Compare := Exp COP Exp
// COP := '=:=' | '=\=' | '<' | '=<' | '>' | '>='
// AOP := '+' | '-' | '*' | '/' | 'mod'
// Exp := Factor { AOP Factor }
// Factor := Link | Num | '(' Exp ')' | ('+'|'-') Factor | 'rand' '(' Num ')'
// Num := [0-9]+

bool Parser::read_factor(vector<string> &tokens) {
    if (is_link_initial(raw_inputs[y][x])) {
        tokens.push_back(read_name());
        return true;
    } else if (isdigit(raw_inputs[y][x])) {
        tokens.push_back(read_number());
        return true;
    } else if (read_token("(")) {
        tokens.push_back("(");
        if (!read_exp(tokens)) return false;
        if (!read_token(")")) return false;
        tokens.push_back(")");
        return true;
    } else if (read_token("+")) {
        tokens.push_back("+");
        return read_factor(tokens);
    } else if (read_token("-")) {
        tokens.push_back("-");
        return read_factor(tokens);
    } else if (read_token("rand")) {
        tokens.push_back("rand");
        if (!read_token("(")) return false;
        tokens.push_back("(");
        if (!isdigit(raw_inputs[y][x])) return false;
        tokens.push_back(read_number());
        if (!read_token(")")) return false;
        tokens.push_back(")");
        return true;
    } else {
        return false;
    }
}

bool Parser::read_exp(vector<string> &tokens) {
    while (true) {
        if (!read_factor(tokens)) {
            return false;
        }
        bool finish = true;
        vector<string> arith_ops = { "+", "-", "*", "/", "mod" };
        for (string &op : arith_ops) {
            if (read_token(op)) {
                tokens.push_back(op);
                finish = false;
                break;
            }
        }
        if (finish) break;
    }
    return true;
}


bool Parser::read_type_check(PrsGuard::TypeCheck &type_check) {
    vector<string> types = { "int", "unary" };
    bool ok = false;
    for (string &type : types) {
        if (read_token(type)) {
            type_check.type = type;
            ok = true;
            break;
        }
    }
    if (!ok) return false;
    if (!read_token("(")) return false;
    if (is_link_initial(raw_inputs[y][x])) {
        type_check.link = read_name();
    } else {
        return false;
    }
    if (!read_token(")")) return false;
    return true;
}


bool Parser::read_assign(PrsGuard::Assign &assign) {
    if (!is_link_initial(raw_inputs[y][x])) return false;
    assign.new_var = read_name();
    if (!read_token("=")) return false;
    if (!read_exp(assign.exp)) return false;
    return true;
}


bool Parser::read_compare(PrsGuard::Compare &compare) {
    if (!read_exp(compare.left_exp)) return false;
    vector<string> compare_ops = { "=:=", "=\\=", "=<", "<", ">=", ">"};
    bool ok = false;
    for (string &op : compare_ops) {
        if (read_token(op)) {
            ok = true;
            compare.op = op;
            break;
        }
    }
    if (!ok) return false;
    if (!read_exp(compare.right_exp)) return false;
    return true;
}

bool Parser::read_guard(PrsGuard &guard) {
    do {
        PrsGuard::TypeCheck type_check;
        int preY = y, preX = x;
        if (read_type_check(type_check)) {
            guard.type_check.push_back(type_check);
            continue;
        }
        y = preY, x = preX;
        PrsGuard::Compare compare;
        if (read_compare(compare)) {
            guard.compare.push_back(compare);
            continue;
        }
        y = preY, x = preX;
        PrsGuard::Assign assign;
        if (read_assign(assign)) {
            guard.assign.push_back(assign);
            continue;
        }
        return false;
    } while (read_token(","));

    if (read_token("|")) {
        return true;
    } else {
        return false;
    }
}


//  Link := [A-Z_][a-zA-Z_0-9]*
bool is_link_initial(char c) {
    return isupper(c) || c == '_';
}


//  Atom := [a-z][a-zA-Z_0-9]* | ''' .* '''
bool is_atom_initial(char c) {
    return islower(c) || c == '\'';
}


//  Atom := [a-z][a-zA-Z_0-9]* | ''' [^']+ ''' | [0-9]+
//  Link := [A-Z_][a-zA-Z_0-9]*
string Parser::read_name() {
    string name;
    if (raw_inputs[y][x] == '\'') {
        int tokenY = y, tokenX = x;
        x++;
        name += '\'';
        while (raw_inputs[y][x] != '\'') {
            name += raw_inputs[y][x];
            x++;
            if (x == (int)raw_inputs[y].size()) {
                y = tokenY, x = tokenX;
                syntax_error("Illegal Character");
            }
        }
        x++;
        name += '\'';
    } else {
        while (x < (int)raw_inputs[y].size()) {
            char c = raw_inputs[y][x];
            if (isalpha(c) || isdigit(c) || c == '_') {
                name += c;
                x++;
            } else {
                break;
            }
        }
    }
    read_ignore();
    return name;
}


// Num := [0-9]+
string Parser::read_number() {
    string name;
    while (x < (int)raw_inputs[y].size()) {
        char c = raw_inputs[y][x];
        if (isdigit(c)) {
            name += c;
            x++;
        } else {
            break;
        }
    }
    read_ignore();
    return name;
}

// ---------------------------------------------------------------------------------

map<string, int> TopSet::get_link_count() {
    map<string, int> link_count;
    for (auto &args : atom_args) {
        for (auto &arg : args) {
            link_count[arg]++;
        }
    }
    for (auto &connector : connect) {        
        link_count[connector.first]++;
        link_count[connector.second]++;
    }
    return link_count;
}


void TopSet::check_link_num_of_graph() {
    map<string, int> link_count = get_link_count();
    for (auto &itr : link_count) {
        const string &link_name = itr.first;
        if (link_count[link_name] > 2) {
            cerr << "Semantic Error: link " << link_name
                 << " appears more than twice" << endl;
            exit(1);
        }
        if (link_count[link_name] == 1) {
            cerr << "Semantic Error: link " << link_name
                 << " is global singleton" << endl;
            exit(1);
        }
    }
}


void PrsRule::check_link_num_of_rule() {

    map<string, int> head_link_count = head.get_link_count();
    map<string, int> body_link_count = body.get_link_count();

    for (auto &itr : head_link_count) {
        const string &link_name = itr.first;
        if (head_link_count[link_name] > 2) {
            cerr << "Semantic Error: link " << link_name
                 << " in head appears more than twice" << endl;
            show();
            exit(1);
        }
        if (head_link_count[link_name] == 1 && body_link_count[link_name] != 1) {
            cerr << "Semantic Error: link " << link_name
                 << " in head is free variable" << endl;
            show();
            exit(1);
        }
    }


    for (auto &itr : body_link_count) {
        const string &link_name = itr.first;
        if (body_link_count[link_name] > 2) {
            cerr << "Semantic Error: link " << link_name
                 << " in body appears more than twice" << endl;
            show();
            exit(1);
        }
        if (body_link_count[link_name] == 1 && head_link_count[link_name] != 1) {
            cerr << "Semantic Error: link " << link_name
                 << " in body is free variable" << endl;
            show();
            exit(1);
        }
    }
}


void TopSet::build_dst_map() {
    for (int atom_id = 0; atom_id < (int)atom_name.size(); atom_id++) {
        int arity = atom_args[atom_id].size();
        // a(.., X, ..), b(.., X, ..)
        for (int link_id = 0; link_id < arity; link_id++) {
            string str_pair = to_string(atom_id) + ":" + to_string(link_id);
            string &link_name = atom_args[atom_id][link_id];
            if (dst.count(link_name)) {
                dst[str_pair] = dst[link_name];
                dst[dst[link_name]] = str_pair;
                dst.erase(link_name);
            } else {
                dst[str_pair] = link_name;
                dst[link_name] = str_pair;
            }
        }
    }

    // a(.., X, ..), X = Y, b(.., Y, ..)
    for (int connect_id = 0; connect_id < (int)connect.size(); connect_id++) {
        string &link_name1 = connect[connect_id].first;
        string &link_name2 = connect[connect_id].second;
        string dst_name1 = link_name1, dst_name2 = link_name2;
        if (dst.count(link_name1)) {
            dst_name1 = dst[link_name1];
            dst.erase(link_name1);
        }
        if (dst.count(link_name2)) {
            dst_name2 = dst[link_name2];
            dst.erase(link_name2);
        }
        if (link_name1 == dst_name2 && link_name2 == dst_name1) continue;
        dst[dst_name1] = dst_name2;
        dst[dst_name2] = dst_name1;
    }
}

pair<int, int> str_to_pair(string str) {
    // id:pos
    int pos = str.find(":");
    return {stoi(str.substr(0,pos)), stoi(str.substr(pos+1))};
}


void set_graph(TopSet &graph) {

    int atom_num = graph.atom_name.size();
    vector<Atom*> atoms(atom_num);
    for (int i = 0; i < atom_num; i++) {
        Functor functor(graph.atom_args[i].size(), graph.atom_name[i]);
        atoms[i] = new Atom(functor);
    }

    for (int i = 0; i < atom_num; i++) {
        Functor functor = atoms[i]->functor;
        for (int j = 0; j < functor.arity; j++) {
            string str_pair = to_string(i) + ":" + to_string(j);
            pair<int,int> p = str_to_pair(graph.dst[str_pair]);
            atoms[i]->link[j] = Link(atoms[p.first], p.second);
        }
    }
}


void set_rule(PrsRule &parser_rule) {

    Rule rule;
    int head_atom_num = parser_rule.head.atom_name.size();
    int body_atom_num = parser_rule.body.atom_name.size();
    vector<RuleAtom*> head_atoms(head_atom_num);
    vector<RuleAtom*> body_atoms(body_atom_num);

    for (int i = 0; i < head_atom_num; i++) {
        Functor functor(parser_rule.head.atom_args[i].size(), parser_rule.head.atom_name[i]);
        head_atoms[i] = new RuleAtom(functor, i);
    }
    for (int i = 0; i < body_atom_num; i++) {
        Functor functor(parser_rule.body.atom_args[i].size(), parser_rule.body.atom_name[i]);
        body_atoms[i] = new RuleAtom(functor, i);
    }

    map<string, int> free_link_id;
    for (int i = 0; i < head_atom_num; i++) {
        for (int j = 0; j < head_atoms[i]->functor.arity; j++) {
            string str_pair = to_string(i) + ":" + to_string(j);
            string &dst_name = parser_rule.head.dst[str_pair];

            // 自由リンク
            if (is_link_initial(dst_name[0])) {
                free_link_id[dst_name] = rule.freelink_num;
                head_atoms[i]->link[j] = RuleLink(rule.freelink_num++);
            }
            // 局所リンク
            else {
                pair<int,int> p = str_to_pair(parser_rule.head.dst[str_pair]);
                head_atoms[i]->link[j] = RuleLink(head_atoms[p.first], p.second);
            }
        }
    }

    // Guardの処理




    for (int i = 0; i < body_atom_num; i++) {
        Functor functor = body_atoms[i]->functor;
        for (int j = 0; j < functor.arity; j++) {
            string str_pair = to_string(i) + ":" + to_string(j);
            string &dst_name = parser_rule.body.dst[str_pair];

            // Guard内のリンクも特にここでの処理はいらない？？
            // 自由リンク
            if (is_link_initial(dst_name[0])) {
                body_atoms[i]->link[j] = RuleLink(free_link_id[dst_name]);
            }
            // 局所リンク
            else {
                pair<int,int> p = str_to_pair(parser_rule.body.dst[str_pair]);
                body_atoms[i]->link[j] = RuleLink(body_atoms[p.first], p.second);
            }
        }
    }

    // connector
    map<string, bool> registered_connector;
    for (auto &dst_itr : parser_rule.body.dst) {
        const string &link_name1 = dst_itr.first;
        const string &link_name2 = dst_itr.second;
        if (isdigit(link_name1[0])) continue;
        if (isdigit(link_name2[0])) continue;
        if (registered_connector[link_name1]) continue;
        if (registered_connector[link_name2]) continue;
        rule.connector.push_back({free_link_id[link_name1], free_link_id[link_name2]});
        registered_connector[link_name1] = true;
        registered_connector[link_name2] = true;
    }

    for (auto &tc : parser_rule.guard.type_check) {
        if (free_link_id.find(tc.link) != free_link_id.end()) {
            tc.link = "#" + to_string(free_link_id[tc.link]);
        }
    }
    for (auto &c : parser_rule.guard.compare) {
        for (string &token : c.left_exp) {
            if (free_link_id.find(token) != free_link_id.end()) {
                token = "#" + to_string(free_link_id[token]);
            }
        }
        for (string &token : c.right_exp) {
            if (free_link_id.find(token) != free_link_id.end()) {
                token = "#" + to_string(free_link_id[token]);
            }
        }
    }

    // rule.guard = parser_rule.guard;
    for (auto &type_check : parser_rule.guard.type_check) {
        rule.guard.type_check.emplace_back(type_check.link, type_check.type);
    }
    for (auto &compare : parser_rule.guard.compare) {
        rule.guard.compare.emplace_back(compare.left_exp, compare.op, compare.right_exp);
    }
    rule.head = head_atoms;
    rule.body = body_atoms;
    rulelist.push_back(rule);
}


void Parser::parse() {
    read_istream();
    read_sentences();
    // show();

    graph.build_dst_map();
    graph.check_link_num_of_graph();
    
    for (auto &prsrule : rule) {
        prsrule.head.build_dst_map();
        prsrule.body.build_dst_map();
        prsrule.check_link_num_of_rule();
    }
}


void load(Parser &parser) {
    set_graph(parser.graph);
    for (auto &prsrule : parser.rule) {
        set_rule(prsrule);
    }
}

