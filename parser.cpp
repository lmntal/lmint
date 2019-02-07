#include "parser.hpp"

TopSet::TopSet() {}
TopSet::~TopSet() {}

int TopSet::add_atom(string atom_name) {
    atom_names.push_back(atom_name);
    int atom_id = atoms_args.size();
    atoms_args.resize(atom_id + 1);
    return atom_id;
}

string TopSet::get_uniq_local_link_name() {
    static int uniq_local_link_num = 0;
    return "#" + to_string(uniq_local_link_num++);
}

void TopSet::show() {
    for (int i = 0; i < (int)atom_names.size(); i++) {
        cout << atom_names[i] << "(";
        int arity = (int)atoms_args[i].size();
        for (int j = 0; j < arity; j++) {
            cout << atoms_args[i][j] 
                 << (j + 1 == arity ? "" : ", ");
        }
        cout << ")" << endl;
    }
    for (auto &p : connects) {
        cout << p.first << " = " << p.second << endl;
    }
}


PrsGuard::Compare::Compare() {}
PrsGuard::Compare::~Compare() {}

PrsGuard::TypeCheck::TypeCheck() {}
PrsGuard::TypeCheck::~TypeCheck() {}

PrsGuard::Assign::Assign() {}
PrsGuard::Assign::~Assign() {}

PrsGuard::PrsGuard() {}
PrsGuard::~PrsGuard() {}


