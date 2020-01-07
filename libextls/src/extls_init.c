#define _GNU_SOURCES
#include <dlfcn.h>
#include <stdio.h>

#include "config.h"

#ifdef HAVE_LIBELF
/* For open */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <libelf.h>
#endif

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "extls.h"
#include "extls_locks.h"
#include "extls_common.h"
#include "extls_init.h"

/** head of dynamic symbol list having a initializer to be called */
static struct dyn_sym_s *_extls_dyn_symbs = NULL;
/** head of discovered executable DSOs from /proc/self/maps reading */
static struct dsos_s *_extls_dsos = NULL;
/** head of discovered private DSOs from /proc/self/maps reading */
static struct dsos_s *_extls_dsos_readonly = NULL;
/** DSO handler pointing to the current binary */
static void *__extls_tls_locate_tls_dyn_initializer_lib_handle = NULL;
/** Avoiding concurrency over dyn_init lookup */
extls_lock_t __extls_tls_locate_tls_dyn_lock = EXTLS_LOCK_INITIALIZER;



/** This function checks if an address is in the range for a given DSO
 * @param list the DSO list to be walked
 * @param name the DSO name to be checked
 * @param addr the address to validate
 */
static inline int is_in_given_dso( struct dsos_s * list, const char * name, void * addr)
{
	while(list)
	{
		if(!strcmp(list->name, name))
		{
			if( (list->dso_start <= addr)
					&&  (addr < list->dso_end))
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}

		list = list->next;
	}

	return 0;
}

static inline int is_in_readonly_dso( const char * name, void * addr )
{
	return is_in_given_dso(_extls_dsos_readonly, name, addr);
}


static inline int is_in_exec_dso( const char * name, void * addr )
{
	return is_in_given_dso(_extls_dsos, name, addr);
}

/**
 * This function is called to discover if a given variable comes with a dynamic initializer.
 * If found, the callback is called.
 *
 * @param[in] fname the functino name in string format
 */
void extls_locate_tls_dyn_initializer(char *fname)
{
	extls_lock(&__extls_tls_locate_tls_dyn_lock);

	if (!__extls_tls_locate_tls_dyn_initializer_lib_handle)
		__extls_tls_locate_tls_dyn_initializer_lib_handle = dlopen(NULL, RTLD_LAZY);

	dlerror(); /* Clear any existing error */

	void *ret = dlsym(__extls_tls_locate_tls_dyn_initializer_lib_handle, fname);

	char *error = NULL;

	if ((error = dlerror()) != NULL)
	{
		extls_unlock(&__extls_tls_locate_tls_dyn_lock);
		return;
	}

	extls_unlock(&__extls_tls_locate_tls_dyn_lock);

	/* If we found a dynamic initializer call it */
	if (ret)
	{
		extls_info("Calling Dyn initalizer for %s ret %p", fname, ret);
		void (*fn)() = (void (*)())ret;
		(fn)();
	}
}

/**
 * Read /proc/self/maps and populate the list with all discovered DSOs.
 * This allow us to know in which DSOs the initializers must be located.
 *
 * @returns 1 if any element list failed to be allocated, 0 otherwise
 */
