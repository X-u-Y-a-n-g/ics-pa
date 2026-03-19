#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <ctype.h>
#include <stdlib.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NE, TK_AND, TK_OR, TK_LT, TK_LE, TK_GT, TK_GE,
  TK_NOT, TK_NUM, TK_REG, TK_DEREF, TK_NEG, TK_BNOT, TK_SHL, TK_SHR

  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\$[a-zA-Z]{2,3}", TK_REG},   // register
  {"0[xX][0-9a-fA-F]+", TK_NUM},  // hex number
  {"[0-9]+", TK_NUM},             // decimal number
  {"\\|\\|", TK_OR},              // logical or
  {"&&", TK_AND},                 // logical and
  {"==", TK_EQ},                  // equal
  {"!=", TK_NE},                  // not equal
  {"<=", TK_LE},                  // less or equal
  {">=", TK_GE},                  // greater or equal
  {"<<", TK_SHL},                 // shift left
  {">>", TK_SHR},                 // shift right
  {"<", TK_LT},                   // less
  {">", TK_GT},                   // greater
  {"\\+", '+'},                   // plus
  {"-", '-'},                     // minus
  {"\\*", '*'},                   // multiply
  {"/", '/'},                     // divide
  {"&", '&'},                     // bitwise and
  {"\\|", '|'},                   // bitwise or
  {"\\^", '^'},                   // bitwise xor
  {"~", TK_BNOT},                 // bitwise not
  {"!", TK_NOT},                  // logical not
  {"\\(", '('},                   // left parenthesis
  {"\\)", ')'}                    // right parenthesis
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          case TK_NUM:
            assert(substr_len < (int)sizeof(tokens[nr_token].str));
            tokens[nr_token].type = TK_NUM;
            memcpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
            break;
          case TK_REG:
            assert(substr_len - 1 < (int)sizeof(tokens[nr_token].str));
            tokens[nr_token].type = TK_REG;
            for (int k = 1; k < substr_len; k++) {
              tokens[nr_token].str[k - 1] = (char)tolower((unsigned char)substr_start[k]);
            }
            tokens[nr_token].str[substr_len - 1] = '\0';
            nr_token++;
            break;
          default:
            tokens[nr_token].type = rules[i].token_type;
            tokens[nr_token].str[0] = '\0';
            nr_token++;
            break;
        }
        assert(nr_token < (int)(sizeof(tokens) / sizeof(tokens[0])));

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

static bool is_value_token(int type) {
  return type == TK_NUM || type == TK_REG || type == ')';
}

static bool check_parentheses(int p, int q) {
  if (tokens[p].type != '(' || tokens[q].type != ')') {
    return false;
  }
  int balance = 0;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') {
      balance++;
    } else if (tokens[i].type == ')') {
      balance--;
    }
    if (balance == 0 && i < q) {
      return false;
    }
    if (balance < 0) {
      return false;
    }
  }
  return balance == 0;
}

static int token_precedence(int type) {
  switch (type) {
    case TK_OR: return 0;
    case TK_AND: return 1;
    case '|': return 2;
    case '^': return 3;
    case '&': return 4;
    case TK_EQ:
    case TK_NE: return 5;
    case TK_LT:
    case TK_LE:
    case TK_GT:
    case TK_GE: return 6;
    case TK_SHL:
    case TK_SHR: return 7;
    case '+':
    case '-': return 8;
    case '*':
    case '/': return 9;
    case TK_NEG:
    case TK_DEREF:
    case TK_BNOT:
    case TK_NOT: return 10;
    default: return 99;
  }
}

static bool is_unary_op(int type) {
  return type == TK_NEG || type == TK_DEREF || type == TK_BNOT ||
         type == TK_NOT;
}

static int dominant_op(int p, int q, bool *success) {
  int op = -1;
  int min_prec = 100;
  int balance = 0;
  for (int i = p; i <= q; i++) {
    int type = tokens[i].type;
    if (type == '(') {
      balance++;
      continue;
    }
    if (type == ')') {
      balance--;
      if (balance < 0) {
        *success = false;
        return -1;
      }
      continue;
    }
    if (balance != 0) {
      continue;
    }

    int prec = token_precedence(type);
    if (prec == 99) {
      continue;
    }

    if (prec < min_prec) {
      min_prec = prec;
      op = i;
    } else if (prec == min_prec) {
      if (!is_unary_op(type)) {
        op = i;
      }
    }
  }
  if (balance != 0) {
    *success = false;
    return -1;
  }
  return op;
}

