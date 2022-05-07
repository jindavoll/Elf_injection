#include <libelf.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <argp.h>
#include <err.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>



#define PAGE_SIZE 4096

/* This structure is used by main to communicate with parse_opt. */
struct arguments {
  char *read_elf;
  char *bin_elf;
  char *section_name;
  int address;
  bool entry;
};

struct inject_bin {
  /* index */
  int index_ptnote;
  size_t numb_pgheader;
  size_t index_shstrtab;
  size_t index_abitag;

  /* size */
  size_t elf_size; /* also the offset of the injected binary */
  size_t injected_size;

  /* program and section header to be overwrite */
  Elf64_Phdr *phdr;
  Elf64_Shdr *shdr;
  Elf64_Shdr *shdr_shstrtab;

  /* elf executable header and elf */
  Elf *elf;
  Elf64_Ehdr *ehdr;

  /* elf file descriptor*/
  FILE *fd_read;
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
Elf * getElf(int fd) {

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
    err(EXIT_FAILURE, "couldn't get the 64bytes executable header error : %s", elf_errmsg(elf_errno()));
  /* to check if it's 32 or 64 we can check the EI_class of the e_ident cf CM2 Mohamed website 
     0 if for invalid class, 1 = 32-bit object and 2 = 64-bit object */
  if (ehdr->e_ident[4] != 2) 
    err(EXIT_FAILURE, "elf file is not 64-bit\n");
  return ehdr;
}

void inject_to_end(struct arguments *arguments, struct inject_bin *inject) {
 /* open both file descritptor */
  FILE *fd_read = fopen(arguments->read_elf, "a"); //append
  if (fd_read == NULL) 
    err(EXIT_FAILURE, "file descriptor failed : error %d", errno);

  FILE *fd_injected = fopen(arguments->bin_elf, "r");
  if (fd_injected == NULL)
    err(EXIT_FAILURE, "file descriptor injected failed : error %d", errno);

  /* check elf size */
  int error = fseek(fd_read, 0L, SEEK_END); //error a faire
  if (error == -1)
    err(EXIT_FAILURE, "fseek failed : error %d", errno);
  inject->elf_size = ftell(fd_read);

  /* check injected file size */
  error = fseek(fd_injected, 0L, SEEK_END); // error a faire 
  if (error == -1)
    err(EXIT_FAILURE, "fseek failed : error %d", errno);
  inject->injected_size = ftell(fd_injected); //size of injected
  rewind(fd_injected);

  /* check the offset of both file descriptor */
  printf("nbytes read %ld\n", inject->elf_size);
  printf("nbytes inj %ld\n", inject->injected_size);

  /* append the injected binary to the end of the elf file */
  char *buffer = malloc(sizeof(char) * inject->injected_size);
  size_t reading_bytes = fread(buffer, 1, inject->injected_size, fd_injected);
  if (ferror(fd_injected) != 0 || reading_bytes == 0) /* -1 is an error */
    err(EXIT_FAILURE, "error when trying to read the injected binary %d", errno);

  /* append the injected code */
  fwrite(buffer, 1, inject->injected_size, fd_read);
  if (ferror(fd_read) != 0) 
    err(EXIT_FAILURE, "error when trying to append to the elf binary %d", errno);

  /* free */
  fclose(fd_read);
  fclose(fd_injected);
  free(buffer);

  //calcul address
  printf("adress %d \n",arguments->address );
  int delta = PAGE_SIZE - ((inject->elf_size - arguments->address) % PAGE_SIZE);
  printf("delta %d\n", delta);
  arguments->address += PAGE_SIZE - delta;
  printf("delta %d and new address %d and modulo 4096 %lu \n", delta, arguments->address ,((arguments->address - inject->elf_size) % PAGE_SIZE));
}


/* TASK5 reordering section header after modifying ABItag*/
void reorder(struct inject_bin *inject, Elf64_Shdr **tab) {

  /* reorder function */
  bool swap = false;

  /* if swap then left */
  size_t index_off_abitag = inject->index_abitag - 1;
  size_t index_shstrtab = inject->index_shstrtab; /* -1 because the tab start from 0 and the first index of the date elf doesn't have a string name so it's not copied */
  if (index_shstrtab != 0 && index_off_abitag != 0 && tab[index_off_abitag - 1]->sh_addr > tab[index_off_abitag]->sh_addr) {
    swap = true;
    for (size_t i = index_off_abitag; i > 0; i--) {
      if (tab[i]->sh_addr < tab[i - 1]->sh_addr && tab[i - 1]->sh_addr != 0) {
        Elf64_Shdr *tmp = tab[i];
        tab[i] = tab[i - 1];
        tab[i - 1] = tmp;
      }
      if (tab[i]->sh_link != 0)
          tab[i]->sh_link++; //j'augmente ou décrémente puisque index pointer différent...
    } /* else right */
  } else if (index_shstrtab != 0 && index_off_abitag != 0 && tab[index_off_abitag + 1]->sh_addr < tab[index_off_abitag]->sh_addr){
    for (size_t i = index_off_abitag; i < index_shstrtab - 1; i++) {
      swap = true;
      if (tab[i]->sh_addr > tab[i + 1]->sh_addr && tab[i + 1]->sh_addr != 0) {
        Elf64_Shdr *tmp = tab[i];
        tab[i] = tab[i + 1];
        tab[i + 1] = tmp;
      }
      if (tab[i]->sh_link != 0)
          tab[i]->sh_link--;
    }
  }
  /* swap true if section have been moved else already ordered */
  if (swap) {

    /* seek to the file descriptor offset in the elf file to overwrite */

    for (size_t i = 0; i < index_shstrtab; i++) {

      int error = fseek(inject->fd_read, (inject->ehdr->e_shoff + ((i + 1) * inject->ehdr->e_shentsize)), SEEK_SET); // je sais pas quoi mettre à la place de 0L pour
      if (error == -1)
        err(EXIT_FAILURE, "fseek failed : error %d", errno);
      
      fwrite(tab[i], 1, inject->ehdr->e_shentsize, inject->fd_read);
      if (ferror(inject->fd_read) != 0) 
        err(EXIT_FAILURE, "error when trying to rewrite the section to the elf binary %d", errno);
    }
  }
}

/* TASK 4 (write) & 5 (reorder) */
void overwrite_section_header(struct arguments *arguments, struct inject_bin *inject) {
  size_t index_shstrtab = 0;
  if (elf_getshdrstrndx(inject->elf, &index_shstrtab) == -1)
    err(EXIT_FAILURE, "error when trying to get the index of shstrtab");

  inject->index_shstrtab = index_shstrtab;

  printf("index %lu\n", index_shstrtab);

  Elf_Scn *scn = NULL;
  inject->shdr = NULL;
  Elf64_Shdr *shdr = NULL;
  char *name = NULL;
  size_t index_off_abitag = 0;

  /* task 5 reoder */
  Elf64_Shdr **tab = malloc(sizeof(Elf64_Shdr *) * index_shstrtab);
  for (size_t i = 0; i < index_shstrtab; i++)
    tab[i] = NULL;
  int index = 0;

  /* loop over all section and try to get the section which name is ".note.ABI-tag" */
  while ((scn = elf_nextscn(inject->elf, scn)) != NULL) { //check the section name
    if ((shdr = elf64_getshdr(scn)) == NULL) //get the sectionheader of referencing on the section name (index)
      err(EXIT_FAILURE, "failed to get section header");
      /* Argument elf is a descriptor for an ELF object. Argument scndx is the section index for an ELF string table. 
      Argument stroffset is the index of the desired string in the string table. */
    
    name = elf_strptr(inject->elf, index_shstrtab, shdr->sh_name); //get the name with the index of the abi tag
    if (name == NULL)
      err(EXIT_FAILURE, "failed to get name");


    printf("name of section header %s \n", name);
      
      
    if (strcmp(name, ".note.ABI-tag") == 0) {//if same name i have the offset in the shstrtab
      /* Argument stroffset is the index of the desired string in the string table. */
      shdr->sh_type = SHT_PROGBITS; /* SHT PROGBITS is defined to contain executable code */
      shdr->sh_flags = SHF_EXECINSTR | SHF_ALLOC;
      shdr->sh_addr = arguments->address; //address after the calcul
      shdr->sh_offset = inject->elf_size; //size of elf without the 15bytes
      shdr->sh_size = inject->injected_size; //size of inject (15bytes)
      shdr->sh_addralign = 16; 

      /* get the index of the section name */
      index_off_abitag = elf_ndxscn(scn);
      inject->shdr = shdr;
      inject->index_abitag = index_off_abitag;
      printf("index abit tag %ld\n", index_off_abitag);
      /* copy the modified section header */

    }
    /* get the section header shstrtab used for lab 6 */
    if (strcmp(name, ".shstrtab") == 0) {
      inject->shdr_shstrtab = shdr;            
    }
    /* add in tab the section TASK5 */
    tab[index] = shdr;
    index++;
  }
  /* call to reorder task 5 */
  reorder(inject, tab); 
  free(tab); 
}


void set_name_section(struct arguments *arguments, struct inject_bin *inject) {
  if (strlen(arguments->section_name) >= strlen(".note.ABI-tag")) {
    err(EXIT_FAILURE, "error name parameter > size of section ABI-tag");
  }

  /* task 5 rewrite */
  if (inject->shdr == NULL)
    err(EXIT_FAILURE, "failed to get inject->shdr in set_name_section");

  size_t offset_in_shstrtab_abitag = inject->shdr->sh_name;
  size_t offset_shstrtab = inject->shdr_shstrtab->sh_offset;

  /* check the offset of both section header */
  if (offset_shstrtab == 0 || offset_in_shstrtab_abitag == 0) {
    err(EXIT_FAILURE, "failed to get an offset in set_name_section");
  }
  printf("offset_shstrtab_abitag %zu\n", offset_in_shstrtab_abitag);
  printf("offset_shstrtab %zu\n", offset_shstrtab);

  size_t file_offset_to_rewrite = offset_shstrtab + offset_in_shstrtab_abitag;

  /* REWRITE */
  
  int error = fseek(inject->fd_read, file_offset_to_rewrite, SEEK_SET); // je sais pas quoi mettre à la place de 0L pour
  if (error == -1)
    err(EXIT_FAILURE, "fseek failed : error %d", errno);

  for (size_t i = strlen(arguments->section_name); i < strlen(".note.ABI-tag"); i++) {
    arguments->section_name[i] = '\0';
  }

  printf("new name = %s\n", arguments->section_name);

  fwrite(arguments->section_name, 1, strlen(".note.ABI-tag"), inject->fd_read);
  if (ferror(inject->fd_read) != 0) 
    err(EXIT_FAILURE, "error when trying to rewrite the shstrtab section to the elf binary %d", errno);
}


/* task6 */
void overwrite_program_header(struct arguments *arguments, struct inject_bin *inject) {
  inject->phdr[inject->index_ptnote].p_type = PT_LOAD;
  inject->phdr[inject->index_ptnote].p_offset = inject->elf_size;
  inject->phdr[inject->index_ptnote].p_vaddr = arguments->address;
  inject->phdr[inject->index_ptnote].p_paddr = arguments->address;
  inject->phdr[inject->index_ptnote].p_filesz = inject->injected_size;
  inject->phdr[inject->index_ptnote].p_memsz = inject->injected_size;
  inject->phdr[inject->index_ptnote].p_flags = PF_X | PF_R;
  inject->phdr[inject->index_ptnote].p_align = PAGE_SIZE;
  
  /* size where i have to write the PT_NOTE in the binary */
  size_t offset = inject->ehdr->e_phoff + (inject->index_ptnote * inject->ehdr->e_phentsize);

  printf("index %u \n", inject->index_ptnote);
  printf("offset %zu\n", offset);

  int error = fseek(inject->fd_read, offset, SEEK_SET); // je sais pas quoi mettre à la place de 0L pour
  if (error == -1)
    err(EXIT_FAILURE, "fseek failed : error %d", errno);

  /* writing */
  fwrite(&inject->phdr[inject->index_ptnote], 1, inject->ehdr->e_phentsize, inject->fd_read); /* ATTENTION dois-je mettre le '.' pour '.nouveauNom ???  sur le fwrite */
  if (ferror(inject->fd_read) != 0) 
    err(EXIT_FAILURE, "error when trying to rewrite the program header PT_NOTE to the elf binary %d", errno);
}

/* overwrite the address of the executable header with the virtual address of the program header 'PT_NOTE' */
void overwrite_entry_point(struct inject_bin *inject) {
  inject->ehdr->e_entry = inject->phdr[inject->index_ptnote].p_vaddr;
  
  int error = fseek(inject->fd_read, 0, SEEK_SET);
    if (error == -1)
      err(EXIT_FAILURE, "fseek failed : error %d", errno);

  fwrite(inject->ehdr, 1, sizeof(Elf64_Ehdr), inject->fd_read);
    if (ferror(inject->fd_read) != 0) 
      err(EXIT_FAILURE, "error when trying to rewrite the shstrtab section to the elf binary %d", errno);
} 

void hijack(struct arguments *arguments, struct inject_bin *inject) {
  Elf_Scn *scn = NULL;
  Elf64_Shdr *shdr = NULL;
  Elf64_Shdr *shdr_plt = NULL;
  char *name = NULL;
  size_t gotplt_index = 0;

  /* task 7 got.plt finder */

  while ((scn = elf_nextscn(inject->elf, scn)) != NULL) { //check the section name
    if ((shdr = elf64_getshdr(scn)) == NULL) 
      err(EXIT_FAILURE, "failed to get section header");   

    name = elf_strptr(inject->elf, inject->index_shstrtab, shdr->sh_name); //get the name with the index of the abi tag
    if (name == NULL)
      err(EXIT_FAILURE, "failed to get name");

    printf("name of section header %s \n", name);
      
    if (strcmp(name, ".got.plt") == 0) {//if same name i have the offset in the shstrtab
      gotplt_index = elf_ndxscn(scn);
      shdr_plt = shdr;

    }
  }
  printf("\nindex of .got.plt %ld\n",gotplt_index );
  if (shdr_plt != NULL) {
    printf("\nplt->sh_addr %ld\n", shdr_plt->sh_addr);

    int add_getenv_in_gotplt = 0x28; //because add is 28 in hexa 
    
    size_t offset = shdr_plt->sh_offset + add_getenv_in_gotplt;

    int error = fseek(inject->fd_read, offset, SEEK_SET);
      if (error == -1)
        err(EXIT_FAILURE, "fseek failed : error %d", errno);

    /* writing */
    fwrite(&arguments->address, 1, sizeof(unsigned long), inject->fd_read);
    if (ferror(inject->fd_read) != 0) 
      err(EXIT_FAILURE, "error when trying to rewrite the GOT entry in the .got.plt section %d", errno);
  }
}


/* --- main --- */
int main(int argc, char **argv) {
  /* declaration of variables */
  struct inject_bin inject;
  inject.index_ptnote = -1;
  inject.numb_pgheader = 0;
  inject.fd_read = NULL;

  /* task1 */
  printf("\n\n-----------------------------------\n");
  printf("Output task1\n");
	struct arguments arguments;
	arguments.read_elf = NULL;
	arguments.bin_elf = NULL;
	arguments.section_name = NULL;
	arguments.address = 0;
	arguments.entry = false;



  argp_parse(&argp, argc, argv, 0, 0, &arguments);
  args_check(&arguments);

  /* open read_elf and check if he's readable*/
  int fd = open(arguments.read_elf, O_RDONLY, 0);
  if (fd < 0)
    err(EXIT_FAILURE, "read_elf file is not readable\n");

  /* get the elf file of the elf passed in argument and check if it's a 64bit executable */
  inject.elf = getElf(fd);

	inject.ehdr = get_64Ehdr(inject.elf);
  printf("executable %d\n", inject.ehdr->e_ident[4]);

  /* Task 2 check the number of program headers that the binary contains */
  printf("\n\n-----------------------------------\n");
  printf("Output task2\n");
  if (elf_getphdrnum(inject.elf, &inject.numb_pgheader) != 0)
    err(EXIT_FAILURE, "error getting the number of program header error : %s", elf_errmsg(elf_errno()));
  printf("number of program header %lu\n", inject.numb_pgheader);


  printf("offset of first program header %ld\n", inject.ehdr->e_phoff);
  /* get the program header */
  inject.phdr = elf64_getphdr(inject.elf);
  if (inject.phdr == NULL)
    err(EXIT_FAILURE, "couldn't get the program headers error : %s", elf_errmsg(elf_errno()));

  /*  iterate over all the program header and return the index of the first  
      program header that contains PT_NOTE in their p_type */
  for (size_t i = 0; i < inject.numb_pgheader; i++) {
    if (inject.phdr[i].p_type == PT_NOTE && inject.index_ptnote == -1)
      inject.index_ptnote = i; 
  }

  if (inject.index_ptnote == -1) 
    err(EXIT_FAILURE, "couldn't get any index of PT_NOTE");

  printf("index %d\n", inject.index_ptnote);

  /* Task 3 */
  printf("\n\n-----------------------------------\n");
  printf("Output task3\n");
  printf("%s\n", arguments.bin_elf);


  FILE *fd_read = fopen(arguments.read_elf, "r+");
  if (fd_read == NULL) 
    err(EXIT_FAILURE, "file descriptor failed : error %d", errno);

  inject.fd_read = fd_read;

  /* inject the binary to the end of the elf file */
  inject_to_end(&arguments, &inject);

  /* TASK 4 */
  /* search the ABI tag and modify it */
  printf("\n\n-----------------------------------\n");
  printf("Output task4\n");
  overwrite_section_header(&arguments, &inject);

  /* TASK 5 */
  /* function to set the name of the injected section */
  printf("\n\n-----------------------------------\n");
  printf("Output task5\n");
  set_name_section(&arguments, &inject);


  /* TASK6  */
  printf("\n\n-----------------------------------\n");
  printf("Output task6\n");
  overwrite_program_header(&arguments, &inject);

  printf("before rewrite %ld\n", inject.ehdr->e_entry);
  /* TASK7 */
  printf(arguments.entry ? "true" : "false");
  if (arguments.entry) {
    printf("\n\n-----------------------------------\n");
    printf("Output task7 -- true entry point overwrite\n");
    overwrite_entry_point(&inject);
  } else {
    //hijack got entries
    printf("\n\n-----------------------------------\n");
    printf("Output task7 -- false\n");
    hijack(&arguments, &inject);
  }

  printf("\nafter rewrite %ld\n", inject.ehdr->e_entry);


  fclose(fd_read);
  close(fd);
  elf_end(inject.elf);


  return 0;
}