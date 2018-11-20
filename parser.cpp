#include "parser.hpp"

int TopSet::orig_local_link_num = 0;

vector<string> read_istream() {
    vector<string> lines;
    string s;
    while (getline(cin, s)) {
        lines.push_back(s);
    }
    return lines;
}


void syntax_error(vector<string> &raw_inputs, int &y, int &x, string message) {
    cerr << "Syntax Error: " << message << " at line " << y+1 << " column " << x+1 << endl;
    int l = max(x-30, 0);
    cerr << raw_inputs[y].substr(l, 70) << endl;
    cerr << string(x-l, ' ') << "^" << endl;
    exit(1);
}


// https://stackoverflow.com/questions/6698039/nested-comments-in-c-c
void read_ignore(vector<string> &raw_inputs, int &y, int &x) {
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
                    syntax_error(raw_inputs, begin_y, begin_x, "unterminated comment");
                } 
            }
            x += 2;
        } else {
            return;
        }
    }
}


bool read_token(vector<string> &raw_inputs, int &y, int &x, string token) {
    int length = token.size();
    if (raw_inputs[y].substr(x, length) == token) {
        x += length;
        read_ignore(raw_inputs, y, x);
        return true;
    } else {
        return false;
    }
}


// Program := {(Rule | Graph) '.'}
// Rule := TopSet ':-' TopSet
// Graph := TopSet
Parser read_sentences(vector<string> &raw_inputs, int &y, int &x) {
    Parser parser;
    read_ignore(raw_inputs, y, x);
    while (y < (int)raw_inputs.size()) {
        TopSet topset = read_topset(raw_inputs, y, x);

        #define append(a, b) a.insert(a.end(), b.begin(), b.end())
        // Graph
        if (read_token(raw_inputs, y, x, ".")) {
            append(parser.graph.atom_name, topset.atom_name);
            append(parser.graph.atom_args, topset.atom_args);
            append(parser.graph.connect, topset.connect);
        }
        // Rule
        else if (read_token(raw_inputs, y, x, ":-")) {
            parser.rule_head.push_back(topset);
            parser.rule_body.push_back(read_topset(raw_inputs, y, x));
            if (!read_token(raw_inputs, y, x, ".")) {
                syntax_error(raw_inputs, y, x, "Unexpected Token");
            }
        } else {
            syntax_error(raw_inputs, y, x, "Unexpected Token");
        }
    }
    return parser;
}


/*
    Link = atom, atom = Link ならatom側へ付け足し
    Link = Link ならそのまま pair<string, string> と等価なもので登録
    atom = atom なら局所リンクを発行し,それぞれ付け足し
*/
// TopSet := (Nest | Nest '=' Nest) {',' (Nest | Nest '=' Nest)}
TopSet read_topset(vector<string> &raw_inputs, int &y, int &x) {
    TopSet topset;
    do {
        int tokenX = x, tokenY = y;
        Result_of_nest result1 = read_nest(raw_inputs, y, x, topset);
        if (read_token(raw_inputs, y, x, "=")) {
            Result_of_nest result2 = read_nest(raw_inputs, y, x, topset);
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
                syntax_error(raw_inputs, tokenX, tokenY,
                    "Top-level variable occurrence: " + result1.link_name
                );
            }
        }
    } while (read_token(raw_inputs, y, x, ","));
    return topset;
}


// Nest := Link | Atom | Atom '(' ')' | Atom '(' Nest {',' Nest} ')'
// 親子のNestなら子は親へidをかえし局所リンクを発行し、子のatomへ付け足し、親はその新規リンクを登録
Result_of_nest read_nest(vector<string> &raw_inputs, int &y, int &x, TopSet &topset) {

    // Link
    if (isupper(raw_inputs[y][x])){
        string cur_link = read_name(raw_inputs, y, x);
        read_ignore(raw_inputs, y, x);
        return Result_of_nest(cur_link);
    }

    // Atom | Atom '(' ')' | Atom '(' Nest {',' Nest} ')'
    else if (is_atom_initial(raw_inputs[y][x])) {
        int atom_id = topset.add_atom(read_name(raw_inputs, y, x));

        // Atom '(' ')' | Atom '(' Nest {',' Nest} ')'
        if (read_token(raw_inputs, y, x, "(")) {

            // Atom '(' ')'
            if (read_token(raw_inputs, y, x, ")")) {
                return Result_of_nest(atom_id);
            }

            // Atom '(' Nest {',' Nest} ')'
            else {
                do {
                    Result_of_nest result = read_nest(raw_inputs, y, x, topset);
                    if (result.is_link) {
                        topset.atom_args[atom_id].push_back(result.link_name);
                    } else {
                        int child_id = result.atom_id;
                        string orig_local_link = TopSet::make_orig_local_link_num();
                        topset.atom_args[atom_id].push_back(orig_local_link);
                        topset.atom_args[child_id].push_back(orig_local_link);
                    }
                } while (read_token(raw_inputs, y, x, ","));

                if (read_token(raw_inputs, y, x, ")")) {
                    return Result_of_nest(atom_id);
                } else {
                    syntax_error(raw_inputs, y, x, "Unexpected Token");
                }
            }
        }

        // Atom
        else {
            return Result_of_nest(atom_id);
        }

    } else {
        syntax_error(raw_inputs, y, x, "Unexpected Token");
    }
    assert(false);
}


