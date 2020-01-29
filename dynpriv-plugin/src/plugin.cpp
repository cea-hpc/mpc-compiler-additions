#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <gcc-plugin.h>
#include <tree-pass.h>
#include <basic-block.h>
#include <tree.h>
#include <gimple.h>
#include <stringpool.h>
#include <varasm.h>
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree-inline.h"
#include "langhooks.h"
#include "flags.h"
#include "print-tree.h"
#include "cgraph.h"
#include "diagnostic.h"
#include "timevar.h"
#include "params.h"
#include "intl.h"
#include "tree-pass.h"
#include "hashtab.h"
#include "ggc.h"
#include "gimple-iterator.h"
#include "rtl.h"
#include "basic-block.h"
#include "gimple-pretty-print.h"
#include "tree-iterator.h"
#include "context.h"
#include "tree-ssa.h"
#include "plugin-version.h"

int plugin_is_GPL_compatible;


/* EXTERNAL FUNCTIONS USED */
tree fix_string_type (tree value);
tree alloc_stmt_list (void);
void append_to_statement_list_force (tree, tree *);
tree build_binary_op (location_t location, enum tree_code code,
		tree orig_op0, tree orig_op1, int convert_p);
tree build_fold_addr_expr_loc (location_t, tree);
void rest_of_decl_compilation (tree decl,  int top_level,   int at_end);
tree create_tmp_var (tree type, const char *prefix);
tree build_stmt (location_t loc, enum tree_code code, ...);

void for_each_global_decl (void (*callback) (tree decl));

/* Define plugin informations */
static struct plugin_info TLS_dyn_plugin_infos =
{
	.version = "0.1a",
	.help = "TLS Dynamic initializer generator"
};


/* 
 * Function used to track wrapped declarations in current TU
 */

/* This is a simple chained list container */
struct wrapped_tls
{
	char * name;
	tree wrapper;
	struct wrapped_tls * next;
};

static struct wrapped_tls * __wrapped_tls = NULL;

/* Create a new wrapped TLS */
struct wrapped_tls * wrapped_tls_new( char * name, tree wrapper_func )
{
	struct wrapped_tls * ret = (struct wrapped_tls *)xmalloc( sizeof( struct wrapped_tls ) );

	if( !ret )
	{
		perror("xmalloc");
		abort();
	}

	ret->name = xstrdup( name );
	ret->wrapper = wrapper_func;
	ret->next = NULL;

	return ret;
}

/* Get a wrapped TLS by name */
struct wrapped_tls * wrapped_tls_get( char * name )
{
	struct wrapped_tls * tmp = __wrapped_tls;

	while( tmp )
	{
		//printf("TEST %s == %s\n", tmp->name, name );
		if( !strcmp( tmp->name, name ) )
		{
			return tmp;
		}

		tmp = tmp->next;
	}

	return NULL;
}

/* Register a new wrapped TLS */
void wrapped_tls_push( char * name, tree wrapper_func )
{
	struct wrapped_tls * prev = wrapped_tls_get( name );

	if( prev )
	{
		fprintf(stderr,"WARNING : A wrapped TLS is being declared twice, shadowing\n");
		prev->wrapper = copy_node( wrapper_func );
		return;
	}


	struct wrapped_tls * pnew = wrapped_tls_new( name, wrapper_func );
	pnew->next = __wrapped_tls;
	__wrapped_tls = pnew;

	//printf("=== PUSHED INIT TO %s === \n", name );

}


/* 
 * UTILITIES for source code generation
 */

const char * source_file = "none";
int source_line = 0;


/** Generate a prinf gimple entry
 * @arg text content of the printf
 * @return a gimple entry for this printf
 */
gcall* printf_call_gen( char * text )
{
	gcall * printf_call;

	/* Build printf content */
	tree format_tree = fix_string_type( build_string (strlen( text ) + 1, text) );
	tree ptr_format = build_pointer_type (TREE_TYPE (TREE_TYPE (format_tree)));
	tree p_format = build1 (ADDR_EXPR, ptr_format, format_tree);

	printf_call = gimple_build_call (builtin_decl_explicit(BUILT_IN_PRINTF), 1, p_format );

	return printf_call;
}

/** Declare a new function
 * @arg Function name
 * @arg return_type Type returned by this function(tree)
 * @arg content sequence of statements to be put in the function
 * @arg is_public if set to 1 the function is not static (visible outside of TU)
 * @return the tree associated with the new function
 */
