#include <iostream>
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::ostream;
using std::getline;
#include <vector>
using std::vector;
#include <string>
using std::string;
using std::to_string;
#include <map>
using std::pair;
#include <algorithm>
using std::max;
using std::min;
#include <cassert>

// EBNF of LMNtal parser
// Program := {(Rule | Graph) '.'}
// Rule := TopSet ':-' TopSet
// Graph := TopSet
// TopSet := (Nest | Nest '=' Nest) {',' (Nest | Nest '=' Nest)}
// Nest := Link | Atom | Atom '(' ')' | Atom '(' Nest {',' Nest} ')'
// Atom := [a-z]{[a-zA-Z_0-9]}
// Link := [A-Z_]{[a-zA-Z_0-9]}

class Parser;
class TopSet;
class Result_of_nest;


vector<string> read_istream();
void syntax_error(vector<string> &raw_inputs, int &y, int &x, string message);
void read_ignore(vector<string> &raw_inputs, int &y, int &x);
bool read_token(vector<string> &raw_inputs, int &y, int &x, string token);
Parser read_sentences(vector<string> &raw_inputs, int &y, int &x);
TopSet read_topset(vector<string> &raw_inputs, int &y, int &x);
Result_of_nest read_nest(vector<string> &raw_inputs, int &y, int &x, TopSet &topset);
string read_name(vector<string> &raw_inputs, int &y, int &x);
bool is_link_initial(char c);
bool is_atom_initial(char c);


class TopSet{
  public:
    static int orig_local_link_num;
    vector<string> atom_name;
    vector<vector<string>> atom_args;
    vector<pair<string,string>> connect;
    TopSet() {}
    ~TopSet() {}
    int add_atom(string atom) {
        atom_name.push_back(atom);
        int atom_id = atom_args.size();
        atom_args.resize(atom_id + 1);
        return atom_id;
    }
    static string make_orig_local_link_num() {
        return "#" + to_string(orig_local_link_num++);
    }
    void show() {
        for (int i = 0; i < (int)atom_name.size(); i++) {
            cout << atom_name[i] << "(";
            for (int j = 0; j < (int)atom_args[i].size(); j++) {
                cout << atom_args[i][j] 
                     << (j + 1 == (int)atom_args[i].size() ? "" : ", ");
            }
            cout << ")" << endl;
        }
        for (auto &p : connect) {
            cout << p.first << " = " << p.second << endl;
        }
    }
};
int TopSet::orig_local_link_num = 0;


class Parser{
  public:
    vector<TopSet> rule_head;
    vector<TopSet> rule_body;
    TopSet graph;
    Parser() {}
    ~Parser() {}
    void show();
};
void Parser::show() {
    // Graph
    printf("------------ Graph ------------\n");
    graph.show();

    // Rule
    int R = rule_head.size();
    for (int i = 0; i < R; i++) {
        printf("------------ Rule %d ------------\n", i);
        rule_head[i].show();
        cout << " :- " << endl;
        rule_body[i].show();
    }
}


class Result_of_nest{
  public:
    bool is_link;
    int atom_id;
    string link_name;
    Result_of_nest(){}
    ~Result_of_nest(){}
    Result_of_nest(string link_name_): is_link(true), link_name(link_name_) {}
    Result_of_nest(int atom_id_): is_link(false), atom_id(atom_id_) {}

};


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
    cerr << raw_inputs[y].substr(l, 20) << endl;
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
    return islower(c);
}


//  Atom := [a-z][a-zA-Z_0-9]*
//  Link := [A-Z_][a-zA-Z_0-9]*
string read_name(vector<string> &raw_inputs, int &y, int &x) {
    string name;
    while (x < (int)raw_inputs[y].size()) {
        char c = raw_inputs[y][x];
        if (isalpha(c) || isdigit(c) || c == '_') {
            name += c;
            x++;
        } else {
            break;
        }
    }
    read_ignore(raw_inputs, y, x);
    return name;
}



int main(){
    vector<string> raw_inputs = read_istream();
    int y = 0, x = 0;
    Parser parser = read_sentences(raw_inputs, y, x);
    parser.show();

    return 0;
}