int extls_load_proc_self_maps()
{
	char *maps = calloc(4 * 1024 * 1024, 1);

	if (!maps)
	{
		return 1;
	}

	FILE *self = fopen("/proc/self/maps", "r");

	size_t ret = fread(maps, 1, 4 * 1024 * 1024, self);

	if( ret < 0 )
	{
		return ret;
	}

	char *line = maps;
	char *next_line;

	int cont = 1;
	int current_object = 0;

	unsigned long begin, end, inode;
	char skip[50], perm[5], dev[50], dsoname[500];

	do
	{
		next_line = strstr(line, "\n");

		if (next_line)
		{
			*next_line = '\0';
			next_line++;
		}
		else
		{
			cont = 0;
		}

		if (!strstr(line, "/"))
		{
			line = next_line;
			continue;
		}

		sscanf(line, "%lx-%lx %4s %49s %49s %ld %499s", &begin, &end,
				perm, skip, dev, &inode, dsoname);

		if (!(dsoname[0] == '['))
		{
			if( (perm[2] == 'x') /* Executable */
					|| !(strcmp(perm, "r--p"))) /* Read-only */
			{

				struct dsos_s *new = malloc(sizeof(struct dsos_s));

				if (!new)
				{
					return 1;
				}

				new->name = strdup(dsoname);
				new->is_main_bin = !current_object;
				new->init_start = NULL;
				new->init_end = NULL;
				new->dso_start = (void*)begin;
				new->dso_end = (void*)end;

				if(perm[2] == 'x')
				{
					/* Executable segments */
					new->next = _extls_dsos;
					_extls_dsos = new;
				}
				else
				{
					/* Readonly segment of DSO */
					new->next = _extls_dsos_readonly;
					_extls_dsos_readonly = new;
				}
			}
		}

		current_object++;

		line = next_line;

	} while (cont);

	fclose(self);

	free(maps);

	return 0;
}

static inline int extls_wrapper_name_is_equal(char *a, char * b)
{
	/* We use the invariant that differences start
	   at byte 12 */
	int i = 0;
	do{
		if( a[i] != b[i])
			return 0;
	}while(i++, a[i] != '\0' && b[i] != '\0');

	return 1;
}


static inline int extls_add_new_symbol( char * name , void * handle)
{
	/* Make sure it is not already present */
	struct dyn_sym_s *tmp = _extls_dyn_symbs;

	while(tmp)
	{
		if( extls_wrapper_name_is_equal(tmp->name, name) )
		{
			return 1;
		}
		tmp = tmp->next;
	}

	/* Resolve the function address */
	void * resolv =  dlsym(handle, name);

	if( !resolv )
	{
		extls_warn("EXTLS : Failed to resolve dynamic initializer %s", name);
		return 1;
	}

	/* Save in symbol list */
	struct dyn_sym_s *new = malloc(sizeof(struct dyn_sym_s));

	if (!new)
	{
		return 1;
	}

	new->name = strdup(name);
	new->addr = resolv;
	new->next = _extls_dyn_symbs;
	_extls_dyn_symbs = new;

	return 0;
}


static inline void extls_load_constructors_check_range_and_insert(struct dsos_s * dso, void * init_start, void * init_end)
{
	if( init_start &&  init_end )
	{
		size_t base_offset = 0;

		if(!dso->is_main_bin)
		{
			base_offset = (size_t)dso->dso_start;
		}

		dso->init_start = init_start + base_offset;
		dso->init_end = init_end + base_offset;

		extls_info(" - CTOR/DTOR Array in  %s [BASE %p] %p %p", dso->name, dso->dso_start, init_start , init_end);

		/* Check that what we computed is actually in the RO section of this DSO */
		if(is_in_readonly_dso(dso->name, dso->init_start) && is_in_readonly_dso(dso->name, dso->init_end))
		{
			void **to_check = NULL;
			int did_fail = 0;

			if( dso->init_start != dso->init_end)
			{
				/* Make sure it is an array of pointers */
				if( ((dso->init_end - dso->init_start)%8) == 0 )
				{
					to_check = (void**)dso->init_start;

					/* Check that each pointer is in the exec */
					while(to_check <= (void **)dso->init_end )
					{
						if( !is_in_exec_dso(dso->name, *to_check) )
						{
							did_fail = 1;
							break;
						}
						to_check++;
					}
				}
				else
				{
					did_fail = 1;
				}
			}

			if( did_fail )
			{
				extls_info("CTOR/DTOR DISCARD for %s\n", dso->name);

				/* Something is wrong discard initializers */
				dso->init_start = NULL;
				dso->init_end = NULL;
			}
		}
	}
}



#ifdef HAVE_LIBELF
static inline int is_tls_wrapping_function(char * name)
{
	char expected[12] = "___mpc_TLS_w";

	int i = 0;
	do{
		if(expected[i] != name[i])
		{
			return 0;
		}
	}while( i++, (i < 12) && (name[i] != '\0'));

	return 1;
}