tree declare_func( const char * fname, tree ret_type, gimple_seq content, int is_public )
{
	tree name = get_identifier(fname);
	tree type = build_function_type_list (ret_type,  NULL_TREE);

	tree t = build_decl (UNKNOWN_LOCATION, RESULT_DECL, NULL_TREE, ret_type );
	tree decl = build_decl (UNKNOWN_LOCATION, FUNCTION_DECL, name, type);

	TREE_STATIC (decl) =!is_public; 
	TREE_PUBLIC (decl) =is_public; 
	TREE_USED (decl) = true;
	DECL_ARGUMENTS(decl) = NULL_TREE;
	DECL_ARTIFICIAL (decl) = 0;
	DECL_IGNORED_P (decl) = 0;
	DECL_EXTERNAL (decl) = 0;
	DECL_CONTEXT (decl) = NULL_TREE;
	DECL_NAMELESS (decl) = 0;

	DECL_INITIAL (decl) = make_node (BLOCK);
	DECL_SAVED_TREE( decl ) = NULL_TREE;
	DECL_RESULT (decl) = t;

	gbind * bind = gimple_build_bind (NULL_TREE, content, NULL);

	push_struct_function (decl);
	cfun->function_end_locus = gimple_location(content);
	init_tree_ssa(cfun);

	gimple_seq seq = NULL;

	/*=== CODE === */
	gimple_seq_add_stmt (&seq, bind);
	/*=== CODE === */
	gimple_set_body (decl, seq);
	cgraph_node::add_new_function (decl, false); 
	pop_cfun ();

	return decl;
}

/** Check if element is suspicious relatively to initializer propagation
 *  @arg t Element to be checked
 *  @return 1 if the element contains a VAR_DECL, 0 otherwise
 */
int check_for_var_deps( tree t )
{
	//printf("=> %s ==================\n", tree_code_name[ TREE_CODE( t ) ] );
	//
	//printf("========================\n");

	if( TREE_CODE( t ) == VAR_DECL )
	{
		//printf("POSSIBLE DEPENDENCY\n");
		/* Possible dependency */
		return 1;
	}

	if( TREE_CODE( t ) == CONSTRUCTOR )
	{
		unsigned int idx;
		tree elt;

		FOR_EACH_CONSTRUCTOR_VALUE (CONSTRUCTOR_ELTS (t), idx, elt)
		{
			if( elt != NULL_TREE )
				return check_for_var_deps( elt );
		}

	}
	else
	{

		int i;
		for( i= 0 ; i < TREE_OPERAND_LENGTH( t ) ; i++ )
		{
			tree next_tree = TREE_OPERAND( t, i );
			
			if( next_tree != NULL_TREE )
				return check_for_var_deps( next_tree );
		}
	}
	

	return 0;
}

/** This routine checks if an initializer is constant (wrapping it or not)
 * 	@arg var the variable to be checked
 *  @arg init the initializer associated with this variable
 *  @return 1 if constant, 0 if not
 */
int initializer_is_constant( tree var , tree init )
{
	int ret = 0;
	
	if( !init )
	{
		return 1;
	}
	
	
	const char *decl_name = gimple_decl_printable_name( var, 3 );

	if(!init)
	{
		//printf("%s No init\n", decl_name);
		ret = 0;
	}
	else
	{
		/* If it's defined in another TU, we can't tell.  */
		if( DECL_EXTERNAL (var) )
		{
			//printf("%s ANOTHER TU\n", decl_name);
			ret = 0;
		}
		else if( check_for_var_deps( init ) )
		{
			/* Detect a possible variable dependency */
			ret = 0;
		}
		else if( initializer_constant_valid_p (init, TREE_TYPE (init)) )
		{
			//printf("%s VALID INITIALIZER\n", decl_name);
			ret = 1;
		}
	
	}

	//if( !ret )
		//fprintf(stderr,"(gcc-DYNTLS) %s INVALID INITIALIZER\n", decl_name);
	
	return ret;
}


/** Hash a variable name (This is DJB2  by dan bernstein)
 * @arg str variable name to be hashed
 * @return a 64bit hash of the string
 */
static inline unsigned long hash_dbj2(char *str)
{
	unsigned long hash = 5381;
	int c;

	while (c = *str++)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}


/** Generate a wrapper function name from a variable
 * @arg decl variable to be wrapped
 * @arg force_local does the name has to be generated for local scope (hashed)
 * @return a wrapper function name matching requirements
 */
char * get_wrapper_func_name( tree decl, int force_local )
{
	static char long_name_wrapp[500];
	char variable_context[500];
	variable_context[0] = '\0';

	int public_var = TREE_PUBLIC( decl );

	const char *decl_name = gimple_decl_printable_name( decl, 3 );
	const char * current_fname = gimple_decl_printable_name( cfun->decl, 3 );

	if( (public_var == 0) || force_local  )
	{
		/* Variable is local only then mangle */
		snprintf(variable_context, 500,"%s-%s-%s-%s", decl_name,  current_fname , progname, source_file);
		unsigned long hash = hash_dbj2( variable_context );
		snprintf( long_name_wrapp, 500, "___local_mpc_TLS_w_%s_%lu", decl_name, hash );
	}
	else
	{
		snprintf( long_name_wrapp, 500, "___mpc_TLS_w_%s", decl_name );
	}

	//printf("WRAPPER IS %s (CTX '%s')\n", long_name_wrapp, variable_context );

	return long_name_wrapp;
}