void PrsGuard::show() {
    for (TypeCheck &type_check : type_checks) {
        cout << type_check.type << ": " << type_check.link_name << endl;
    }

    for (Compare &compare : compares) {
        for (string &token : compare.left_exp) {
            cout << token << " ";
        }
        cout << compare.op << " ";
        for (string &token : compare.right_exp) {
            cout << token << " ";
        }
        cout << endl;
    }

    for (Assign &assign : assigns) {
        cout << assign.new_var << " = ";
        for (string &token : assign.exp) {
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
    int R = rules.size();
    for (int i = 0; i < R; i++) {
        printf("* ------- [Rule %d] -------- *\n", i);
        rules[i].show();
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
            append(graph.atom_names, topset.atom_names);
            append(graph.atoms_args, topset.atoms_args);
            append(graph.connects, topset.connects);
        }
        // Rule
        else if (read_token(":-")) {
            PrsGuard guard;
            int preY = y, preX = x;
            if (read_guard(guard)) {
                rules.push_back(PrsRule(topset, guard, read_topset()));
            } else {
                y = preY, x = preX;
                rules.push_back(PrsRule(topset, read_topset()));
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
                    topset.connects.push_back({result1.link_name, result2.link_name});
                }
                // Link = Atom(...)
                else {
                    int atom_id = result2.atom_id;
                    topset.atoms_args[atom_id].push_back(result1.link_name);
                }
            } else {
                // Atom(...) = Link
                if (result2.is_link) {
                    int atom_id = result1.atom_id;
                    topset.atoms_args[atom_id].push_back(result2.link_name);
                }
                // Atom(...) = Atom(...)
                else {
                    int atom_id1 = result1.atom_id;
                    int atom_id2 = result2.atom_id;
                    string orig_local_link = TopSet::get_uniq_local_link_name();
                    topset.atoms_args[atom_id1].push_back(orig_local_link);
                    topset.atoms_args[atom_id2].push_back(orig_local_link);
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
                        topset.atoms_args[atom_id].push_back(result.link_name);
                    } else {
                        int child_id = result.atom_id;
                        string orig_local_link = TopSet::get_uniq_local_link_name();
                        topset.atoms_args[atom_id].push_back(orig_local_link);
                        topset.atoms_args[child_id].push_back(orig_local_link);
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
        type_check.link_name = read_name();
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
            guard.type_checks.push_back(type_check);
            continue;
        }
        y = preY, x = preX;
        PrsGuard::Compare compare;
        if (read_compare(compare)) {
            guard.compares.push_back(compare);
            continue;
        }
        y = preY, x = preX;
        PrsGuard::Assign assign;
        if (read_assign(assign)) {
            guard.assigns.push_back(assign);
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

void TopSet::build_link_count() {
    for (auto &args : atoms_args) {
        for (auto &arg : args) {
            link_count[arg]++;
        }
    }
    for (auto &connect : connects) {        
        link_count[connect.first]++;
        link_count[connect.second]++;
    }
}


void TopSet::validate_links() {
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


void PrsRule::validate_links() {
    map<string, bool> has_type;
    for (auto &type_check : guard.type_checks) {
        has_type[type_check.link_name] = true;
    }
    for (auto &assign : guard.assigns) {
        has_type[assign.new_var] = true;
        for (auto token : assign.exp) {
            if (is_link_initial(token[0])) {
                has_type[token] = true;
            }
        }
    }
    for (auto &compare : guard.compares) {
        for (auto token : compare.left_exp) {
            if (is_link_initial(token[0])) {
                has_type[token] = true;
            }
        }
        for (auto token : compare.right_exp) {
            if (is_link_initial(token[0])) {
                has_type[token] = true;
            }
        }
    }

    for (auto &itr : head.link_count) {
        const string &link_name = itr.first;
        if (head.link_count[link_name] >= 3) {
            cerr << "Semantic Error: link " << link_name
                 << " in the head appears more than twice" << endl;
            show();
            exit(1);
        }
        if (head.link_count[link_name] == 2) {
            if (has_type[link_name]) {
                cerr << "Semantic Error: link " << link_name
                     << " in the head is local link, but it's in the guard" << endl;
                show();
                exit(1);
            }
        }
        if (head.link_count[link_name] == 1){
            if (has_type[link_name]) continue;
            if (body.link_count[link_name] != 1) {
                cerr << "Semantic Error: link " << link_name
                     << " in the head is free variable" << endl;
                show();
                exit(1);
            }
        }
    }

    for (auto &itr : body.link_count) {
        const string &link_name = itr.first;
        if (has_type[link_name]) continue;
        if (body.link_count[link_name] >= 3) {
            cerr << "Semantic Error: link " << link_name
                 << " in the body appears more than twice" << endl;
            show();
            exit(1);
        }
        if (body.link_count[link_name] == 1 && head.link_count[link_name] != 1) {
            cerr << "Semantic Error: link " << link_name
                 << " in the body is free variable" << endl;
            show();
            exit(1);
        }
    }
}


void TopSet::build_dst_map() {
    for (int atom_id = 0; atom_id < (int)atom_names.size(); atom_id++) {
        int arity = atoms_args[atom_id].size();
        // a(.., X, ..), b(.., X, ..)
        for (int link_id = 0; link_id < arity; link_id++) {
            string str_pair = to_string(atom_id) + ":" + to_string(link_id);
            string &link_name = atoms_args[atom_id][link_id];
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
    for (int connect_id = 0; connect_id < (int)connects.size(); connect_id++) {
        string &link_name1 = connects[connect_id].first;
        string &link_name2 = connects[connect_id].second;
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


void PrsRule::build_var_id() {
    for (auto &itr : head.link_count) {
        const string &link_name = itr.first;
        if (head.link_count[link_name] == 1) {
            int id = var_id.size();
            var_id[link_name] = id;
        }
    }
    for (auto &assign : guard.assigns) {
        int id = var_id.size();
        var_id[assign.new_var] = id;
    }
}

void set_graph(TopSet &graph) {

    int atom_num = graph.atom_names.size();
    vector<Atom*> atoms(atom_num);
    for (int i = 0; i < atom_num; i++) {
        Functor functor(graph.atoms_args[i].size(), graph.atom_names[i]);
        atoms[i] = new Atom(functor);
    }

    for (int i = 0; i < atom_num; i++) {
        Functor functor = atoms[i]->functor;
        for (int j = 0; j < functor.arity; j++) {
            string str_pair = to_string(i) + ":" + to_string(j);
            pair<int,int> p = str_to_pair(graph.dst[str_pair]);
            connect_links(atoms[i], j, atoms[p.first], p.second);
        }
    }
}


void set_rule(PrsRule &parser_rule) {

    Rule rule;
    int head_atom_num = parser_rule.head.atom_names.size();
    int body_atom_num = parser_rule.body.atom_names.size();
    vector<RuleAtom*> head_atoms(head_atom_num);
    vector<RuleAtom*> body_atoms(body_atom_num);

    for (int i = 0; i < head_atom_num; i++) {
        Functor functor(parser_rule.head.atoms_args[i].size(), parser_rule.head.atom_names[i]);
        head_atoms[i] = new RuleAtom(functor, i);
    }
    for (int i = 0; i < body_atom_num; i++) {
        Functor functor(parser_rule.body.atoms_args[i].size(), parser_rule.body.atom_names[i]);
        body_atoms[i] = new RuleAtom(functor, i);
    }

    for (int i = 0; i < head_atom_num; i++) {
        for (int j = 0; j < head_atoms[i]->functor.arity; j++) {
            string str_pair = to_string(i) + ":" + to_string(j);
            string &dst_name = parser_rule.head.dst[str_pair];

            // 自由リンク
            if (is_link_initial(dst_name[0])) {
                head_atoms[i]->link[j] = RuleLink(parser_rule.var_id[dst_name]);
            }
            // 局所リンク
            else {
                pair<int,int> p = str_to_pair(parser_rule.head.dst[str_pair]);
                head_atoms[i]->link[j] = RuleLink(head_atoms[p.first], p.second);
            }
        }
    }

    // in_guardを調べる
    map<string, bool> in_guard;
    for (auto &compare : parser_rule.guard.compares) {
        for (string &token : compare.left_exp) {
            if (parser_rule.var_id.count(token)) {
                in_guard[token] = true;
            }
        }
        for (string &token : compare.right_exp) {
            if (parser_rule.var_id.count(token)) {
                in_guard[token] = true;
            }
        }
    }

    for (int i = 0; i < body_atom_num; i++) {
        Functor functor = body_atoms[i]->functor;
        for (int j = 0; j < functor.arity; j++) {
            string str_pair = to_string(i) + ":" + to_string(j);
            string &dst_name = parser_rule.body.dst[str_pair];

            // Guard内のリンクも特にここでの処理はいらない？？
            // 自由リンク
            if (is_link_initial(dst_name[0])) {
                body_atoms[i]->link[j] = RuleLink(parser_rule.var_id[dst_name]);
            }
            // Guardにある型付きのリンク // 直したい
            else if (in_guard[parser_rule.body.atoms_args[i][j]]) {
                body_atoms[i]->link[j] = RuleLink(parser_rule.var_id[parser_rule.body.atoms_args[i][j]]);
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
        rule.connectors.push_back({parser_rule.var_id[link_name1], parser_rule.var_id[link_name2]});
        registered_connector[link_name1] = true;
        registered_connector[link_name2] = true;
    }

    // 以下、いずれ修正

    for (auto &type_check : parser_rule.guard.type_checks) {
        type_check.link_name = "#" + to_string(parser_rule.var_id[type_check.link_name]);
    }
    for (auto &compare : parser_rule.guard.compares) {
        for (string &token : compare.left_exp) {
            if (parser_rule.var_id.count(token)) {
                token = "#" + to_string(parser_rule.var_id[token]);
            }
        }
        for (string &token : compare.right_exp) {
            if (parser_rule.var_id.count(token)) {
                token = "#" + to_string(parser_rule.var_id[token]);
            }
        }
    }

    // rule.guard = parser_rule.guard;
    for (auto &type_check : parser_rule.guard.type_checks) {
        rule.guard.type_checks.emplace_back(type_check.link_name, type_check.type);
    }
    for (auto &compare : parser_rule.guard.compares) {
        rule.guard.compares.emplace_back(compare.left_exp, compare.op, compare.right_exp);
    }
    rule.head_atoms = head_atoms;
    rule.body_atoms = body_atoms;
    rule.freelink_num = parser_rule.var_id.size();
    rulelist.push_back(rule);
}


void Parser::parse() {
    read_istream();
    read_sentences();
    // show();

    graph.build_dst_map();
    graph.build_link_count();
    graph.validate_links();
    
    for (auto &prsrule : rules) {
        prsrule.head.build_dst_map();
        prsrule.body.build_dst_map();

        prsrule.head.build_link_count();
        prsrule.body.build_link_count();

        prsrule.build_var_id();

        prsrule.validate_links();
    }
}


void load(Parser &parser) {
    set_graph(parser.graph);
    for (auto &prsrule : parser.rules) {
        set_rule(prsrule);
    }
}

