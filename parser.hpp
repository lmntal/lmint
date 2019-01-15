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
class PrsGuard;
class PrsRule;


class TopSet{
  public:
    class Link{
      public:
        bool is_connector;
        int id;
        int pos;
        Link();
        ~Link();
        Link(bool is_connector_, int id_, int pos_);

        bool operator==(const Link rhs) const;
        friend ostream& operator<<(ostream& ost, const Link &rhs);
    };

    static int orig_local_link_num;

    vector<string> atom_name;
    vector<vector<string>> atom_args;
    vector<pair<string,string>> connect;
    map<string,string> dst;

    TopSet();
    ~TopSet();

    int add_atom(string atom);
    static string make_orig_local_link_num();
    map<string,int> get_link_count();
    void build_dst_map();
    void check_link_num_of_graph();
    void show();
};


class PrsGuard{
  public:

    class Compare{
      public:
        vector<string> left_exp;
        string op;
        vector<string> right_exp;

        Compare();
        ~Compare();
        Compare(vector<string> &left_exp_, string &op_, vector<string> &right_exp_);
        bool is_null();
    };

    class TypeCheck{
      public:
        string link;
        string type;

        TypeCheck();
        ~TypeCheck();
        TypeCheck(string link_, string type_);

        bool is_null();
    };

    class Assign{
      public:
        string new_var;
        vector<string> exp;

        Assign();
        ~Assign();

        bool is_null();
    };

    vector<TypeCheck> type_check;
    vector<Compare> compare;
    vector<Assign> assign;

    PrsGuard();
    ~PrsGuard();

    bool is_null();
    void show();
};


class PrsRule{
  public:
    TopSet head, body;
    PrsGuard guard;

    PrsRule();
    ~PrsRule();
    PrsRule(TopSet head_, TopSet body_);
    PrsRule(TopSet head_, PrsGuard guard_, TopSet body_);

    void check_link_num_of_rule();
    void show();
};


class Parser{
  public:
    int y,x;
    vector<string> raw_inputs;
    TopSet graph;
    vector<PrsRule> rule;

    Parser();
    ~Parser();

    void read_istream();
    void syntax_error(string message);
    void read_ignore();
    bool read_token(string token);
    void read_sentences();
    TopSet read_topset();
    Result_of_nest read_nest(TopSet &topset);

    bool read_factor(vector<string> &tokens);
    bool read_exp(vector<string> &tokens);
    bool read_type_check(PrsGuard::TypeCheck &type_check);
    bool read_assign(PrsGuard::Assign &assign);
    bool read_compare(PrsGuard::Compare &compare);
    bool read_guard(PrsGuard &guard);

    string read_number();
    string read_name();

    void parse();
    void show();
};


class Result_of_nest{
  public:
    bool is_link;
    int atom_id;
    string link_name;

    Result_of_nest();
    ~Result_of_nest();
    Result_of_nest(string link_name_);
    Result_of_nest(int atom_id_);
};


bool is_link_initial(char c);
bool is_atom_initial(char c);

// vm
void set_graph(TopSet &graph);
void set_rule(PrsRule &parser_rule);
void load(Parser &parser);

#endif