static bool reg_val(const char *name, uint32_t *val) {
  if (strcmp(name, "eip") == 0) {
    *val = cpu.eip;
    return true;
  }
  for (int i = 0; i < 8; i++) {
    if (strcmp(name, regsl[i]) == 0) {
      *val = reg_l(i);
      return true;
    }
    if (strcmp(name, regsw[i]) == 0) {
      *val = reg_w(i);
      return true;
    }
    if (strcmp(name, regsb[i]) == 0) {
      *val = reg_b(i);
      return true;
    }
  }
  return false;
}

static uint32_t eval(int p, int q, bool *success) {
  if (p > q) {
    *success = false;
    return 0;
  }
  if (p == q) {
    if (tokens[p].type == TK_NUM) {
      char *endptr = NULL;
      uint32_t val = (uint32_t)strtoul(tokens[p].str, &endptr, 0);
      if (endptr == tokens[p].str || *endptr != '\0') {
        *success = false;
        return 0;
      }
      return val;
    }
    if (tokens[p].type == TK_REG) {
      uint32_t val = 0;
      if (!reg_val(tokens[p].str, &val)) {
        *success = false;
        return 0;
      }
      return val;
    }
    *success = false;
    return 0;
  }

  if (check_parentheses(p, q)) {
    return eval(p + 1, q - 1, success);
  }

  int op = dominant_op(p, q, success);
  if (!*success || op < 0) {
    return 0;
  }

  if (tokens[op].type == TK_NEG || tokens[op].type == TK_DEREF ||
      tokens[op].type == TK_BNOT || tokens[op].type == TK_NOT) {
    if (op != p) {
      *success = false;
      return 0;
    }
    uint32_t val = eval(op + 1, q, success);
    if (!*success) { return 0; }
    switch (tokens[op].type) {
      case TK_NEG: return 0 - val;
      case TK_DEREF: return vaddr_read(val, 4);
      case TK_BNOT: return ~val;
      case TK_NOT: return !val;
      default: break;
    }
  }

  uint32_t val1 = eval(p, op - 1, success);
  if (!*success) { return 0; }
  uint32_t val2 = eval(op + 1, q, success);
  if (!*success) { return 0; }

  switch (tokens[op].type) {
    case '+': return val1 + val2;
    case '-': return val1 - val2;
    case '*': return val1 * val2;
    case '/':
      if (val2 == 0) { *success = false; return 0; }
      return val1 / val2;
    case TK_EQ: return (val1 == val2);
    case TK_NE: return (val1 != val2);
    case TK_LT: return (val1 < val2);
    case TK_LE: return (val1 <= val2);
    case TK_GT: return (val1 > val2);
    case TK_GE: return (val1 >= val2);
    case TK_AND: return (val1 && val2);
    case TK_OR: return (val1 || val2);
    case '&': return val1 & val2;
    case '|': return val1 | val2;
    case '^': return val1 ^ val2;
    case TK_SHL: return val1 << (val2 & 31);
    case TK_SHR: return val1 >> (val2 & 31);
    default:
      *success = false;
      return 0;
  }
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  for (int i = 0; i < nr_token; i++) {
    if (tokens[i].type == '*' || tokens[i].type == '-') {
      if (i == 0 || !is_value_token(tokens[i - 1].type)) {
        tokens[i].type = (tokens[i].type == '*') ? TK_DEREF : TK_NEG;
      }
    }
    if (tokens[i].type == TK_BNOT || tokens[i].type == TK_NOT) {
      if (i != 0 && is_value_token(tokens[i - 1].type)) {
        *success = false;
        return 0;
      }
    }
  }

  /* TODO: Insert codes to evaluate the expression. */
  *success = true;
  if (nr_token == 0) {
    *success = false;
    return 0;
  }
  return eval(0, nr_token - 1, success);

  return 0;
}
