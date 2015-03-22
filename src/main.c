#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

// TODO: garbage collection
// TODO: macros
// TODO: multi arity functions
// TODO: primitives: numbers, strings, dates, regex, keywords
// TODO: spagette stack implementation of lists
// TODO: array data structure
// TODO: map data structure
// TODO: set data structure
// TODO: tail call optimization
// TODO: JIT

const struct Expression _kUndefined = {
  .type = UNDEFINED
};

Expression const kUndefined = &_kUndefined;

const size_t to_offset =  offsetof(struct Expression, to);

// Buffer globals

const size_t kBuf_size = 1024;
int8_t *start_buf = NULL;
int8_t *end_buf = NULL;
int8_t *cursor_buf = NULL;
int8_t *fill_buf = NULL;

// Scope globals

Scope scope = NULL;

void *alloc(const size_t size, const char * const s);
void perror_quit(const char * const s);

// Grows buffer if full and then reads to buffer.

void read_buf();

// Shifts bytes after cursor to beginning of buffer.

void shift_buf();

Expression read_expression();

void print_expression(Expression expression);
void print_break();

void error(const char * const s);

Expression lookup(Atom atom);
Expression atom(const char * const a);

Expression head_c(List l);
List tail_c(List l);
List cons_c(Expression head, List tail);

// Special forms 'lambda', 'quote', and 'define' are non-strict and cannot be
// used as a value.

// Core functions that are strictly evalutated and can be used as values.

Expression eval(List l);
Expression cons(List l);
Expression head(List l);
Expression tail(List l);

struct Expression core[] = {{
    .type = CORE,
    .to.function = eval   //0
  }, {
    .type = CORE,
    .to.function = cons   //1
  }, {
    .type = CORE,
    .to.function = head   //2
  }, {
    .type = CORE,
    .to.function = tail   //3
  }
};

Expression eval_lambda(Lambda lambda, List l);

int main(int argc, char **argv) {
  start_buf = alloc(kBuf_size, "malloc");
  end_buf = start_buf + kBuf_size;
  cursor_buf = start_buf;
  fill_buf = start_buf;

  for (;;) {
    Expression expression = read_expression();
    Expression result = eval(cons_c(expression, NULL));
    print_expression(result);
    print_break();
    shift_buf();
  }
}

List recur_parse_list() {

  for (;;) {

    if (cursor_buf >= fill_buf) {
      read_buf();
    }

    switch (*cursor_buf) {
    WHITESPACE_CASES:
      cursor_buf++;
      continue;

    case ')':
      cursor_buf++;
      return NULL;

    default:
      return cons_c(read_expression(), recur_parse_list());
    }
  }
}

Expression read_expression() {

  for (;;) {

    if (cursor_buf >= fill_buf) {
      read_buf();
    }

    switch (*cursor_buf) {
    WHITESPACE_CASES:
      cursor_buf++;
      continue;

    case '(':
      cursor_buf++;
      {
        List l = recur_parse_list();
        return TO_EXPRESSION(l);
      }

    ATOM_CASES: {

        // Save str index because buffer could be moved by read_buf().

        size_t index_str = cursor_buf - start_buf;
        cursor_buf++;

        for (;;) {

          if (cursor_buf >= fill_buf) {
            read_buf();
          }

          switch (*cursor_buf) {
          ATOM_CASES:
            cursor_buf++;
            continue;

          default: {
              const size_t size_str =
                (cursor_buf - start_buf) - (index_str) + 1;

              char * const ptr = (char *)alloc(size_str, "malloc");
              strlcpy(ptr, (char *)(start_buf + index_str), size_str);
              return atom(ptr);
            }
          }
        }
      }

    default:
      error(__func__);
    }
  }
}

Expression atom(const char * const atom) {
  _Expression expression =
    (_Expression)alloc(to_offset + sizeof(Atom), __func__);
  expression->type = ATOM;
  expression->to.atom = atom;
  return expression;
}