int extls_load_wrapper_symbols_elf(char *dso, void *handle)
{
	int ret = 0;
	int dso_fd = open(dso, O_RDONLY);

	if( dso_fd < 0 )
	{
		return EXTLS_ENKNWN;
	}

	if( elf_version(EV_CURRENT) == EV_NONE)
	{
		goto ELF_END;
	}

	Elf * elf = elf_begin(dso_fd, ELF_C_READ, (Elf *) 0);

	if(!elf)
	{
		extls_warn("%s", elf_errmsg(elf_errno()));
		goto ELF_END;
	}

	if( elf_kind(elf) !=  ELF_K_ELF )
	{
		/* This is not an ELF object */
		elf_end(elf);
		goto ELF_END;
	}

	/* Get the offset to the string section */
	size_t string_section = 0;
	if( elf_getshdrstrndx(elf, &string_section) )
	{
		elf_end(elf);
		goto ELF_END;
	}

	/* It is now time to walk sections */
	Elf_Scn *section = NULL;

	extls_info(" - Loading TLS_w symbols in %s with ELF", dso);

	/* For each section */
	while((section = elf_nextscn(elf, section)) != NULL )
	{
		Elf64_Shdr * sheader = elf64_getshdr(section);

		if(!sheader)
		{
			extls_warn("%s", elf_errmsg(elf_errno()));
			elf_end(elf);
			goto ELF_END;
		}

		if ( (sheader->sh_type != SHT_DYNSYM)
				&&   (sheader->sh_type != SHT_SYMTAB))
			continue;


		int count = sheader->sh_size / sheader->sh_entsize;

		Elf_Data *psymb = NULL;
		psymb = elf_getdata(section, psymb);

		if( !psymb )
		{
			continue;
		}

		int i;
		/* Now proceed to walk all the symbols */
		for (i = 0; i < count; i++) {
			Elf64_Sym * symb = ((Elf64_Sym*)psymb->d_buf + i);
			/* Only consider functions */
			if( ELF64_ST_TYPE(symb->st_info) == STT_FUNC )
			{
				/* With a name */
				if( symb->st_name != 0 )
				{
					/* Get Name */
					char * name = elf_strptr(elf, sheader->sh_link, symb->st_name);
					if( name )
					{
						if( is_tls_wrapping_function(name) )
						{
							extls_add_new_symbol( name , handle);
							extls_info("    * ELF found %s", name );

						}
					}
				}
			}
		}
	}
	elf_end(elf);
ELF_END:
	close(dso_fd);
	return ret;
}

int extls_load_constructors_elf(struct dsos_s * dso)
{
	int ret = 0;
	int dso_fd = open(dso->name, O_RDONLY);

	if( dso_fd < 0 )
	{
		return EXTLS_ENKNWN;
	}

	if( elf_version(EV_CURRENT) == EV_NONE)
	{
		goto CONST_ELF_END;
	}

	Elf * elf = elf_begin(dso_fd, ELF_C_READ, (Elf *) 0);

	if(!elf)
	{
		fprintf(stderr, "%s", elf_errmsg(elf_errno()));
		goto CONST_ELF_END;
	}

	if( elf_kind(elf) !=  ELF_K_ELF )
	{
		/* This is not an ELF object */
		elf_end(elf);
		goto CONST_ELF_END;
	}


	/* It is now time to walk sections */
	Elf_Scn *section = NULL;

	void * init_start = NULL;
	void * init_end = NULL;

	/* For each section */
	while((section = elf_nextscn(elf, section)) != NULL )
	{
		Elf64_Shdr * sheader = elf64_getshdr(section);

		if(!sheader)
		{
			fprintf(stderr, "%s", elf_errmsg(elf_errno()));
			elf_end(elf);
			goto CONST_ELF_END;
		}

		if (sheader->sh_type != SHT_SYMTAB)
			continue;


		int count = sheader->sh_size / sheader->sh_entsize;

		Elf_Data *psymb = NULL;
		psymb = elf_getdata(section, psymb);

		if( !psymb )
		{
			continue;
		}

		int i;
		/* Now proceed to walk all the symbols */
		for (i = 0; i < count; i++) {
			Elf64_Sym * symb = ((Elf64_Sym*)psymb->d_buf + i);
			/* Only consider functions */
			//if( ELF64_ST_TYPE(symb->st_info) == STT_NOTYPE )
			{
				/* With a name */
				if( symb->st_name != 0 )
				{
					/* Get Name */
					char * name = elf_strptr(elf, sheader->sh_link, symb->st_name);
					if( name )
					{
						//fprintf(stderr, "====> %s\n", name);
						if(!strcmp(name, "__frame_dummy_init_array_entry"))
						{
							init_start = (void *)symb->st_value;
						}
						else if(!strcmp(name, "__do_global_dtors_aux_fini_array_entry"))
						{
							init_end = (void *)symb->st_value;
						}
					}
				}
			}
		}
	}

	extls_load_constructors_check_range_and_insert(dso, init_start, init_end);

	elf_end(elf);
CONST_ELF_END:
	close(dso_fd);
	return ret;
}
#endif



