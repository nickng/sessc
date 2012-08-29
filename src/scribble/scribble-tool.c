#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "st_node.h"
#include "canonicalise.h"

#include "scribble/check.h"
#include "scribble/print.h"
#include "scribble/project.h"

extern int yyparse(st_tree *tree);
extern FILE *yyin;

int main(int argc, char *argv[])
{
  int option;
  int check = 0;
  int parse = 0;
  int show_usage = 0;
  int show_version = 0;
  int verbosity_level = 0;
  char *output_file   = NULL;
  char *project_role  = NULL;
  char *scribble_file = NULL;

  while (1) {
    static struct option long_options[] = {
      {"project", required_argument, 0, 'p'},
      {"output",  required_argument, 0, 'o'},
      {"colour",  no_argument,       0,  0 },
      {"parse",   no_argument,       0, 's'},
      {"check",   no_argument,       0, 'c'},
      {"version", no_argument,       0, 'v'},
      {"verbose", no_argument,       0, 'V'},
      {"help",    no_argument,       0, 'h'},
      {0, 0, 0, 0}
    };

    int option_idx = 0;
    option = getopt_long(argc, argv, "p:o:scvVh", long_options, &option_idx);

    if (option == -1) break;

    switch (option) {
      case 0:
        if (0 == strcmp(long_options[option_idx].name, "colour")) {
          scribble_colour_mode(1);
        }
        break;
      case 'p':
        project_role = (char *)calloc(sizeof(char), strlen(optarg)+1);
        strcpy(project_role, optarg);
        break;
      case 'o':
        output_file = (char *)calloc(sizeof(char), strlen(optarg)+1);
        strcpy(output_file, optarg);
        break;
      case 's':
        parse = 1;
        break;
      case 'c':
        check = 1;
        break;
      case 'v':
        show_version = 1;
        break;
      case 'V':
        verbosity_level++;
        break;
      case 'h':
        show_usage |= 1;
        break;
    }
  }

  argc -= optind-1;
  argv[optind-1] = argv[0];
  argv += optind-1;

  if (argc < 2) {
    show_usage |= 1;
  }

  if (show_usage) {
    fprintf(stderr, "Usage: %s [--parse] [--project role] [--check] [-v] [-h] Scribble.spr\n", argv[0]);
    return EXIT_SUCCESS;
  }

  if (show_version) {
    fprintf(stderr, "scribble-tool 1.0.0\n");
    return EXIT_SUCCESS;
  }

  scribble_file = argv[1];
  yyin = fopen(scribble_file, "r");
  if (yyin == NULL) {
    perror(scribble_file);
    return EXIT_FAILURE;
  }

  st_tree *tree = st_tree_init((st_tree *)malloc(sizeof(st_tree)));
  if (0 != yyparse(tree)) {
    fprintf(stderr, "Error: Parse failed\n");
    return EXIT_FAILURE;
  }

  if (parse) {
    if (verbosity_level > 0) fprintf(stderr, "Parsed %s\n", scribble_file);
    if (verbosity_level > 1) st_tree_print(tree);
    st_node_canonicalise(tree->root);
    scribble_print(tree);
  }

  if (check) {
    if (verbosity_level > 0) fprintf(stderr, "Well-formedness check\n");
    if (scribble_check(tree) != 0) {
      fprintf(stderr, "Well-formedness checks failed! (see error messages above)\n");
      if (verbosity_level > 1) st_tree_print(tree);
      scribble_print(tree);
    }
    st_node_reset_markedflag(tree->root);
    assert(0 /* Well-formedness checks unimplemented */);
  }

  /*
  if (project_role != NULL) {
    if (verbosity_level > 0) fprintf(stderr, "Projection of %s for %s\n", scribble_file, project_role);
    st_tree *projected_tree = scribble_project(tree, project_role);
    st_node_canonicalise(projected_tree->root);
    if (verbosity_level > 1) st_tree_print(projected_tree);
    scribble_print(projected_tree);
    st_tree_free(projected_tree);
  }
  */

  st_tree_free(tree);

  return EXIT_SUCCESS;
}
