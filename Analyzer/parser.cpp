#include "parser.h"

#include <sstream>
#include <vector>
#include <cstdio>
#include <stack>

Node::~Node() {
  for (auto child: children)
    delete child;
}

class StackEntry {
public:
  DFA *dfa;
  int state;
  Node *node;

  StackEntry(DFA *dfa, int state, Node *node):
    dfa(dfa), state(state), node(node) {}
};


bool IsInFirst(DFA *dfa, int label) {
  for (int i = 0; i < dfa->n_first; i++)
    if (label == dfa->first[i])
      return true;
  return false;
}

bool IsAcceptOnlyState(DFAState *state) {
  return state->is_final && state->n_arcs == 1;
}

void DumpStackEntry(StackEntry e) {
  printf("state: %d\n", e.state);
  printf("dfa: %s\n", e.dfa->name);
  /* DumpDFA(e.dfa); */
}

class Parser {
public:
  std::stack<StackEntry> stack;
  Grammar *grammar;
  Node *rootnode;

  explicit Parser(Grammar *grammar): stack(), grammar(grammar) {
    auto start = grammar->start;
    Node *newnode = new Node(static_cast<Symbol>(start), "", Location(0, 0));
    rootnode = nullptr;
    stack.push(StackEntry(grammar->dfas[start - NT_OFFSET], 0, newnode));
  }

  int Classify(const TokenInfo &token) {
    if (token.type == Symbol::NAME || token.type == Symbol::OP) {
      for (int i = 1; i < grammar->n_labels; i++) {
        const Label &l = grammar->labels[i];
        if (l.string && l.string == token.string)
          return i;
      }
    }
    for (int i = 1; i < grammar->n_labels; i++) {
      const Label &l = grammar->labels[i];
      if (l.type == static_cast<int>(token.type) && l.string == nullptr) {
        return i;
      }
    }
    Error(token.start, "unexpected", Quote(token.string));
    return -1;
  }

  void Shift(const TokenInfo &token, int newstate) {
    /* printf("shift %s\n", GetSymName(token.type)); */
    StackEntry &tos = stack.top();
    tos.node->AddChild(new Node(token.type, token.string, token.start));
    tos.state = newstate;
  }

  void Push(int type, DFA *newdfa, int newstate, Location location) {
    /* printf("push %s\n", GetSymName(type)); */
    StackEntry &tos = stack.top();
    Node *newnode = new Node(static_cast<Symbol>(type), "", location);
    tos.state = newstate;
    stack.push(StackEntry(newdfa, 0, newnode));
  }

  void Pop() {
    /* printf("pop\n"); */
    StackEntry tos = stack.top();
    stack.pop();
    Node *newnode = tos.node;

    if (stack.size()) {
      stack.top().node->AddChild(newnode);
    }
    else {
      rootnode = newnode;
    }
  }

  bool AddToken(const TokenInfo &token) {
    int label = Classify(token);
    if (label < 0) {
      return -1;
    }
    /* token.Format(stdout); */

    while (true) {
      StackEntry &tos = stack.top();
      DFA *dfa = tos.dfa;
      DFAState *states = dfa->states;
      DFAState *state = &states[tos.state];
      bool flag = true;

      /* DumpStackEntry(tos); */

      for (int i = 0; i < state->n_arcs; ++i) {
        Arc &arc = state->arcs[i];
        int type = grammar->labels[arc.label].type;
        int newstate = arc.state;

        if (label == arc.label) {
          Shift(token, newstate);
          /* printf("shift %s\n", GetSymName(type)); */

          while (IsAcceptOnlyState(&states[newstate])) {
            /* printf("pop\n"); */
            Pop();
            if (stack.empty()) {
              return true;
            }
            newstate = stack.top().state;
            states = stack.top().dfa->states;
          }
          return false;
        }

        else if (type >= 256) {
          DFA *itsdfa = grammar->dfas[type - NT_OFFSET];
          if (IsInFirst(itsdfa, label)) {
            /* printf("push %s\n", GetSymName(type)); */
            Push(type, itsdfa, newstate, token.start);
            flag = false;
            break;
          }
        }
      }

      if (flag) {
        if (state->is_final) {
          Pop();
          if (stack.empty()) {
            Error(token.start, "too much input");
            return -1;
          }
        }
        else {
          Error(token.start, "unexpected", Quote(token.line));
          return -1;
        }
      }
    }
  }

};

Node *ParseTokens(const TokenBuffer &tokens) {
  Parser parser(&CompilerGrammar);

  for (auto token: tokens) {
    if (token->type == Symbol::ERRORTOKEN) {
      Error(token->start, "error token", Quote(token->string));
      return nullptr;
    }
    int ret = parser.AddToken(*token);
    if (ret == 1) {
      return parser.rootnode;
    }
    if (ret < 0) {
      return nullptr;
    }
  }
  auto last = tokens.end() - 1;
  Error((*last)->start, "incomplete input");
  return nullptr;
}
