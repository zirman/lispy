#ifndef __MAIN__
#define __MAIN__

typedef const char *Atom;
typedef const struct List *List;

typedef const struct Expression *Expression;
typedef struct Expression *_Expression;

typedef const struct Lambda *Lambda;
typedef struct Lambda *_Lambda;

typedef const struct Scope *Scope;
typedef struct Scope *_Scope;

typedef const double Number;
typedef const char *String;

typedef Expression (*Function)(List list);

struct List {
  Expression head;
  List tail;
};

struct Lambda {
  List arguments;
  Expression expression;
};

typedef union Value {
  Atom atom;
  struct List list;
  Function function;
  Number number;
  String string;
  struct Lambda lambda;
} *Value;

typedef enum Type Type;

enum Type {
  UNDEFINED,
  ATOM,
  LIST,
  CORE,
  BOOLEAN,
  NUMBER,
  STRING,
  LAMBDA,
};

struct Scope {
  List arguments;
  List values;
  Scope parent;
};

struct Expression {
  Type type;
  union Value to;
};

extern Expression const kUndefined;

// Safe macros for converting expressions to specific types.

#define TO_ATOM(expression)\
  (assert(expression->type == ATOM), expression->to.atom)

#define TO_LIST(expression)\
  (assert(expression == NULL || expression->type == LIST),\
   (List)(expression ? &expression->to.list : NULL))

// Converts a value to an expression.

#define TO_EXPRESSION(value) \
  (value == NULL ? NULL : (Expression)((void *)value - to_offset))

// Array implimentation

#define ATOM_CASES\
 case '!':\
 case '"':\
 case '#':\
 case '$':\
 case '%':\
 case '&':\
 case '\'':\
 case '*':\
 case '+':\
 case '-':\
 case '.':\
 case '/':\
 case '0':\
 case '1':\
 case '2':\
 case '3':\
 case '4':\
 case '5':\
 case '6':\
 case '7':\
 case '8':\
 case '9':\
 case ':':\
 case ';':\
 case '<':\
 case '=':\
 case '>':\
 case '?':\
 case '@':\
 case 'A':\
 case 'B':\
 case 'C':\
 case 'D':\
 case 'E':\
 case 'F':\
 case 'G':\
 case 'H':\
 case 'I':\
 case 'J':\
 case 'K':\
 case 'L':\
 case 'M':\
 case 'N':\
 case 'O':\
 case 'P':\
 case 'Q':\
 case 'R':\
 case 'S':\
 case 'T':\
 case 'U':\
 case 'V':\
 case 'W':\
 case 'X':\
 case 'Y':\
 case 'Z':\
 case '\\':\
 case '^':\
 case '_':\
 case '`':\
 case 'a':\
 case 'b':\
 case 'c':\
 case 'd':\
 case 'e':\
 case 'f':\
 case 'g':\
 case 'h':\
 case 'i':\
 case 'j':\
 case 'k':\
 case 'l':\
 case 'm':\
 case 'n':\
 case 'o':\
 case 'p':\
 case 'q':\
 case 'r':\
 case 's':\
 case 't':\
 case 'u':\
 case 'v':\
 case 'w':\
 case 'x':\
 case 'y':\
 case 'z':\
 case '|':\
 case '~'

#define WHITESPACE_CASES\
 case ' ':\
 case ',':\
 case '\t':\
 case '\n':\
 case '\r'

#endif // __MAIN__