List cons_c(Expression head, List tail) {
  _Expression expression =
    (_Expression)alloc(to_offset + sizeof(struct List), __func__);
  expression->type = LIST;
  expression->to.list.head = head;
  expression->to.list.tail = tail;
  return &expression->to.list;
}

Expression head_c(List list) {

  if (! list) {
    error("head on empty list");
  }

  return list->head;
}

List tail_c(List list) {

  if (! list) {
    error("tail on empty list");
  }

  return list->tail;
}

void recur_print_list(FILE * restrict stream, List list) {

  if (list == NULL) {
    return;
  }

  fputc(' ', stream);
  print_expression(list->head);
  recur_print_list(stream, list->tail);
}

void fprint_expression(FILE * restrict stream, Expression expression) {
  assert(expression != NULL);

  switch (expression->type) {
  case UNDEFINED:
    fprintf(stream, "UNDEFINED");
    return;

  case ATOM:
    fprintf(stream, "%s", expression->to.atom);
    return;

  case LIST:
    fputc('(', stream);
    fprint_expression(stream, expression->to.list.head);
    recur_print_list(stream, expression->to.list.tail);
    fputc(')', stream);
    return;

  case CORE:
    switch (expression - core) {
    case 0:
      fprintf(stream, "CORE: eval");
      return;
    case 1:
      fprintf(stream, "CORE: cons");
      return;
    case 2:
      fprintf(stream, "CORE: head");
      return;
    case 3:
      fprintf(stream, "CORE: tail");
      return;
    default:
      assert(0);
    }

  case LAMBDA:
    fprintf(stream, "(lambda ");
    fprint_expression(stream, TO_EXPRESSION(expression->to.lambda.arguments));
    fputc(' ', stream);
    fprint_expression(stream, expression->to.lambda.expression);
    fputc(')', stream);
    return;

  case NUMBER:
    fprintf(stream, "NUMBER: ");
    return;

  case STRING:
    fprintf(stream, "STRING: %s", expression->to.string);
    return;
  }
}

void print_expression(Expression expression) {
  fprint_expression(stdout, expression);
}

void print_break() {
  fputc('\n', stdout);
}

List recur_strict_eval(List l) {

  if (l == NULL) {
    return NULL;
  }

  return cons_c(eval(cons_c(l->head, NULL)), recur_strict_eval(l->tail));
}

Expression eval(List l) {
  assert(tail_c(l) == NULL);
  Expression expression = l->head;

  if (expression == NULL) {
    error("empty list evaled");
  }

  switch (expression->type) {
  case ATOM:
    return lookup(expression->to.atom);

  case LIST:
    {
      l = &expression->to.list;
      Expression head_expression = l->head;

      // Eval special forms

      if (head_expression->type == ATOM) {
        Atom a = head_expression->to.atom;

        switch (*a++) {
        case 'l': // l|ambda

          if (strcmp("ambda", a)) {
            break;
          }

          { // (lambda (argument1 argument2 ...) expression) special form

            List ll = l->tail;
            assert(ll != NULL);
            List lll = ll->tail;
            assert(lll != NULL && lll->tail == NULL);

            // TODO: validate lambda expression

            _Expression new_expression =
              (_Expression)alloc(to_offset + sizeof(struct Lambda),
                                 __func__);

            new_expression->type = LAMBDA;
            _Lambda lambda = &new_expression->to.lambda;
            lambda->arguments = TO_LIST(head_c(ll));
            lambda->expression = head_c(lll);
            return new_expression;
          }

        case 'q': // q|uote

          if (strcmp("uote", a)) {
            break;
          }

          {  // (quote x) special form
            List ll = l->tail;
            assert(ll != NULL && ll->tail == NULL);
            return ll->head;
          }
        }
      }

      // Perform strict evaluation of functions and lambdas.

      l = recur_strict_eval(&expression->to.list);
      expression = l->head;

      switch(expression->type) {
      case CORE:
        return expression->to.function(l->tail);

      case LAMBDA:
        return eval_lambda(&l->head->to.lambda, l->tail);

      default:
        assert(0);
      }
    }

  case CORE:
  case LAMBDA:
  case NUMBER:
  case STRING:
  case UNDEFINED:
    return expression;
  }

  error("invalid type evaled");
}

