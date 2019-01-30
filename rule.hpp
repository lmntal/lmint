#ifndef RULE_HPP
#define RULE_HPP

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
#include <map>
using std::map;
using std::pair;
#include <unordered_set>
using std::unordered_set;
#include <list>
using std::list;
#include <algorithm>
using std::max;
using std::min;
#include <random>
#include <cassert>

#define debug(x) cerr << (#x) << ": " << (x) << endl;

class Functor;
class Atom;
class Link;
class RuleAtom;
class RuleLink;
class Guard;
class Register;
class Rule;

extern map<Functor,list<Atom*>> atomlist;
extern vector<Rule> rulelist;

class Functor {
  public:
    int arity;
    string name;

    Functor();
    ~Functor();
    Functor(int arity_, string name_);

    bool operator==(const Functor &rhs) const;
    bool operator!=(const Functor &rhs) const;
    bool is_int() const;
    bool operator<(const Functor &rhs) const;
    friend ostream& operator<<(ostream& ost, const Functor &rhs);

};


class Link {
  public:
    Atom *atom;
    int pos;

    Link();
    ~Link();
    Link(Atom *atom_, int pos_);

    bool operator<(const Link &rhs) const;
};

class Atom {
  public:
    Functor functor;
    vector<Link> link;
    list<Atom*>::iterator itr;

    Atom();
    ~Atom();
    Atom(Functor functor_);

    bool is_int();
};

class RuleLink {
  public:
    RuleAtom *atom;
    int pos;

    RuleLink();
    ~RuleLink();
    RuleLink(int pos_);
    RuleLink(RuleAtom *atom_, int pos_);

    bool is_freelink();
    int freelinkID();
};


class RuleAtom {
  public:
    Functor functor;
    int id;
    vector<RuleLink> link;

    RuleAtom();
    ~RuleAtom();
    RuleAtom(Functor functor_, int id_);
    RuleAtom(Functor functor_, int id_, vector<RuleLink> link_);
};


class Guard {
  public:

    class Compare {
      public:

        vector<string> left_exp;
        string op;
        vector<string> right_exp;
        
        Compare();
        ~Compare();
        Compare(vector<string> &left_exp_, string &op_, vector<string> &right_exp_);
        
        bool is_null();
    };


    class TypeCheck {
      public:
        string link;
        string type;

        TypeCheck();
        ~TypeCheck();
        TypeCheck(string link_, string type_);

        bool is_null();
    };

    class Assign {
      public:
        string new_var;
        vector<string> exp;

        Assign();
        ~Assign();

        bool is_null();
    };

    vector<TypeCheck> type_checks;
    vector<Compare> compares;
    vector<Assign> assigns;

    Guard();
    ~Guard();

    bool is_null();
};


class Rule {
  public:
    int freelink_num;
    vector<RuleAtom*> head_atoms;
    Guard guard;
    vector<RuleAtom*> body_atoms;
    vector<pair<int,int>> connectors;

    Rule();
    ~Rule();

    void show();
};


class Register {
  public:
    vector<Atom*> head_atoms;
    vector<Atom*> body_atoms;
    vector<Link> freelinks;

    Register();
    ~Register();
    Register(Rule &rule);
};


bool try_rule(Rule &rule);
bool find_atom(Rule &rule, Register &reg);
bool set_atom_to_reg(Rule &rule, Register &reg, Atom* atom, int head_id);
void remove_atom_from_reg(Rule &rule, Register &reg, Atom* atom, int head_id);
int eval_exp(Rule &rule, Register &reg, vector<string> &tokens, int &i);
int eval_factor(Rule &rule, Register &reg, vector<string> &tokens, int &i);
int eval_term(Rule &rule, Register &reg, vector<string> &tokens, int &i);
int eval_exp(Rule &rule, Register &reg, vector<string> &tokens, int &i);
bool eval_compare(int left_exp, string &op, int right_exp);
bool guard_check(Rule &rule, Register &reg);
void rewrite(Rule &rule, Register &reg);

void show_graph();
void nest_dump(Atom* atom, int depth, unordered_set<Atom*> &dumped_atoms, map<Link, int> &locallink_id);
void dump();
void show_atomlist_size();
void show_rules();

#endif