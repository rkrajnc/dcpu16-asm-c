#include <stdio.h>
#include <stdlib.h>

#include "assembler.h"
#include "file.h"
#include "parser.h"

#define SOURCE_PATH "fixtures/sample.s"

int main(int argc, const char ** argv)
{
  int i;
  char * source;
  parse_result_t * pr;
  program_t* program;

  source = load_file(SOURCE_PATH);
  pr = parse(source);
  free(source);

  for (i = 0; i < pr->statement_count; i++)
    print_statement(&pr->statements[i]);

  program = assemble(pr);
  printf("\nProgram:");
  for (i = 0; i < program->length; i++)
  {
    if (i % 8 == 0) printf("\n%04x:", i);
    printf(" %04x", *(program->code + i));
  }
  printf("\n");

  exit(EXIT_SUCCESS);
}
