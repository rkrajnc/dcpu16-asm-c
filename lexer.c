#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#include "lexer.h"

static int is_name_char(char);
static int is_digit_char(char);
static int is_hex_char(char);

token_t * read_token(lexer_state * state)
{
  token_t * t;
  int type, value_size;
  char * left = state->ptr;
  char * right;
  char c;

  /* Ignore spaces. */
  while ((c = *left) && (c == ' ')) left++;
  right = left;

  if (c == ';')
  {
    while (c != '\n') c = *(++right);
    type = T_COMMENT;
  }
  else if (c == '\n')
  {
    right++;
    type = T_NEWLINE;
  }
  else if (is_name_char(c))
  {
    while (is_name_char(c)) c = *(++right);
    type = T_NAME;
  }
  else if (c == ':')
  {
    c = *(++right);
    while (is_name_char(c)) c = *(++right);
    type = T_LABEL;
  }
  else if (is_digit_char(c))
  {
    if (*(left + 1) == 'x') /* look-ahead */
    {
      while (is_hex_char(c)) c = *(++right);
      type = T_INT_HEX;
    }
    else
    {
      while (is_digit_char(c)) c = *(++right);
      type = T_INT_DEC;
    }
  }
  else if (c == '[' || c == ']')
  {
    right++;
    type = c == '[' ? T_BRACKET_L : T_BRACKET_R;
  }
  else if (c == ',')
  {
    right++;
    type = T_COMMA;
  }
  else if (c == '+')
  {
    right++;
    type = T_PLUS;
  }
  else if (c == 0)
  {
    return NULL;
  }
  else
  {
    printf("c: '%c' (0x%02x)\n", c, (int)c);
    CRASH("unhandled token");
  }

  t = (token_t *)malloc(sizeof(token_t));
  t->type = type;
  t->size = value_size = right - left;
  t->value = (char *)malloc(value_size + 1);
  strlcpy(t->value, left, value_size + 1);

  state->ptr = right;

  return t;
}

token_t * next_token(lexer_state * state)
{
  if (state->ahead > 0)
  {
    state->ahead--;
    state->token_buffer = state->token_buffer->next;
    return state->token_buffer->token;
  }
  else
  {
    return read_token(state);
  }
}

token_t * peek_token(lexer_state * state, int offset)
{
  int ahead;
  token_list* b;
  int i;

  if (offset > TOKEN_BUFFER_SIZE)
    CRASH("offset > TOKEN_BUFFER_SIZE");

  if (offset <= 0)
    CRASH("offset <= 0");

  ahead = state->ahead;
  b = state->token_buffer;

  for (i = 0; i < offset; i++)
  {
    b = b->next;
    if (i >= ahead)
    {
      b->token = read_token(state);
      state->ahead++;
    }
  }

  return b->token;
}

void lexer_init(lexer_state * state, char * source)
{
  int i;
  /* Circular linked list: 0 -> 1, 1 -> 2, 2 -> 3, 3 -> 0 */
  token_list * b = (token_list *)malloc(TOKEN_BUFFER_SIZE * sizeof(token_list));
  for (i = 0; i < TOKEN_BUFFER_SIZE; i++)
    (b + i)->next = (b + ((i + 1) % TOKEN_BUFFER_SIZE));

  state->ptr = source;
  state->token_buffer = b;
  state->ahead = 0;
}

static int is_name_char(char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int is_digit_char(char c)
{
  return c >= '0' && c <= '9';
}

static int is_hex_char(char c)
{
  return is_digit_char(c) || (c >= 'A' && c <= 'F') || c == 'x';
}