static tree construct_tls_guard(const char*name, tls_model model)
{
	tree guard;

	char s[1024];
	snprintf(s, 1024, "%s_%d_%d", name, (int)model, rand());

	guard = build_decl(UNKNOWN_LOCATION, VAR_DECL, get_identifier(s), boolean_type_node);
	
	TREE_PUBLIC (guard) = 0;
	TREE_STATIC (guard) = 1;
	TREE_READONLY(guard) = 0;
	DECL_COMMON (guard) = 1;
	DECL_ARTIFICIAL (guard) = 1;
	DECL_IGNORED_P (guard) = 1;
	DECL_CONTEXT( guard ) = NULL;
	DECL_USER_ALIGN(guard) = 1;
	set_decl_tls_model (guard, model);
	DECL_INITIAL (guard) = boolean_false_node;
	make_decl_rtl(guard);
	return guard;
}

/** Create a function to propagate var dependency either locally or remotely (using DLSYM)
 * @arg var Var to be remotely loaded
 * @return a wrapper function for remote initialization
 */
void declare_per_var_getter( tree var )
{
	/* Do not wrap artificial decls */
	if( DECL_ARTIFICIAL(var) )
		return;

	gimple_seq bind_seq = NULL;
	tree type = TREE_TYPE (var);


	/* Now try to locate the dynamic initializer if present */

	char * remote_name = get_wrapper_func_name( var, 0 /* Force Local */ );

	tree wrapp_name_tree = fix_string_type( build_string (strlen( remote_name ) + 1, remote_name) );
	tree ptr_wrapp_name = build_pointer_type (TREE_TYPE (TREE_TYPE (wrapp_name_tree)));
	tree p_wrapp_name = build1 (ADDR_EXPR, ptr_wrapp_name, wrapp_name_tree);

	tree pftype = build_function_type_list( void_type_node, ptr_type_node, NULL_TREE );
	tree pfunc = build_fn_decl("extls_locate_tls_dyn_initializer", pftype );

	gcall * get_wrapp_func_call = gimple_build_call (pfunc, 1, p_wrapp_name );


	/* CREATE THE DYN INIT FLAG */
	tree init_done_val = construct_tls_guard("mpc_var_need_to_call_init", DECL_TLS_MODEL(var));

	/* Local VAR for INIT done */
	tree init_done_local_val = create_tmp_var( boolean_type_node, "tmp" );
	DECL_COMMON (init_done_local_val) = 1;
	DECL_ARTIFICIAL (init_done_local_val) = 1;
	DECL_IGNORED_P (init_done_local_val) = 1;
	DECL_CONTEXT( init_done_local_val ) = NULL;

	gassign * assign_init_done_to_local_one = gimple_build_assign (init_done_local_val, init_done_val);

	/* DONE CREATING THE FLAG */

	/* BUILD LABELS */
	tree return_val_label = create_artificial_label (UNKNOWN_LOCATION);
	tree try_load_dyn_label = create_artificial_label (UNKNOWN_LOCATION);

	/* BUILD the IF to handle dyn init presence
	 * ==> If val is 1 then we directly return as no initializer or init
	 *     already called during previous call
	 *  */
	gcond * cond_init_done = gimple_build_cond (EQ_EXPR, init_done_local_val, boolean_false_node, try_load_dyn_label, return_val_label);

	gassign * assign_one_to_init_done = gimple_build_assign (init_done_val, boolean_true_node);

	char * name = get_wrapper_func_name( var, 1 );
	char desc[150];
	snprintf(desc, 150, "calling per var local getter for %s\n", name );
	gcall* print_fname = printf_call_gen( desc );


	/*=== CODE === */
	//gimple_seq_add_stmt (&bind_seq, print_fname);
	gimple_seq_add_stmt (&bind_seq, assign_init_done_to_local_one);
	gimple_seq_add_stmt (&bind_seq, cond_init_done);
	gimple_seq_add_stmt (&bind_seq, gimple_build_label (try_load_dyn_label));
	gimple_seq_add_stmt (&bind_seq, assign_one_to_init_done);
	gimple_seq_add_stmt (&bind_seq, get_wrapp_func_call);
	gimple_seq_add_stmt (&bind_seq, gimple_build_label (return_val_label));
	/*=== CODE === */
	varpool_node::add (init_done_val);



	tree func = declare_func( name, void_type_node, bind_seq, 0  );

	/* Register the wrapper */
	char *decl_name = (char *)gimple_decl_printable_name( var, 3 );
	wrapped_tls_push( decl_name, func );
}


/** Condition check for remote variable getter generation (remote init)
 * 
 * If the symbol is remote, a dynamic remote initiazer will be generated
 * with the \ref declare_per_var_getter function
 * 
 * @arg decl Symbol to check
 */
void generate_extern_decl_getters(tree decl )
{

	char *decl_name = (char *)gimple_decl_printable_name( decl, 3 );

	if( decl == NULL_TREE )
		return;

	if( !decl_name )
		return;


	/* Only handle vars */
	if( TREE_CODE( decl ) != VAR_DECL )
		return;


	/* Only handle external vars */
	if( !DECL_EXTERNAL( decl ) )
		return;

	/* Only handle used vars */
	if( !TREE_USED( decl ) )
		return;

	/* Only handle thread local vars */
	//~ if( !DECL_THREAD_LOCAL_P( decl ) )
		//~ return;

	//printf("============> %s\n", decl_name );

	declare_per_var_getter( decl );

}

