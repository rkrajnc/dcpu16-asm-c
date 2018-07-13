#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assembler.h"
#include "utils.h"

/* TODO: dynamically grow program code allocation. */
#define PROGRAM_MAX_LENGTH 1024

static void assembler_state_init(assembler_state_t *);
static void assemble_statement(assembler_state_t *, statement_t *);
static void assemble_operand(assembler_state_t *, instruction_t *, operand_t *, int index);

program_t * assemble(parse_result_t * pr)
{
  statement_t * s;
  int i;
  uint16_t address, resolved;


  assembler_state_t state;
  assembler_state_init(&state);

  for (i = 0; i < pr->statement_count; i++)
  {
    s = &pr->statements[i];
    assemble_statement(&state, s);
  }

  for (i = 0; i < state.label_refs.length; i++)
  {
    address = state.label_refs.address[i];
    resolved = label_lookup(&state.label_defs, state.label_refs.label[i]);
    state.program->code[address] = resolved;
  }

  return state.program;
}

static void assembler_state_init(assembler_state_t * s)
{
  program_t * p = (program_t *)malloc(
      sizeof(program_t) +
      sizeof(uint16_t) * PROGRAM_MAX_LENGTH
  );
  if (!p) CRASH("malloc program_t");
  p->code = (uint16_t *)(p + 1);
  p->length = 0;

  s->program = p;
  s->next_word = p->code;
  label_table_init(&s->label_defs);
  label_table_init(&s->label_refs);
}

static void assemble_statement(assembler_state_t * state, statement_t * s)
{
  int i;
  instruction_t instruction;
  instruction.word[0] = s->opcode;
  instruction.word_count = 1;

  if (s->label)
    label_write(&state->label_defs, s->label,
      state->next_word - state->program->code);

  if (s->opcode == 0x0)
  {
    if (s->operand[1].type != O_NULL)
      CRASH("Non-basic opcode must have single operand.");

    switch (s->opcode_non_basic)
    {
      case OP_JSR:
        instruction.word[0] = 0;
        instruction.word[0] |= OP_JSR << OPCODE_WIDTH;
        assemble_operand(state, &instruction, &s->operand[0], 1);
        break;
      default:
        CRASH("Unhandled non-basic opcode");
    }
  }
  else
  {
    for (i = 0; i < 2; i++)
      assemble_operand(state, &instruction, &s->operand[i], i);
  }

  /* write assembled statement bytes to program. 
   UNUSED uint16_t * code = state->next_word; */
  for (i = 0; i < instruction.word_count; i++)
    *(state->next_word++) = instruction.word[i];
  state->program->length += instruction.word_count;
}

static void assemble_operand(assembler_state_t * state, instruction_t * instruction, operand_t * o, int index)
{
  int shift = OPCODE_WIDTH + (index * OPERAND_WIDTH);

  switch (o->type)
  {
    case O_REG:
    case O_INDIRECT_REG:
      instruction->word[0] |= (o->type + o->reg) << shift;
      break;
    case O_INDIRECT_NW_OFFSET:
      instruction->word[0] |= (0x10 + o->reg) << shift;
      instruction->word[instruction->word_count] = o->next_word;
      instruction->word_count++;
      break;
    case O_NW:
    case O_INDIRECT_NW:
      if (o->next_word <= OPERAND_LITERAL_MAX && !o->label)
      {
        instruction->word[0] |= (o->next_word + OPERAND_LITERAL_OFFSET) << shift;
      }
      else
      {
        instruction->word[0] |= o->type << shift;
        if (o->label)
        {
          uint16_t label_address = label_lookup_write(
            &state->label_defs,
            &state->label_refs,
            o->label,
            state->next_word - state->program->code + instruction->word_count
          );
          instruction->word[instruction->word_count] = label_address;
        }
        else
        {
          instruction->word[instruction->word_count] = o->next_word;
        }
        instruction->word_count++;
      }
      break;
    case O_POP:
    case O_PEEK:
    case O_PUSH:
    case O_SP:
    case O_PC:
    case O_O:
      instruction->word[0] |= o->type << shift;
      break;
    default:
      fprintf(stderr, "operand_to_s: %s\n", operand_to_s(o));
      fprintf(stderr, "operand type: %u 0x%02X\n", o->type, o->type);
      CRASH("unhandled operand type");
      break;
  }
}
