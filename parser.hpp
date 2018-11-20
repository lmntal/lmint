#ifndef PARSER_HPP
#define PARSER_HPP

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
using std::map;
using std::pair;
#include <algorithm>
using std::max;
using std::min;
#include <cassert>

#include "rule.hpp"

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

class TopSet{
  public:
    class Link{
      public:
        bool is_connector;
        int id;
        int pos;
        Link() {}
        ~Link() {}
        Link(bool is_connector_, int id_, int pos_):
            is_connector(is_connector_), id(id_), pos(pos_) {}
        bool operator==(const Link r) const {
            return is_connector == r.is_connector && id == r.id && pos == r.pos;
        }
        friend ostream& operator<<(ostream& ost, const Link& r) {
            ost << "<" << (r.is_connector ? "( = ) " : "(...) ") << r.id << "," << r.pos << ">";
            return ost;
        }
    };

    static int orig_local_link_num;

    vector<string> atom_name;
    vector<vector<string>> atom_args;
    vector<pair<string,string>> connect;
    map<string,vector<Link>> link_port;

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
};


class Parser{
  public:
    vector<TopSet> rule_head;
    vector<TopSet> rule_body;
    TopSet graph;
    Parser() {}
    ~Parser() {}
    void show() {
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
};


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

void build_link_port(TopSet &topset);
void check_link_num_of_graph(TopSet &topset);
void check_link_num_of_rule(int rule_id, TopSet &head, TopSet &body);
TopSet::Link& follow_link(TopSet &topset, TopSet::Link &pre, string &link_name);
void set_graph(TopSet &graph);
void set_rule(TopSet &head, TopSet &body);
void parse();

#endif