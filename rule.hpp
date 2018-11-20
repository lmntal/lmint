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

    Functor() {}
    ~Functor() {}
    Functor(int a, string n): arity(a),name(n) {}

    bool operator==(const Functor &r) const {
        return arity == r.arity && name == r.name;
    }
    bool operator!=(const Functor &r) const {
        return arity != r.arity || name != r.name;
    }
    bool operator<(const Functor &r) const {
        return name != r.name ? name < r.name : arity < r.arity;
    }

    friend ostream& operator<<(ostream& ost, const Functor &r) {
        ost << "<" << r.name << "," << r.arity << ">";
        return ost;
    }
};

class Link {
  public:
    Atom *atom;
    int pos;

    Link() {}
    ~Link() {}
    Link(Atom *a, int p): atom(a), pos(p) {}

    bool operator<(const Link &r) const {
        return atom != r.atom ? atom < r.atom : pos < r.pos;
    }
};

class Atom {
  public:
    Functor functor;
    vector<Link> link;
    list<Atom*>::iterator itr;

    Atom() {}
    ~Atom() {
        assert(*itr == this);
        atomlist[functor].erase(itr);
    }
    Atom(Functor f): functor(f) {
        link.resize(functor.arity);
        atomlist[functor].push_front(this);
        itr = atomlist[functor].begin();
    }
};

class RuleLink {
  public:
    RuleAtom *atom;
    int pos;

    RuleLink() {}
    ~RuleLink() {}
    RuleLink(int p): atom(NULL), pos(p) {}
    RuleLink(RuleAtom *a, int p): atom(a), pos(p) {}
    bool is_freelink() {
        return atom == NULL;
    }
    int freelinkID(){
        assert(is_freelink());
        return pos;
    }
};


class RuleAtom{
  public:
    Functor functor;
    int id;
    vector<RuleLink> link;

    RuleAtom() {}
    ~RuleAtom() {}
    RuleAtom(Functor f, int i): functor(f), id(i) {
        link.resize(functor.arity);
    }

    RuleAtom(Functor f, int i, vector<RuleLink> l):
        functor(f), id(i), link(l) {}

};

class Guard{
  public:
    string function;
    vector<string> variable;
};

class Rule {
  public:
    int freelink_num;
    vector<RuleAtom*> head;
    vector<Guard> gurad;
    vector<RuleAtom*> body;
    vector<pair<int,int>> connector;
    Rule(): freelink_num(0) {}
    ~Rule() {}

    void show() {
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
};

class Register {
  public:
    vector<Atom*> head;
    vector<Atom*> body;
    vector<Link> freelink;

    Register() {}
    ~Register() {}
    Register(Rule &rule) {
        head.resize(rule.head.size());
        body.resize(rule.body.size());
        freelink.resize(rule.freelink_num);
    }
};


bool find_atom(Rule &rule, Register &reg);
bool set_atom_to_reg(Rule &rule, Register &reg, Atom* atom, int hi);
void remove_atom_from_reg(Rule &rule, Register &reg, Atom* atom, int hi);
void rewrite(Rule &rule, Register &reg);

#endif