/** This function generates the local getters for non constant variables
 * 
 * Each non constant global will have one of these function (___mpc_TLS_w_*)
 * 
 * @arg var Target variable
 * @arg tu_init Reference to the per TU init function
 */
void declare_per_var_wrapper_with_tu_init( tree var, tree tu_init )
{
	/* Do not wrap artificial decls */
	if( DECL_ARTIFICIAL(var) )
		return;

	gimple_seq bind_seq = NULL;

	/* Get wrapper name */
	char * name = get_wrapper_func_name( var, 0 );

	fprintf(stderr,"(gcc-DYNTLS) Wrapping %s\n", name );

	/*
	 * BUILD THE CALL TO THE PER TU INIT
	 */
	gcall * call = gimple_build_call (tu_init, 0 );

	char desc[150];
	snprintf(desc, 150, "calling per var trampoline for %s (AFTER)\n", name );
	gcall* print_fname = printf_call_gen( desc );
	
	snprintf(desc, 150, "calling per var trampoline for %s (BEFORE)\n", name );
	gcall* print_fname_cond = printf_call_gen( desc );


	char s[128];
	snprintf(s, 128,"mpc_var_need_to_call_init");
	/* CREATE THE DYN INIT FLAG */
	tree init_done_val = construct_tls_guard(s, DECL_TLS_MODEL(var));

	/* Local VAR for INIT done */
	tree init_done_local_val = create_tmp_var( boolean_type_node, "tmp" );
	DECL_COMMON (init_done_local_val) = 1;
	DECL_ARTIFICIAL (init_done_local_val) = 1;
	DECL_IGNORED_P (init_done_local_val) = 1;
	DECL_CONTEXT( init_done_local_val ) = NULL;

	gassign * assign_init_done_to_local_one = gimple_build_assign (init_done_local_val, init_done_val);

	/* DONE CREATING THE FLAG */

	/* BUILD LABELS */
	tree return_val_label = create_artificial_label (UNKNOWN_LOCATION);
	tree try_load_dyn_label = create_artificial_label (UNKNOWN_LOCATION);

	/* BUILD the IF to handle dyn init presence
	 * ==> If val is 1 then we directly return as no initializer or init
	 *     already called during previous call
	 *  */
	gcond * cond_init_done = gimple_build_cond (EQ_EXPR, init_done_local_val, boolean_false_node, try_load_dyn_label, return_val_label);

	gassign * assign_one_to_init_done = gimple_build_assign (init_done_val, boolean_true_node);


	/*=== CODE === */
	if( getenv("MPC_DYN_PRIV_DUMP") )
		gimple_seq_add_stmt (&bind_seq, print_fname_cond);

	gimple_seq_add_stmt (&bind_seq, assign_init_done_to_local_one);
	gimple_seq_add_stmt (&bind_seq, cond_init_done);
	gimple_seq_add_stmt (&bind_seq, gimple_build_label (try_load_dyn_label));
	
	if( getenv("MPC_DYN_PRIV_DUMP") )
		gimple_seq_add_stmt (&bind_seq, print_fname);

	gimple_seq_add_stmt (&bind_seq, assign_one_to_init_done);
	gimple_seq_add_stmt (&bind_seq, call);
	gimple_seq_add_stmt (&bind_seq, gimple_build_label (return_val_label));
	/*=== CODE === */
	varpool_node::add (init_done_val);

	tree func = declare_func( name, void_type_node, bind_seq, 1 );

	/* Register the wrapper */
	char *decl_name = (char *)gimple_decl_printable_name( var, 3 );
	wrapped_tls_push( decl_name, func );
}

tree
gimple_fold_indirect_ref (tree t);

tree
build_simple_mem_ref_loc (location_t loc, tree ptr);


tree find_constant_in_compound( tree haystack )
{
	tree elt;

	if( TREE_CODE(haystack) == ARRAY_REF)
	{
		tree array = TREE_OPERAND(haystack,0);
		tree off = TREE_OPERAND(haystack,1);

		if( (TREE_CODE(array) == VAR_DECL)
		&&  (TREE_CODE(off) == INTEGER_CST) )
		{
			long ioff = int_cst_value(off);

			tree init = DECL_INITIAL(array);
			if( init )
			{
				unsigned int idx;
				tree elt;

				FOR_EACH_CONSTRUCTOR_VALUE (CONSTRUCTOR_ELTS (init), idx, elt)
				{
					if( elt != NULL_TREE ){
						if( TREE_CODE(elt) == INTEGER_CST )
						{
							if( ioff == idx )
							{
								return elt;
							}
						}
						else
						{
							return find_constant_in_compound(elt);
						}
					}
				}
			}

			return NULL_TREE;
		}

		return NULL_TREE;
	}

	if( TREE_CODE(haystack) == VAR_DECL )
	{
		tree init = DECL_INITIAL(haystack);

		if( TREE_CODE(init) == INTEGER_CST )
			return init;
		else
			return NULL_TREE;
	}

	int i;
	for( i = 0 ; i < TREE_OPERAND_LENGTH(haystack); i++)
	{
		elt = find_constant_in_compound(TREE_OPERAND(haystack, i));

		if(elt != NULL_TREE)
			return elt;
	}

	return NULL_TREE;
}