void read_buf() {
  size_t size;

  if (fill_buf >= end_buf) {
    size = (end_buf - start_buf) * 2;
    size_t cursor = cursor_buf - start_buf;
    size_t fill = fill_buf - start_buf;
    start_buf = realloc(start_buf, size);

    if (start_buf == NULL) {
      perror("realloc");
      exit(-1);
    }

    cursor_buf = start_buf + cursor;
    fill_buf = start_buf + fill;
    end_buf = start_buf + size;
  }

  size = end_buf - fill_buf;

  for (;;) {
    ssize_t retval = read(STDIN_FILENO, fill_buf, size);

    switch (retval) {
    case -1: // Error

      if (errno == EINTR) {  // interrupted by a signal.
        continue;
      }

      perror(__func__);
      exit(-1);

    case 0: // EOF
      exit(0);

    default:
      fill_buf = fill_buf + retval;
      return;
    }
  }
}

void shift_buf() {
  memmove(start_buf, cursor_buf, fill_buf - cursor_buf);
  fill_buf = start_buf + (size_t)(fill_buf - cursor_buf);
  cursor_buf = start_buf;
}

void *alloc(const size_t size, const char * const s) {
  void *ptr = malloc(size);

  if (ptr == NULL) {
    perror(s);
    exit(-1);
  }

  return ptr;
}

void error(const char * const s) {
  fprintf(stderr, "error: %s\n", s);
  exit(-1);
}

Expression lookup(Atom atom) {

  // Error when looking up special form keyword

  Atom a = atom;

  switch (*a++) {
  case 'l': // l|ambda

    if (strcmp("ambda", a)) {
      break;
    }

    // lambda special form
    error("evaled 'lambda' special form keyword");
    return NULL;

  case 'q': // q|uote

    if (strcmp("uote", a)) {
      break;
    }

    // quote special form
    error("evaled 'quote' special form keyword");
    return NULL;

  case 'e': // e|val
    return strcmp(a, "val") ? NULL : core + 0;
  case 'c': // c|ons
    return strcmp(a, "ons") ? NULL : core + 1;
  case 'h': // h|ead
    return strcmp(a, "ead") ? NULL : core + 2;
  case 't': // t|ail
    return strcmp(a, "ail") ? NULL : core + 3;
  default:
    break;
  }

  Scope cur_scope = scope;

  while (cur_scope != NULL) {
    List arguments = cur_scope->arguments;
    List values = cur_scope->values;

    while (arguments != NULL) {

      if (! strcmp(TO_ATOM(head_c(arguments)), atom)) {
        return head_c(values);
      }

      arguments = tail_c(arguments);
      values = tail_c(values);
    }

    cur_scope = cur_scope->parent;
  }

  return kUndefined;
}

Expression cons(List l) {
  assert(tail_c(tail_c(l)) == NULL);
  List ll = tail_c(l);
  _Expression expression =
    (_Expression)alloc(to_offset + sizeof(struct List), __func__);
  expression->type = LIST;
  expression->to.list.head = l->head;
  expression->to.list.tail = TO_LIST(ll->head);
  return expression;
}

Expression head(List l) {
  assert(tail_c(l) == NULL);
  Expression expression = l->head;
  assert(expression->type == LIST);
  return expression->to.list.head;
}

Expression tail(List l) {
  assert(tail_c(l) == NULL);
  Expression expression = head_c(l);
  List list = tail_c(TO_LIST(expression));
  return TO_EXPRESSION(list);
}

Expression eval_lambda(Lambda lambda, List values) {
  _Scope new_scope = (_Scope)alloc(sizeof(struct Scope), __func__);
  new_scope->arguments = lambda->arguments;
  new_scope->values = values;
  new_scope->parent = scope;
  scope = new_scope;
  Expression result = eval(cons_c(lambda->expression, NULL));
  scope = scope->parent;
  return result;
}