//  Link := [A-Z_][a-zA-Z_0-9]*
bool is_link_initial(char c) {
    return isupper(c) || c == '_';
}


//  Atom := [a-z][a-zA-Z_0-9]*
bool is_atom_initial(char c) {
    return islower(c) || c == '\'';
}


//  Atom := [a-z][a-zA-Z_0-9]*
//  Link := [A-Z_][a-zA-Z_0-9]*
string read_name(vector<string> &raw_inputs, int &y, int &x) {
    string name;
    if (raw_inputs[y][x] == '\'') {
        int tokenY = y, tokenX = x;
        x++;
        name += '\'';
        while (raw_inputs[y][x] != '\'') {
            name += raw_inputs[y][x];
            x++;
            if (x == (int)raw_inputs[y].size()) {
                syntax_error(raw_inputs, tokenY, tokenX, "Illegal Character");
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
    read_ignore(raw_inputs, y, x);
    return name;
}

// ---------------------------------------------------------------------------------


void build_link_port(TopSet &topset) {
    for (int atom_id = 0; atom_id < (int)topset.atom_name.size(); atom_id++) {
        int arity = topset.atom_args[atom_id].size();
        for (int link_id = 0; link_id < arity; link_id++) {
            topset.link_port[topset.atom_args[atom_id][link_id]].push_back(TopSet::Link(false, atom_id, link_id));
        }
    }
    for (int connect_id = 0; connect_id < (int)topset.connect.size(); connect_id++) {        
        topset.link_port[topset.connect[connect_id].first ].push_back(TopSet::Link(true, connect_id, 1));
        topset.link_port[topset.connect[connect_id].second].push_back(TopSet::Link(true, connect_id, 2));
    }
}


void check_link_num_of_graph(TopSet &topset) {
    for (auto &itr : topset.link_port) {
        string link_name = itr.first;
        vector<TopSet::Link> &link_port = itr.second;
        if ((int)link_port.size() > 2) {
            cerr << "Semantic Error: link " << link_name
                 << " appears more than twice" << endl;
            exit(1);
        }
        if ((int)link_port.size() == 1) {
            cerr << "Semantic Error: link " << link_name
                 << " is global singleton" << endl;
            exit(1);
        }
    }
}


void check_link_num_of_rule(int rule_id, TopSet &head, TopSet &body) {
    for (auto &itr : head.link_port) {
        string link_name = itr.first;
        vector<TopSet::Link> &link_port = itr.second;
        if ((int)link_port.size() > 2) {
            cerr << "Semantic Error: link " << link_name
                 << " in head of rule " << rule_id
                 << " appears more than twice" << endl;
            exit(1);
        }
        if ((int)link_port.size() == 1 && (int)body.link_port[link_name].size() != 1) {
            cerr << "Semantic Error: link " << link_name
                 << " in head of rule " << rule_id
                 << " is free variable" << endl;
            exit(1);
        }
    }

    for (auto &itr : body.link_port) {
        string link_name = itr.first;
        vector<TopSet::Link> &link_port = itr.second;
        if ((int)link_port.size() > 2) {
            cerr << "Semantic Error: link " << link_name
                 << " in body of rule " << rule_id
                 << " appears more than twice" << endl;
            exit(1);
        }
        if ((int)link_port.size() == 1 && (int)head.link_port[link_name].size() != 1) {
            cerr << "Semantic Error: link " << link_name
                 << " in body of rule " << rule_id
                 << " is free variable" << endl;
            exit(1);
        }
    }
}



TopSet::Link& follow_link(TopSet &topset, TopSet::Link &pre, string &link_name) {
    for (TopSet::Link &to : topset.link_port[link_name]) {
        if (to == pre) continue;
        if (to.is_connector) {
            if (to.pos == 1) {
                TopSet::Link myself(true, to.id, 2);
                return follow_link(topset, myself, topset.connect[to.id].second);
            }
            if (to.pos == 2) {
                TopSet::Link myself(true, to.id, 1);
                return follow_link(topset, myself, topset.connect[to.id].first);
            }
            assert(false);
        } else {
            return to;
        }
    }
    return pre;
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
            string link_name = graph.atom_args[i][j];
            TopSet::Link myself(false, i, j);
            TopSet::Link &to_link = follow_link(graph, myself, link_name);

            assert(!to_link.is_connector);
            atoms[i]->link[j] = Link(atoms[to_link.id], to_link.pos);
        }
    }
}


void set_rule(TopSet &head, TopSet &body) {

    Rule rule;
    int head_atom_num = head.atom_name.size();
    int body_atom_num = body.atom_name.size();
    vector<RuleAtom*> head_atoms(head_atom_num);
    vector<RuleAtom*> body_atoms(body_atom_num);
    for (int i = 0; i < head_atom_num; i++) {
        Functor functor(head.atom_args[i].size(), head.atom_name[i]);
        head_atoms[i] = new RuleAtom(functor, i);
    }
    for (int i = 0; i < body_atom_num; i++) {
        Functor functor(body.atom_args[i].size(), body.atom_name[i]);
        body_atoms[i] = new RuleAtom(functor, i);
    }

    map<string, int> free_link_id;
    for (int i = 0; i < head_atom_num; i++) {
        Functor functor = head_atoms[i]->functor;
        for (int j = 0; j < functor.arity; j++) {
            string link_name = head.atom_args[i][j];
            TopSet::Link myself(false, i, j);
            TopSet::Link &to_link = follow_link(head, myself, link_name);

            // 自由リンク
            if ((int)head.link_port[link_name].size() == 1) {
                free_link_id[link_name] = rule.freelink_num;
                head_atoms[i]->link[j] = RuleLink(rule.freelink_num);
                rule.freelink_num++;
            } else if (to_link.is_connector) {
                string to_name = (to_link.pos == 1) ? head.connect[to_link.id].first : head.connect[to_link.id].second;
                free_link_id[to_name] = rule.freelink_num;
                head_atoms[i]->link[j] = RuleLink(rule.freelink_num);
                rule.freelink_num++;
            }
            // 局所リンク
            else {
                head_atoms[i]->link[j] = RuleLink(head_atoms[to_link.id], to_link.pos);
            }
        }
    }

    for (int i = 0; i < body_atom_num; i++) {
        Functor functor = body_atoms[i]->functor;
        for (int j = 0; j < functor.arity; j++) {
            string link_name = body.atom_args[i][j];
            TopSet::Link myself(false, i, j);
            TopSet::Link &to_link = follow_link(body, myself, link_name);

            // 自由リンク
            if ((int)body.link_port[link_name].size() == 1) {
                body_atoms[i]->link[j] = RuleLink(free_link_id[link_name]);
            } else if (to_link.is_connector) {
                string to_name = (to_link.pos == 1) ? body.connect[to_link.id].first : body.connect[to_link.id].second;
                body_atoms[i]->link[j] = RuleLink(free_link_id[to_name]);
            }
            // 局所リンク
            else {
                body_atoms[i]->link[j] = RuleLink(body_atoms[to_link.id], to_link.pos);
            }
        }
    }

    // connector
    for (pair<string,string> &connector : body.connect) {
        string link_name1 = connector.first;
        string link_name2 = connector.second;
        TopSet::Link null_link(true, -1, -1);
        if ((int)body.link_port[link_name1].size() == 1) {
            TopSet::Link &to_link = follow_link(body, null_link, link_name1);
            if (to_link.is_connector) {
                string to_name = (to_link.pos == 1) ? body.connect[to_link.id].first : body.connect[to_link.id].second;
                rule.connector.push_back({free_link_id[link_name1], free_link_id[to_name]});
            }
        } else if ((int)body.link_port[link_name2].size() == 1) {
            TopSet::Link &to_link = follow_link(body, null_link, link_name2);
            if (to_link.is_connector) {
                string to_name = (to_link.pos == 1) ? body.connect[to_link.id].first : body.connect[to_link.id].second;
                rule.connector.push_back({free_link_id[link_name2], free_link_id[to_name]});
            }
        }
    }
    rule.head = head_atoms;
    rule.body = body_atoms;
    rulelist.push_back(rule);
}


void parse() {
    vector<string> raw_inputs = read_istream();
    int y = 0, x = 0;
    Parser parser = read_sentences(raw_inputs, y, x);
    // parser.show();

    build_link_port(parser.graph);
    check_link_num_of_graph(parser.graph);
    
    int rule_num = parser.rule_head.size();
    for (int i = 0; i < rule_num; i++) {
        build_link_port(parser.rule_head[i]);
        build_link_port(parser.rule_body[i]);
        check_link_num_of_rule(i, parser.rule_head[i], parser.rule_body[i]);
    }

    set_graph(parser.graph);
    for (int i = 0; i < rule_num; i++) {
        set_rule(parser.rule_head[i], parser.rule_body[i]);
    }
}