tree build_pointer_to( tree target )
{
	tree ptr_type = build_pointer_type (TREE_TYPE (target));
	tree ptr = build1 (ADDR_EXPR, ptr_type, target);

	return ptr;
}


tree unflold_pointer_plus_expr_and_build_ref(tree plus_expr)
{
	tree mem_reference = gimple_fold_indirect_ref(plus_expr);

	if( mem_reference )
	{
		/* Create a pointer to the array ref */
		tree init_ptr = build_pointer_to(mem_reference);
		return init_ptr;
	}

	/* If we are here something failed */
	return NULL_TREE;
}

/** This function generate the per TU initialization function
 *  - Assigns all variables with their non-constant value
 *  - Call the wrapper generator for non-constant variables
 */ 
void generate_per_tu_init()
{
	tree decl;
	struct varpool_node *node;

	gimple_seq* guards_seqs = NULL;

	int i;
	int mod_shift = TLS_MODEL_MPC_OPENMP;
	int nb_models = TLS_MODEL_HLS_CORE - mod_shift + 1;
	short* used_cond;

	/* create two arrays: the 'if' sequence blocks & a flag array 
	 * to check if a given 'if' contains at least one variable */
	guards_seqs = (gimple_seq*) xmalloc(sizeof(gimple_seq) * nb_models);
	used_cond = (short*)xmalloc(sizeof(short) * nb_models);

	if(!guards_seqs)
	{
		perror("xmalloc");
		abort();
	}
	memset(guards_seqs, 0, sizeof(gimple_seq) * nb_models);
	memset(used_cond, 0, sizeof(short) * nb_models);

	/* for each TLS level: insert a guard */
	for(i = 0; i < nb_models; i++)
	{
		char long_name[64];
		enum tls_model model;
		tree guard = NULL_TREE;
		tree val_guard = NULL_TREE;

		tree true_label = NULL_TREE;
		tree false_label = NULL_TREE;

		gassign * set_guard_to_1 = NULL;
		gassign * set_tmp_to_guard = NULL;

		gcond * guard_condition = NULL;

		model = (enum tls_model)(i + mod_shift);

		/* Create a temporary storing the guard */
		snprintf(long_name, 64, "tmp_guard_level_%d", (int)model );
		val_guard = create_tmp_var( boolean_type_node, long_name);
		gcc_assert(val_guard != NULL_TREE);
		DECL_COMMON (val_guard)     = 1;
		DECL_ARTIFICIAL (val_guard) = 1;
		DECL_IGNORED_P (val_guard)  = 1;
		DECL_CONTEXT( val_guard)    = NULL;

		/* Generate the GUARD */
		snprintf( long_name, 64, "mpc_tls_TU_guard_level_%d", (int) model );
		guard = construct_tls_guard(long_name, model); 
		gcc_assert(guard != NULL_TREE);
		/* DONE CREATING THE GUARD */


		/* create assigns */
		set_tmp_to_guard = gimple_build_assign (val_guard, guard);
		gcc_assert(set_tmp_to_guard);
		set_guard_to_1 = gimple_build_assign (guard, boolean_true_node);
		gcc_assert(set_guard_to_1);

		/* Implement the GUARD conditition */
		true_label      = create_artificial_label (UNKNOWN_LOCATION);
		false_label     = create_artificial_label (UNKNOWN_LOCATION);
		guard_condition = gimple_build_cond (EQ_EXPR, val_guard, boolean_false_node, true_label, false_label);

		/* DEBUG PRINT */
		char descrip[200];
		//snprintf( descrip, 200, "Calling Per TU init:%s (BEFORE) %s\n",  source_file , long_name);
		//gcall* print_fname_cond = printf_call_gen( descrip );

		snprintf( descrip, 200, "Calling Per TU init:%s (AFTER) %s\n",  source_file, long_name );
		gcall* print_fname = printf_call_gen( descrip );

		/*=== CODE ===cond */
		//if( getenv("MPC_DYN_PRIV_DUMP") )
			//gimple_seq_add_stmt (&guards_seqs[i], print_fname_cond );
		/* END DEBUG PRINT */

		/* fill the sequence:
		 * 1. Load the guard into temp var
		 * 2. Check temp var condition
		 * 3. Set the true label (= if the condition above is true)
		 * 4. Move the guard (not the temp) to 1
		 */
		gimple_seq_add_stmt (&guards_seqs[i], set_tmp_to_guard);
		gimple_seq_add_stmt (&guards_seqs[i], guard_condition);
		gimple_seq_add_stmt (&guards_seqs[i], gimple_build_label(true_label) );
		gimple_seq_add_stmt (&guards_seqs[i], set_guard_to_1 );

		/* add the guard to the known GIMPLE vars */
		varpool_node::add (guard);

		/* == DEBUG === */
		if( getenv("MPC_DYN_PRIV_DUMP") )
			gimple_seq_add_stmt (&guards_seqs[i], print_fname );
		/*============= */
	}

	/* Now, browse each TLS vars to add them to their corresponding 'if' block */
	FOR_EACH_VARIABLE (node)
	{
		tree decl       = node->decl;
		char *decl_name = (char *)gimple_decl_printable_name( decl, 3 );
		tree init       = DECL_INITIAL( decl );

		if( init )
		{
			if((TREE_CODE( init ) == POINTER_PLUS_EXPR))
			{
				/* This handle the case of pointer
				to global arrays with arithmetic

				const char* array[] = {"Hello", "World", "Dead", "Beef"};
				const char* const* slot = array + 3;
				*/
				tree resolved_init = unflold_pointer_plus_expr_and_build_ref(init);

				if( resolved_init )
				{
					init = resolved_init;
				}
				else{
					/* It could be an integer offset to the base
						const int i = 2;
						int array[3] = {0,1,2};
						int * slot = array + i;
					 */

					/* Find the OFFSET */
					tree constant_off = NULL_TREE;
					constant_off = find_constant_in_compound(TREE_OPERAND(init, 1));

					/* Rebuid with direct ref to const */
					if( constant_off ){
						long elem_off  = int_cst_value(constant_off);
						tree size = array_ref_element_size(init);
						long elem_size = int_cst_value(size);
						long off = elem_off * elem_size;

						constant_off = build_int_cst (ptr_type_node, off);

						tree ref = TREE_OPERAND(init,0);

						tree plus = fold_build_pointer_plus(ref, constant_off);

						tree new_init = unflold_pointer_plus_expr_and_build_ref(plus);

						if( new_init )
						{
							init = new_init;
						}
						else
						{
							fprintf(stderr, "Could not resolve pointer plus expr\n");
						}
					}
				}
			}
			else{
				/*
				*  Here we stabilize array references
				*  char aa[99];
				*  char * b = aa;
				*  Needs this.
				*/
				if( (TREE_CODE( init ) == NOP_EXPR ) )
				{
					tree stab = stabilize_reference ( tree_strip_nop_conversions(init) );
					init=stab;
				}
			}


		}

		if( !DECL_EXTERNAL (decl) )
		{

			int cst = initializer_is_constant( decl, init );

			/* Only initialize non-constant variables */
			if( !cst )
			{
				gassign * assign = gimple_build_assign (decl, init);
				/* retrieve the TLS model */
				enum tls_model model = DECL_TLS_MODEL(decl);
				gcc_assert(model >= TLS_MODEL_MPC_OPENMP && model <= TLS_MODEL_HLS_CORE);
				/* Save the init statement */
				gimple_seq_add_stmt (&guards_seqs[model - mod_shift], assign);
				/* tag this guard to be kept */
				used_cond[model - mod_shift] = 1;
			}
		}
	}

	/* We are done creating function body
	 * now generate the per TU init */
	gimple_seq bind_seq = NULL;
	for(i = 0; i < nb_models; i++)
	{
		/* the block is created only if the guard contains at least one initialization */
		if(used_cond[i])
		{
			/* in that case, we have something dirty to do:
			 * Appending, in each statement the false_label allowing to skip the block
			 * when the condition is false( =guard already true).
			 * So, first we have to identify which statement is the condition
			 */
			gimple_stmt_iterator gsi;

			for(gsi = gsi_start(guards_seqs[i]) ; !gsi_end_p(gsi) && gimple_code(gsi_stmt(gsi)) != GIMPLE_COND; gsi_next(&gsi));

			gcc_assert(gimple_code(gsi_stmt(gsi)) == GIMPLE_COND);

			tree false_label = gimple_cond_false_label((gcond*)gsi_stmt(gsi));
			gimple_seq_add_stmt(&guards_seqs[i], gimple_build_label(false_label));
			gimple_seq_add_seq(&bind_seq, guards_seqs[i]);
		}
	}

	/* do not insert useless tu_init functions */
	if(gimple_seq_empty_p(bind_seq))
		return;

	tree tu_func = declare_func( "mpc_TLS_per_tu_init", void_type_node, bind_seq, 0 );

	/* Now generate the wrappers */
	FOR_EACH_VARIABLE (node)
	{
		tree decl = node->decl;
		tree init = DECL_INITIAL( decl );

		int cst = initializer_is_constant( decl, init );

		/* Only generate wrappers for non-constant variables */
		if( !cst )
		{
			/* Generate a wrapper calling the init func */
			declare_per_var_wrapper_with_tu_init( decl, tu_func ); 
		}
	}

}



