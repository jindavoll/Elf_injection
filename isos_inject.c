#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <argp.h>
#include <err.h>
#include <fcntl.h>
#include <libelf.h>




/* This structure is used by main to communicate with parse_opt. */
struct arguments {
  char *read_elf;
  char *bin_elf;
  char *section_name;
  int address;
  bool entry;
};

/* options used to parse the arguments */
static struct argp_option options[] = {
		{"help", 'h', 0, 0, "display help option to stdout", 0},
		{"read_elf", 'r', "PATH", 0, "read the elf file to the path given in argument", 0},
		{"bin_file", 'b', "PATH", 0, "machine code to be injected", 0},
		{"create_section", 'c', "STRING", 0, "name of newly created section", 0},
		{"base_addr", 'a', "INT", 0, "base adress of injected code", 0},
		{"entry_func", 'e', 0, 0, "entry func should be modified or not", 0},
		{ 0 }
	};


static error_t parse_opt (int key, char *arg, struct argp_state *state) {
	struct arguments *arguments = state->input;

  switch (key) {
    case 'h':
      fputs("Usage: isos_inject with 5arguments\n"
      			"  -r, --read_elf PATH,          The elf that will be analyzed\n"
      			"  -b, --bin_file PATH,          A binary file that contains th machine code to be injected\n"
      			"  -c, --create_section STRING,  The name of the newly created section, must be a String\n"
      			"  -a, --base_addr INT,          The base address of the injected code\n"
      			"  -e, --entry_func BOOLEAN,     A boolean that indicates whether the entry function should be modified\n"
      			"  -h, --help,                   display this help and exit\n\n", stdout);
      exit(EXIT_SUCCESS);
    case 'r':
      arguments->read_elf = arg;
    	break;

    case 'b':
      arguments->bin_elf = arg;
    	break;

    case 'c':
      arguments->section_name = arg;
    	break;

    case 'a':
      arguments->address = atoi(arg);
    	break;

    case 'e':
      arguments->entry = true;
    	break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

/* options, parse_opt, *args_doc, *doc, *childen, *help_filter, *argp_domain */
static struct argp argp = { options, parse_opt, 0, 0, 0, 0, 0};

/* method used to check if the parsing was done all the options was used
   if something is not initialized, then an error is printed and we exit the program
   @param the arguments structure supposed to be init*/
void args_check(struct arguments *args) {
  if (args->read_elf == NULL) 
    err(EXIT_FAILURE, "the read_elf file has not been provided to the program\n");
  else if (args->bin_elf == NULL)
    err(EXIT_FAILURE, "the bin_file file has not been provided to the program\n");
  else if (args->section_name == NULL) 
    err(EXIT_FAILURE, "the section string name has not been provided to the program\n");
  else if (args->address == 0)
    err(EXIT_FAILURE, "the address has no been provided to the program\n");
}

/* take the path to an elf file and return the elf descriptor */
Elf * getElf(char * path_to_file) {
  /* open read_elf and check if he's readable*/
  int fd = open(path_to_file, O_RDONLY, 0);
  if (fd < 0)
    err(EXIT_FAILURE, "read_elf file is not readable\n");

  /* on the IBM doc we must check the elf version before using the operation offered by the libelf */
  if (elf_version(EV_CURRENT) == EV_NONE) {  
    err(EXIT_FAILURE, "libelf not up to date cannot use the operation of the libelf\n");
  }

  /* open the elf using libelf and convert it in an elf descriptor */
  Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
  if (elf == NULL)
    err(EXIT_FAILURE, "elf file is not readable, error : %s\n", elf_errmsg(elf_errno()));

  return elf;
}
/* take an elf and return an executable header of this elf if that's a 64bit-executable */
Elf64_Ehdr * get_64Ehdr(Elf *elf) {
  /* get the 64 bytes executable header */ 
  Elf64_Ehdr *ehdr = elf64_getehdr(elf);
  if (ehdr == NULL)
    err(EXIT_FAILURE, "couldn't get the 64bytes executable header : %s", elf_errmsg(elf_errno()));
  /* to check if it's 32 or 64 we can check the EI_class of the e_ident cf CM2 Mohamed website 
     0 if for invalid class, 1 = 32-bit object and 2 = 64-bit object */
  if (ehdr->e_ident[4] != 2) 
    err(EXIT_FAILURE, "elf file is not 64-bit\n");
  return ehdr;
}

int main(int argc, char **argv) {
  /* task1 */
	struct arguments arguments;
	arguments.read_elf = NULL;
	arguments.bin_elf = NULL;
	arguments.section_name = NULL;
	arguments.address = 0;
	arguments.entry = false;

  argp_parse(&argp, argc, argv, 0, 0, &arguments);
  args_check(&arguments);

  printf("read_elf %s\n", arguments.read_elf);
  
  Elf *elf = getElf(arguments.read_elf);
	Elf64_Ehdr *ehdr = get_64Ehdr(elf);
  printf("executable %d\n", ehdr->e_ident[4]);
  

	printf("good\n");
  elf_end(elf);
}