/**
 * Look for all constructors generated by patched GCC in the given DSO
 * @param[in] dso the dynamic object to look up
 * @returns 1 if an error is encountered, 0 otherwise
 */
int extls_load_constructors_compat(struct dsos_s * dso, char * prefix_command)
{
	int ret = 0;


	void * init_start = NULL;
	void * init_end = NULL;

	/* because DMTCP waste too much time to track these commands */
	char command[1000];
	/* because DMTCP waste too much time to track these commands */
	char *ckpt_wrapper = (prefix_command) ? prefix_command : "";
	snprintf(command, 1000, "%s nm  %s 2>&1 | %s grep \"__frame_dummy_init_array_entry\\|__do_global_dtors_aux_fini_array_entry\"", ckpt_wrapper, dso->name, ckpt_wrapper);

	FILE *constructor_array = popen(command, "r");

	if(!constructor_array)
	{
		perror("popen");
		return 1;
	}

	while (!feof(constructor_array))
	{
		char buff[500];

		if (fgets(buff, 500, constructor_array) == 0)
		{
			break;
		}

		void * addr = NULL;

		int ret = sscanf(buff, "%lx t %*s", &addr);

		if( ret == 1 )
		{
			if( strstr(buff, "__do_global_dtors_aux_fini_array_entry") )
			{
				/* FINI */
				init_end = addr;
			}
			else if( strstr(buff, "__frame_dummy_init_array_entry") )
			{
				/* START */
				init_start = addr;

			}

		}
	}

	pclose(constructor_array);

	extls_load_constructors_check_range_and_insert(dso, init_start, init_end);

	return ret;
}



/**
 * Look for all wrapper generated by patched GCC in the given DSO
 * @param[in] dso the dynamic object to look up
 * @param[in] handle the associated handler returned by dlopen()
 * @param[in] prefix_command command to wrap the symbol resolution in compat mode (useful for DMTCP)
 * @returns 1 if an error is encountered, 0 otherwise
 */
int extls_load_wrapper_symbols_compat(char *dso, void *handle, char *prefix_command)
{
	char command[1000];

	extls_info(" - Loading TLS_w symbols in %s  with COMPAT", dso);

	/* because DMTCP waste too much time to track these commands */
	char *ckpt_wrapper = (prefix_command) ? prefix_command : "";
	snprintf(command, 1000, "%s nm  %s 2>&1 | %s grep \"___mpc_TLS_w\\|_ZTW\"", ckpt_wrapper, dso, ckpt_wrapper);

	FILE *wrapper_symbols = popen(command, "r");

	if(!wrapper_symbols)
	{
		perror("popen");
		return 1;
	}

	while (!feof(wrapper_symbols))
	{
		char buff[500];

		if (fgets(buff, 500, wrapper_symbols) == 0)
		{
			break;
		}

		char *wrapp = strstr(buff, "___mpc_TLS_w");

		if (wrapp)
		{
			char *retl = strstr(buff, "\n");

			if (retl)
			{
				*retl = '\0';

				while (retl != wrapp)
				{
					if (*(retl - 1) == ' ')
					{
						retl--;
						*retl = '\0';
					}
					else
					{
						break;
					}
				}
			}

			extls_add_new_symbol( wrapp , handle);

			extls_info("    * COMPAT found %s", wrapp );
		}
	}

	pclose(wrapper_symbols);

	return 0;
}