struct var_to_push
{
	char * name;
	gcall * call;
	struct var_to_push * next;
};


struct var_to_push * var_to_push_already_present( struct var_to_push * vars, char * name )
{
	struct var_to_push * cur = vars;
	
	while( cur )
	{
		if( !strcmp( name, cur->name ) )
			return cur;
		
		cur = cur->next;
	}
	
	return NULL;
}


void new_var_to_push( struct var_to_push ** vars, char * name, tree var )
{
	if( var_to_push_already_present( *vars, name ) )
	{
		return;
	}

	struct wrapped_tls * wtls = NULL;

	/* Lookup the symbol in the wrapped TLS table */
	wtls = wrapped_tls_get( name );

	gcall * call = NULL;


	if( wtls )
	{
		/* Already wrapped */
		//printf("SCAN : registered %s\n", name );
			
		 call = gimple_build_call( wtls->wrapper, 0 );
	}
	else
	{
		/* Not registered */
		if( DECL_EXTERNAL( var ) )
		{
			/* It is an external ref and should be registered */
			generate_extern_decl_getters( var );
			
			wtls = wrapped_tls_get( name );
			
			if( wtls == NULL )
			{
				fprintf(stderr,"(gcc-DYNTLS) Warning failed to retrieve wrapper for %s\n", name);
				return;
			}

			//printf("SCAN : registered and generated %s\n", name );
			
			call = gimple_build_call( wtls->wrapper, 0 );
			
		}
		else
		{
			/* It is not external and then is not wrapped */
			return;
		}
	}

	
	struct var_to_push * new_var = (struct var_to_push *)xmalloc( sizeof( struct var_to_push ) );
	
	if( !new_var )
	{
		perror("xmalloc");
		abort();
	}
	
	new_var->name = xstrdup( name );
	new_var->call = call;
	
	new_var->next = *vars;
	
	*vars = new_var;
}

