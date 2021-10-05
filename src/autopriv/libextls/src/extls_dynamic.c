#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <link.h>
#include <linux/limits.h>
#include <sys/auxv.h>

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
#include "extls_dynamic.h"

static char locate_dynamic_done = 0;
/** head of dynamic symbol list having a initializer to be called */
static struct dyn_sym_s *_extls_dyn_symbs = NULL;
/** head of discovered executable DSOs from /proc/self/maps reading */
static struct dsos_s *_extls_dsos = NULL;
/** DSO handler pointing to the current binary */
static void *__extls_tls_locate_tls_dyn_initializer_lib_handle = NULL;
/** Avoiding concurrency over dyn_init lookup */
extls_lock_t __extls_tls_locate_tls_dyn_lock = EXTLS_LOCK_INITIALIZER;



/** This function checks if an address is in the range for a given DSO
 * @param list the DSO list to be walked
 * @param name the DSO name to be checked
 * @param addr the address to validate
 */
static inline int __extls_is_in_dso(const char * name, void * addr)
{
	struct dsos_s* tmp = _extls_dsos;
	while(tmp)
	{
		if(!strcmp(tmp->name, name))
		{
			return (tmp->dso_start <= addr && addr < tmp->dso_end);
		}
		tmp = tmp->next;
	}

	return 0;
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
#include <unistd.h>
static inline char* __extls_program_path()
{
	ssize_t sz = PATH_MAX;
	static char* program = NULL;
	
	if(!program)
	{
		program = malloc(sz);
		extls_assert(program);
		sz = readlink("/proc/self/exe", program, sz);
		if(sz == -1 || sz >= PATH_MAX)
		{
			extls_fatal("Unable to get the actual program path");
		}
		/* 'man 3 readlink' clearly indicates no assumption should be made
		 * to the presence of a null-terminated character... */
		program[sz] = '\0';
	}
	return program;

}

static inline int __is_vdso(struct dl_phdr_info *info)
{
	if(strlen(info->dlpi_name))
	{
		char * vdso_names[] = {"linux-vdso.so.1",
							   "linux-gate.so.1",
							   "linux-vdso32.so.1",
							   "linux-vdso64.so.1",
							   NULL};

		int i = 0;

		while(vdso_names[i])
		{

			if(!strcmp(info->dlpi_name, vdso_names[i]))
			{
				return 1;
			}

			i++;
		}

	}
	else
	{
		/* On older libcs the VSDO could have an empty name */
		if(1 <= info->dlpi_phnum)
		{
			void * base_mod_addr = (void *)info->dlpi_addr + info->dlpi_phdr[0].p_vaddr;
			void * vdso = (void *)getauxval(AT_SYSINFO_EHDR);

			if(base_mod_addr == vdso)
			{
				return 1;
			}

		}
	}

	return 0;
}

static int __extls_lookfor_dsos(struct dl_phdr_info* info, size_t sz, void* data)
{
	static char first_dso = 1;
	UNUSED(data);
	UNUSED(sz);

	/* We skip the VDSO */
	if(__is_vdso(info))
	{
		return 0;
	}

	struct dsos_s *new = malloc(sizeof(struct dsos_s));
	new->name = (char*)info->dlpi_name;
	new->cdtor_start = NULL;
	new->cdtor_end = NULL;
	new->dso_start = (void*)info->dlpi_addr;
	new->dso_end=new->dso_start + info->dlpi_phdr[info->dlpi_phnum -1].p_vaddr + info->dlpi_phdr[info->dlpi_phnum -1].p_memsz;

	/* This is an ugly way to remember the first iteration (known to be the executable.
	 * Previously it was detected by considering an empty dlpi_name as the executable DSO.
	 * Usage showed it was a wrong assumption. */
	if(first_dso)
	{
		first_dso = 0;
		new->name = __extls_program_path();
	}
	else
	{
		/* Make sure we do not catch
		 * DSOs without name */
		if(!strlen(new->name))
		{
			extls_warn("EXTLS: a DSO with empty name was found @ %p", new->dso_start);
			free(new);
			return 0;
		}
	}

	new->next = _extls_dsos;
	_extls_dsos = new;

	return 0;
}

/**
 * Read /proc/self/maps and populate the list with all discovered DSOs.
 * This allow us to know in which DSOs the initializers must be located.
 *
 * @returns 1 if any element list failed to be allocated, 0 otherwise
 */
static inline int __extls_register_loaded_dsos()
{
	return dl_iterate_phdr(__extls_lookfor_dsos, NULL);
}

static inline int __extls_wrapper_name_is_equal(char *a, char * b)
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


static inline int __extls_add_new_symbol( char * name , void * handle)
{
	/* Make sure it is not already present */
	struct dyn_sym_s *tmp = _extls_dyn_symbs;

	while(tmp)
	{
		if( __extls_wrapper_name_is_equal(tmp->name, name) )
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

	extls_dbg("DYNAMIC: register %s (%p)", name, resolv );
	new->name = strdup(name);
	new->addr = resolv;
	new->next = _extls_dyn_symbs;
	_extls_dyn_symbs = new;

	return 0;
}


static inline void __extls_load_constructors_check_range_and_insert(struct dsos_s * dso, void * region_start, void * region_end)
{
	if( !region_start ||  !region_end )
		return;

	/* register ctor/dtor array in the DSO */
	dso->cdtor_start = (char*)region_start + (size_t)dso->dso_start;
	dso->cdtor_end = (char*)region_end + (size_t)dso->dso_start;

	extls_dbg("DYNAMIC: register CDTOR (%p-%p) from %s", dso->dso_start, dso->dso_end, (dso->name));

	/* sanity checks */
	size_t i, nbentries = ((size_t)(dso->cdtor_end - dso->cdtor_start))/sizeof(void*);
	void **to_check = (void**)dso->cdtor_start;

	int mod = ((dso->cdtor_end - dso->cdtor_start)%8);
	extls_assert( mod == 0); /* outside of assert() because of "%" sign */
	for(i = 0; to_check++, i < nbentries; i++)
	{
			
		if( !__extls_is_in_dso(dso->name, *to_check) )
		{
			extls_dbg("ctor/dtor seems to refers an address outside its DSO. Prefer to discard it.");
			dso->cdtor_start = NULL;
			dso->cdtor_end = NULL;
			break;
		}
	}
}



#ifdef HAVE_LIBELF
static inline int __extls_is_tls_wrapping_function(char * name)
{
	char expected[] = "___mpc_TLS_w";
	return ((strncmp(name, expected, strlen(expected)) == 0));
}

static inline int __extls_load_wrapper_symbols_elf(char *dso, void *handle)
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

	extls_dbg("DYNAMIC: (ELF) loading from %s", dso);

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
						if( __extls_is_tls_wrapping_function(name) )
						{
							__extls_add_new_symbol( name , handle);
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

static inline int __extls_load_constructors_elf(struct dsos_s * dso)
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
	if(init_start && init_end)
		__extls_load_constructors_check_range_and_insert(dso, init_start, init_end);

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
static inline int __extls_load_constructors_compat(struct dsos_s * dso)
{
	int ret = 0;

	/* if no name is detected, this method cannot be used.
	 * This condition is required as running 'nm' without any argument
	 * will process a potential 'a.out' file, leading to major breaks !*/
	if(!dso->name || strcmp(dso->name, "") == 0)
		return 1;

	void * cdtor_start = NULL;
	void * cdtor_end = NULL;

	char command[1000];
	snprintf(command, 1000, "nm  %s 2>&1 | grep \"__frame_dummy_init_array_entry\\|__do_global_dtors_aux_fini_array_entry\"", dso->name);

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
		int ret = sscanf(buff, "%lx t %*s", (long unsigned int*)&addr);

		if( ret == 1 )
		{
			if( strstr(buff, "__do_global_dtors_aux_fini_array_entry") )
			{
				/* FINI */
				cdtor_end = addr;
			}
			else if( strstr(buff, "__frame_dummy_init_array_entry") )
			{
				/* START */
				cdtor_start = addr;
			}
		}
	}

	pclose(constructor_array);

	if(cdtor_start && cdtor_end)
		__extls_load_constructors_check_range_and_insert(dso, cdtor_start, cdtor_end);

	return ret;
}



/**
 * Look for all wrapper generated by patched GCC in the given DSO
 * @param[in] dso the dynamic object to look up
 * @param[in] handle the associated handler returned by dlopen()
 * @param[in] prefix_command command to wrap the symbol resolution in compat mode (useful for DMTCP)
 * @returns 1 if an error is encountered, 0 otherwise
 */
static inline int __extls_load_wrapper_symbols_compat(char *dso, void *handle)
{
	char command[1000];

	snprintf(command, 1000, "nm  %s 2>&1 | grep \"___mpc_TLS_w\\|_ZTW\"", dso);

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

			__extls_add_new_symbol( wrapp , handle);
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
static inline int __extls_load_wrapper_symbols(struct dsos_s *dso, void *handle)
{
#ifdef HAVE_LIBELF
	int ret = __extls_load_wrapper_symbols_elf(dso->name, handle);

	if( ret == 0 )
	{
		/* If elf succeeded proceed to extract constructors */
		__extls_load_constructors_elf(dso);
		/* If no error return */
		return ret;
	}
#endif
	/* Use the compatibility model with NM */

	/* Extract constuctors using the compat model */
	__extls_load_constructors_compat(dso);
	/* Extract the TLS wrappers using compat model */
	return __extls_load_wrapper_symbols_compat(dso->name, handle);
}

/**
 * Locate all dynamic initializers generated by patched GCC.
 * This function does not call them. This function should be called as early
 * as possible by MPC (probably in extls_launch.c), before any MPI task.
 *
 * It needs to be called at least once
 *
 * @returns EXTLS_ENKNWN if something's gone wrong, EXTLS_SUCCESS otherwise
 */
extls_ret_t extls_locate_dynamic_initializers()
{
	static extls_lock_t init_lock = EXTLS_LOCK_INITIALIZER;
	if(!locate_dynamic_done)
	{
		extls_lock(&init_lock);
		if(!locate_dynamic_done)
		{
			locate_dynamic_done = 1;
			__extls_register_loaded_dsos();
	
			struct dsos_s *current = _extls_dsos;
			void *lib_handle = dlopen(NULL, RTLD_LAZY);

			extls_info("DYNAMIC: Locate initializers");	

			while (current)
			{
				if (__extls_load_wrapper_symbols(current, lib_handle))
				{
					return EXTLS_ENKNWN;
				}
				current = current->next;
			}

			dlclose(lib_handle);
		}
		extls_unlock(&init_lock);
	}

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
	if(!locate_dynamic_done)
		extls_locate_dynamic_initializers();

	struct dyn_sym_s *current = _extls_dyn_symbs;

	while (current)
	{
		if (current->addr)
		{
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
		if(current->cdtor_start && current->cdtor_end)
		{
			/* Init array found */
			size_t count = (void (**)())current->cdtor_end - (void (**)())current->cdtor_start;
			size_t i;
			void (**pfunc)() = (void (**)())current->cdtor_start;
			
			for( i = 0 ; i < count ; i++)
			{
				pfunc[i]();
			}
		}
		current = current->next;
	}

	return EXTLS_SUCCESS;
}
