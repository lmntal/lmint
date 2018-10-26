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

map<Functor,list<Atom*>> atomlist;
vector<Rule> rulelist;

class Functor {
  public:
    int arity;
    string name;

    Functor() {}
    ~Functor() {}
    Functor(int a, string n): arity(a),name(n) {}

    bool operator==(const Functor r) const {
        return arity == r.arity && name == r.name;
    }
    bool operator!=(const Functor r) const {
        return arity != r.arity || name != r.name;
    }
    bool operator<(const Functor r) const {
        return name != r.name ? name < r.name : arity < r.arity;
    }

    friend ostream& operator<<(ostream& ost, const Functor& r) {
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
    vector<pair<int,int>> connect;
    Rule() {}
    ~Rule() {}
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