/* Here we store all variables we encounter
 * and their trampoline */
static struct var_to_push * __all_vars = NULL;


void extract_var_ref( struct var_to_push ** vars , tree  decl, int print )
{

	if( print )
	{
		//printf("==%s=\n", tree_code_name[ TREE_CODE( decl ) ]);
		//
		//printf("......(%p)\n", decl );
		//print_generic_stmt (stdout, decl, TDF_TREE | TDF_DETAILS | TDF_VOPS );
	}

	char *name;

	tree elt;
	
	switch( TREE_CODE( decl ) )
	{
		case TREE_VEC:
		{
			int cnt;
			
			for (cnt = 0; cnt < TREE_VEC_LENGTH (decl); cnt++)
			{
				extract_var_ref( vars, TREE_VEC_ELT (decl, cnt), print );
			}
			
		}
		break;
		case VECTOR_CST:
		{
			unsigned int cnt;

			for (cnt = 0; cnt < VECTOR_CST_NELTS (decl); ++cnt)
			{
				extract_var_ref( vars, VECTOR_CST_ELT (decl, cnt), print );
			}

		}
		break;
		case TREE_LIST:
		{
			elt = decl;
			while (elt && elt != error_mark_node)
			{
				if (TREE_PURPOSE (elt))
				{
					extract_var_ref( vars, TREE_PURPOSE (elt), print );
				}
				
				extract_var_ref( vars, elt, print );
				elt = TREE_CHAIN (elt);
			}
		}
		break;
		case CONSTRUCTOR:
		{
			unsigned int idx;
	

			FOR_EACH_CONSTRUCTOR_VALUE (CONSTRUCTOR_ELTS (decl), idx, elt)
			{
				if( elt != NULL_TREE )
					extract_var_ref( vars, elt, print );
			}

			break;
		}
		case ARRAY_TYPE:
		{
			for (elt = TREE_TYPE (decl); TREE_CODE (elt) == ARRAY_TYPE; elt = TREE_TYPE (elt))
				extract_var_ref( vars, elt, print  );

		}
		case VAR_DECL:
			{				
				name  = (char *)gimple_decl_printable_name( decl, 3 );

				
				if( print )
				{
				//	printf("SCAN: seen %s\n", name );
					
				}

				if( !name )
					return;
	
				new_var_to_push( vars, name, decl );
				
			}
			break;
			default:
			{}
	}

	
	/* Recursively process tree members */
	int i;
	
	for( i = 0 ; i  < TREE_OPERAND_LENGTH( decl ) ; i++ )
	{
		tree next_tree = TREE_OPERAND( decl, i );
		
		if( next_tree != NULL_TREE )
			extract_var_ref( vars, next_tree, print  );
	}
	

}

void scan_all_vars( struct var_to_push ** vars )
{
	tree op;
	unsigned int i;
	basic_block bb;
	gimple * stmt;
	gimple_stmt_iterator gsi;

	FOR_EACH_BB_FN (bb, cfun)
	{
		for (gsi=gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi))
		{
			stmt = gsi_stmt(gsi);

			for (i=0; i<gimple_num_ops(stmt); ++i)
			{
				op = gimple_op(stmt, i);

				if( op != NULL_TREE )
				{
					extract_var_ref( vars, op, 1 );
				}
			}
		}
	}

}