/**
 * Look for all wrapper generated by patched GCC in the given DSO
 * @param[in] dso the dynamic object to look up
 * @param[in] handle the associated handler returned by dlopen()
 * @param[in] prefix_command command to wrap the symbol resolution in compat mode (useful for DMTCP)
 * @returns 1 if any element list failed to be allocated, 0 otherwise
 */
int extls_load_wrapper_symbols(struct dsos_s *dso, void *handle, char *prefix_command)
{
#ifdef HAVE_LIBELF
	int ret = extls_load_wrapper_symbols_elf(dso->name, handle);

	if( ret == 0 )
	{
		/* If elf succeeded proceed to extract constructors */
		extls_load_constructors_elf(dso);
		/* If no error return */
		return ret;
	}
#endif
	/* Use the compatibility model with NM */

	/* Extract constuctors using the compat model */
	extls_load_constructors_compat(dso, prefix_command);
	/* Extract the TLS wrappers using compat model */
	return extls_load_wrapper_symbols_compat(dso->name, handle, prefix_command);
}

/**
 * Locate all dynamic initializers generated by patched GCC.
 * This function does not call them. This function should be called as early
 * as possible by MPC (probably in extls_launch.c), before any MPI task
 *
 * @returns EXTLS_ENKNWN if something's gone wrong, EXTLS_SUCCESS otherwise
 */
extls_ret_t extls_locate_dynamic_initializers(char * wrap_prefix)
{
	if (extls_load_proc_self_maps())
	{
		extls_warn("Failed to load /proc/self/maps");
		return EXTLS_ENKNWN;
	}

	struct dsos_s *current = _extls_dsos;

	void *lib_handle = dlopen(NULL, RTLD_LAZY);

	while (current)
	{
		extls_info("===== Processing DSO %s", current->name);
		if (extls_load_wrapper_symbols(current, lib_handle, wrap_prefix))
		{
			return EXTLS_ENKNWN;
		}

		current = current->next;
	}

	dlclose(lib_handle);

	return EXTLS_SUCCESS;
}

/**
 * Call all dynamic initializers previously discovered.
 * This function erases current values and replaces with the initial value
 * For instance, for task-level initialization, this function should be called
 * during task instanciation, in order to have one init copy per task
 * @returns EXTLS_ENKNWN if something's gone wrong, EXTLS_SUCCESS othewise.
 */
extls_ret_t extls_call_dynamic_initializers()
{
	struct dyn_sym_s *current = _extls_dyn_symbs;

	while (current)
	{
		if (current->addr)
		{
			extls_dbg("TLS : Calling dynamic init %s", current->name);
			((void (*)())current->addr)();
		}

		current = current->next;
	}

	return EXTLS_SUCCESS;
}

/**
 * Call constructors for stactic objects once for each thread
 * Previously discovered with extls_locate_dynamic_initializers
 */
extls_ret_t extls_call_static_constructors()
{
	struct dsos_s *current = _extls_dsos;

	while (current)
	{
		if(current->init_start && current->init_end)
		{
			/* Init array found */
			unsigned int count = (void (**)())current->init_end - (void (**)())current->init_start;
			int i;
			void (**pfunc)() = (void (**)())current->init_start;

			for( i = 0 ; i < count ; i++)
			{
				pfunc[i]();
			}
		}
		current = current->next;
	}

	return EXTLS_SUCCESS;
}