void rewrite_TLS_refs()
{
	/* For each variable insert init call once */
	struct var_to_push * cur_var = NULL;
	const char *fname = gimple_decl_printable_name( cfun->decl, 3 );
	
	if( !getenv("MPC_DYN_INSERT") )
	{
		/* In the non-dyn insert mode, we only target
		 * the per-TU inti function to propagate
		 * internal init dependencies */ 
		if( strcmp( "mpc_TLS_per_tu_init", fname ) )
			return;
	}
	
	//printf("##### PROCESSING %s ========================\n", fname);
	
	scan_all_vars(&cur_var);

	/* 
	 * 
	 * Now insert calls at function start
	 * 
	 */
	
	/* Get entry */
	basic_block cur = ENTRY_BLOCK_PTR_FOR_FN(cfun);
	
	/* Move to the next block (skip entry) */
	cur = cur->next_bb;
	
	if( !cur )
	{
		/* First BB not found */
		return;
	}
	
	/* Get iterator to first entry */
	gimple_stmt_iterator gsi = gsi_start_bb(cur);

	
	while( cur_var )
	{
		//printf("INSERTING CALLL for %s\n", cur_var->name );
		gsi_insert_before(&gsi, cur_var->call, GSI_SAME_STMT);
		cur_var = cur_var->next;
	}


	//printf("DONE PROCESSING %s ========================\n", fname);

}



void dump_full_tree()
{
	const char *fname = gimple_decl_printable_name( cfun->decl, 3 );

	tree op;
	int i;
	basic_block bb;
	gimple  *stmt;
	gimple_stmt_iterator gsi;

	printf("PROCESSING %s ========================\n", fname);

	FOR_EACH_BB_FN (bb, cfun)
	{
		for (gsi=gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi))
		{
			stmt = gsi_stmt(gsi);
			debug_gimple_stmt( stmt );

			//printf("||||||\n");
			for (i=0; i<gimple_num_ops(stmt); ++i)
			{
				op = gimple_op(stmt, i);

				if( op != NULL_TREE )
				{
					//debug_tree( op );
				}
				//printf("----------\n");
			}
			//printf("||||||\n");
		}
	}

	printf("DONE PROCESSING %s ========================\n", fname);

}



int count = 0;

unsigned int TLS_dyn_exec()
{
	const char *fname = gimple_decl_printable_name( cfun->decl, 3 );

	/* Extract some context to handle locals */
	source_file = DECL_SOURCE_FILE( cfun->decl );

	/* Only call the plugin once */
	if( count < 1 )
	{
		/* Proceed to the TLS wrapping */
		generate_per_tu_init();
	}

	count++;

	rewrite_TLS_refs();
	
	if( getenv("MPC_DYN_PRIV_DUMP") )
		dump_full_tree();

	return 0;
}

/* Define a new gimple pass ==> see tree_pass.h */

void plugin_release(void *gcc_data, void *user_data)
{

}


bool TLS_dyn_gate()
{
	/* You can do some checks here
	 * to enable or disable the pass */
	if( !getenv("MPC_DYN_PRIV_ENABLED") )
	{
		fprintf(stderr,"(gcc-DYNTLS) MPC Dynamic Privatization Plugin DISABLED\n");
		return false;
	}
	
	return true;
}



const pass_data pass_data_mpc_priv_plug =
{
  GIMPLE_PASS, /* type */
  "mpc_plugin", /* name */
  OPTGROUP_NONE, /* optinfo_flags */
  TV_NONE,
  ( PROP_cfg | PROP_ssa ), /* properties_required */
  0, /* properties_provided */
  0, /* properties_destroyed */
  0, /* todo_flags_start */
  0, /* todo_flags_finish */
};

class mpc_dyn_priv_pass : public gimple_opt_pass
{
public:
  mpc_dyn_priv_pass (gcc::context * ctxt)
    : gimple_opt_pass (pass_data_mpc_priv_plug,  ctxt)
  {}

  bool gate ( function * ) { return TLS_dyn_gate(); }
  virtual unsigned int execute (function *) { return TLS_dyn_exec(); } 
};

int plugin_init (struct plugin_name_args *plugin_ctx,
		struct plugin_gcc_version *version)
{
	if( getenv("MPC_DYN_PRIV_ENABLED") )
	{
		fprintf(stderr,"(gcc-DYNTLS) Loading MPC Dynamic Privatization Plugin...\n");
	}

	if (!plugin_default_version_check (version, &gcc_version))
	{
		warning(0, G_("Mismatch version for dynpriv plugin !"));
	}

	/* Fill in new pass informations */
	struct register_pass_info new_pass;

	new_pass.pass = new mpc_dyn_priv_pass(g);
	new_pass.reference_pass_name = "cfg";
	new_pass.ref_pass_instance_number = 1;
	new_pass.pos_op = PASS_POS_INSERT_AFTER;

	/* Register the pass */
	register_callback(plugin_ctx->base_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &new_pass);

	/* Register plugin infos */
	register_callback(plugin_ctx->base_name, PLUGIN_INFO, NULL, &TLS_dyn_plugin_infos );

	/* Register a callback for cleanup */
	register_callback(plugin_ctx->base_name, PLUGIN_FINISH, plugin_release, NULL );

	return 0;
}

