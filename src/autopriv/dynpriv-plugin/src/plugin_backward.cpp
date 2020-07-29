#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <gcc-plugin.h>
#include <tree-pass.h>
#include <basic-block.h>
#include <gimple.h>

#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "tree-inline.h"
#include "langhooks.h"
#include "flags.h"
#include "cgraph.h"
#include "diagnostic.h"
#include "timevar.h"
#include "params.h"
#include "intl.h"
#include "tree-pass.h"
#include "hashtab.h"
#include "ggc.h"
#include "tree-flow.h"
#include "rtl.h"
#include "ipa-prop.h"
#include "basic-block.h"
#include "gimple-pretty-print.h"
#include "tree-iterator.h"

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
gimple printf_call_gen( char * text )
{
	gimple printf_call;

	/* Build printf content */
	tree format_tree = fix_string_type( build_string (strlen( text ) + 1, text) );
	tree ptr_format = build_pointer_type (TREE_TYPE (TREE_TYPE (format_tree)));
	tree p_format = build1 (ADDR_EXPR, ptr_format, format_tree);

	printf_call = gimple_build_call (builtin_decl_explicit(BUILT_IN_PRINTF), 1, p_format );
}

/** Declare a new function
 * @arg Function name
 * @arg return_type Type returned by this function(tree)
 * @arg content sequence of statements to be put in the function
 * @arg is_public if set to 1 the function is not static (visible outside of TU)
 * @return the tree associated with the new function
 */
tree declare_func( char * fname, tree ret_type, gimple_seq content, int is_public )
{
	tree name = get_identifier(fname);
	tree type = build_function_type_list (ret_type,  NULL_TREE);

	tree t = build_decl (UNKNOWN_LOCATION, RESULT_DECL, NULL_TREE, ret_type );
	tree decl = build_decl (UNKNOWN_LOCATION, FUNCTION_DECL, name, type);

	TREE_STATIC (decl) = !is_public;
	TREE_PUBLIC (decl) = is_public;
	TREE_USED (decl) = true;
	DECL_ARTIFICIAL (decl) = 0;
	DECL_IGNORED_P (decl) = 0;
	DECL_EXTERNAL (decl) = 0;
	DECL_CONTEXT (decl) = NULL_TREE;
	DECL_NAMELESS (decl) = 0;

	DECL_INITIAL (decl) = make_node (BLOCK);
	DECL_SAVED_TREE( decl ) = NULL_TREE;
	DECL_RESULT (decl) = t;


	gimple bind = gimple_build_bind (NULL_TREE, content, NULL);

	push_struct_function (decl);
	cfun->function_end_locus = UNKNOWN_LOCATION;
	pop_cfun ();

	struct function *child_cfun = DECL_STRUCT_FUNCTION (decl);

	gimple_seq seq = NULL;

	push_cfun (child_cfun);
	/*=== CODE === */
	gimple_seq_add_stmt (&seq, bind);
	/*=== CODE === */
	gimple_set_body (decl, seq);
	pop_cfun ();
	cgraph_add_new_function (decl, false); 

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

	if( !ret )
		fprintf(stderr,"(gcc-DYNTLS) %s INVALID INITIALIZER\n", decl_name);
	
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
		snprintf( long_name_wrapp, 500, "___mpc_TLS_w_%s_local_%lu", decl_name, hash );
	}
	else
	{
		snprintf( long_name_wrapp, 500, "___mpc_TLS_w_%s", decl_name );
	}

	//printf("WRAPPER IS %s (CTX '%s')\n", long_name_wrapp, variable_context );

	return long_name_wrapp;
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

	gimple get_wrapp_func_call = gimple_build_call (pfunc, 1, p_wrapp_name );


	/* CREATE THE DYN INIT FLAG */
	tree init_done_val = create_tmp_var( integer_type_node, "mpc_var_need_to_call_init" );

	TREE_PUBLIC (init_done_val) = 0;
	TREE_STATIC (init_done_val) = 1;
	DECL_COMMON (init_done_val) = 1;
	DECL_ARTIFICIAL (init_done_val) = 1;
	DECL_IGNORED_P (init_done_val) = 1;
	DECL_CONTEXT( init_done_val ) = NULL;
	DECL_TLS_MODEL (init_done_val) = TLS_MODEL_MPC_TASK;
	DECL_INITIAL (init_done_val) = integer_zero_node;

	varpool_finalize_decl (init_done_val);


	/* Local VAR for INIT done */
	tree init_done_local_val = create_tmp_var( integer_type_node, "tmp" );
	DECL_COMMON (init_done_local_val) = 1;
	DECL_ARTIFICIAL (init_done_local_val) = 1;
	DECL_IGNORED_P (init_done_local_val) = 1;
	DECL_CONTEXT( init_done_local_val ) = NULL;

	gimple assign_init_done_to_local_one = gimple_build_assign_stat (init_done_local_val, init_done_val);

	/* DONE CREATING THE FLAG */

	/* BUILD LABELS */
	tree return_val_label = create_artificial_label (UNKNOWN_LOCATION);
	tree try_load_dyn_label = create_artificial_label (UNKNOWN_LOCATION);

	/* BUILD the IF to handle dyn init presence
	 * ==> If val is 1 then we directly return as no initializer or init
	 *     already called during previous call
	 *  */
	gimple cond_init_done = gimple_build_cond (EQ_EXPR, init_done_local_val, integer_one_node, return_val_label, try_load_dyn_label);

	gimple assign_one_to_init_done = gimple_build_assign_stat (init_done_val, integer_one_node);

	char * name = get_wrapper_func_name( var, 1 );
	char desc[150];
	snprintf(desc, 150, "calling per var local getter for %s\n", name );
	gimple print_fname = printf_call_gen( desc );


	/*=== CODE === */
	//gimple_seq_add_stmt (&bind_seq, print_fname);
	gimple_seq_add_stmt (&bind_seq, assign_init_done_to_local_one);
	gimple_seq_add_stmt (&bind_seq, cond_init_done);
	gimple_seq_add_stmt (&bind_seq, gimple_build_label (try_load_dyn_label));
	gimple_seq_add_stmt (&bind_seq, assign_one_to_init_done);
	gimple_seq_add_stmt (&bind_seq, get_wrapp_func_call);
	gimple_seq_add_stmt (&bind_seq, gimple_build_label (return_val_label));
	/*=== CODE === */



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
	gimple call = gimple_build_call (tu_init, 0 );

	char desc[150];
	snprintf(desc, 150, "calling per var trampoline for %s\n", name );
	gimple print_fname = printf_call_gen( desc );


	/* CREATE THE DYN INIT FLAG */
	tree init_done_val = create_tmp_var( integer_type_node, "mpc_var_need_to_call_init" );

	TREE_PUBLIC (init_done_val) = 0;
	TREE_STATIC (init_done_val) = 1;
	DECL_COMMON (init_done_val) = 1;
	DECL_ARTIFICIAL (init_done_val) = 1;
	DECL_IGNORED_P (init_done_val) = 1;
	DECL_CONTEXT( init_done_val ) = NULL;
	DECL_TLS_MODEL (init_done_val) = TLS_MODEL_MPC_TASK;
	DECL_INITIAL (init_done_val) = integer_zero_node;

	varpool_finalize_decl (init_done_val);


	/* Local VAR for INIT done */
	tree init_done_local_val = create_tmp_var( integer_type_node, "tmp" );
	DECL_COMMON (init_done_local_val) = 1;
	DECL_ARTIFICIAL (init_done_local_val) = 1;
	DECL_IGNORED_P (init_done_local_val) = 1;
	DECL_CONTEXT( init_done_local_val ) = NULL;

	gimple assign_init_done_to_local_one = gimple_build_assign_stat (init_done_local_val, init_done_val);

	/* DONE CREATING THE FLAG */

	/* BUILD LABELS */
	tree return_val_label = create_artificial_label (UNKNOWN_LOCATION);
	tree try_load_dyn_label = create_artificial_label (UNKNOWN_LOCATION);

	/* BUILD the IF to handle dyn init presence
	 * ==> If val is 1 then we directly return as no initializer or init
	 *     already called during previous call
	 *  */
	gimple cond_init_done = gimple_build_cond (EQ_EXPR, init_done_local_val, integer_one_node, return_val_label, try_load_dyn_label);

	gimple assign_one_to_init_done = gimple_build_assign_stat (init_done_val, integer_one_node);


	/*=== CODE === */
	if( getenv("MPC_DYN_PRIV_DEBUG") )
		gimple_seq_add_stmt (&bind_seq, print_fname);

	gimple_seq_add_stmt (&bind_seq, assign_init_done_to_local_one);
	gimple_seq_add_stmt (&bind_seq, cond_init_done);
	gimple_seq_add_stmt (&bind_seq, gimple_build_label (try_load_dyn_label));
	gimple_seq_add_stmt (&bind_seq, assign_one_to_init_done);
	gimple_seq_add_stmt (&bind_seq, call);
	gimple_seq_add_stmt (&bind_seq, gimple_build_label (return_val_label));
	/*=== CODE === */

	tree func = declare_func( name, void_type_node, bind_seq, 1 );

	/* Register the wrapper */
	char *decl_name = (char *)gimple_decl_printable_name( var, 3 );
	wrapped_tls_push( decl_name, func );
}


/** This function generate the per TU initialization function
 *  - Assigns all variables with their non-constant value
 *  - Call the wrapper generator for non-constant variables
 */ 
void generate_per_tu_init()
{
	tree decl;
	struct varpool_node *node;

	char long_name[500];

	gimple_seq bind_seq = NULL;

	/* Generate the GUARD */
	snprintf( long_name, 500, "mpc_tls_TU_guard" );

	tree guard = create_tmp_var( integer_type_node, long_name );

	TREE_PUBLIC (guard) = 0;
	TREE_STATIC (guard) = 1;
	DECL_COMMON (guard) = 1;
	DECL_ARTIFICIAL (guard) = 1;
	DECL_IGNORED_P (guard) = 1;
	DECL_CONTEXT( guard ) = NULL;
	DECL_TLS_MODEL (guard) = TLS_MODEL_MPC_TASK;
	DECL_INITIAL (guard) = integer_zero_node;

	varpool_finalize_decl (guard);
	/* DONE CREATING THE GUARD */

	/* Create a temporary storing the guard */
	tree val_guard = create_tmp_var( integer_type_node, "tmp" );

	DECL_COMMON (val_guard) = 1;
	DECL_ARTIFICIAL (val_guard) = 1;
	DECL_IGNORED_P (val_guard) = 1;
	DECL_CONTEXT( val_guard ) = NULL;

	gimple assign_guard_to_tmp = gimple_build_assign_stat (val_guard, guard);

	/* The assignment of "1" to guard */
	gimple assign_1_to_guard = gimple_build_assign_stat (guard, integer_one_node);

	/* Implement the GUARD IF */
	tree tlabel = create_artificial_label (UNKNOWN_LOCATION);
	tree flabel = create_artificial_label (UNKNOWN_LOCATION);

	gimple cond = gimple_build_cond (EQ_EXPR, val_guard, integer_zero_node, tlabel, flabel);

	/* DEBUG PRINT */
	char descrip[200];
	snprintf( descrip, 200, "Calling Per TU init:%s\n",  source_file );
	gimple print_fname = printf_call_gen( descrip );
	gimple print_done_init = printf_call_gen( "INIT_DONE\n" );

	/*=== CODE === */
	gimple_seq_add_stmt (&bind_seq, assign_guard_to_tmp);
	//gimple_seq_add_stmt (&bind_seq, print_fname );
	gimple_seq_add_stmt (&bind_seq, cond);
	gimple_seq_add_stmt (&bind_seq, gimple_build_label(tlabel) );
	gimple_seq_add_stmt (&bind_seq, assign_1_to_guard );
	/*=== CODE === */

	gimple_seq init_seq = NULL;
	FOR_EACH_VARIABLE (node)
	{
		tree decl = node->symbol.decl;

		char *decl_name = (char *)gimple_decl_printable_name( decl, 3 );

		//if( !DECL_THREAD_LOCAL_P( decl ) )
		//	continue;

		tree init = DECL_INITIAL( decl );

		if( init )
		{
			/*
			 *  Here we stabilize array references
			 *  char aa[99];
			 *  char * b = aa;
			 *  Needs this.
			 */
			if( TREE_CODE( init ) == NOP_EXPR )
			{
				tree stab = stabilize_reference ( tree_strip_nop_conversions(init) );
				init=stab;
			}
		}

		if( !DECL_EXTERNAL (decl) )
		{

			int cst = initializer_is_constant( decl, init );

			/* Only initialize non-constant variables */
			if( !cst )
			{
				gimple assign = gimple_build_assign_stat (decl, init);

				/* Save the init statement */
				/*=== CODE === */
				gimple_seq_add_stmt (&init_seq, assign);
				/*=== CODE === */
			}
		}
	}

	/*=== CODE === */
	gimple_seq_add_seq( &bind_seq, init_seq ); /* Concatenate init list */
	//gimple_seq_add_stmt (&bind_seq, print_done_init);
	gimple_seq_add_stmt (&bind_seq, gimple_build_label(flabel) );
	/*=== CODE === */

	/* We are done creating function body
	 * now generate the per TU init */
	tree tu_func = declare_func( "mpc_TLS_per_tu_init", void_type_node, bind_seq, 0 );

	/* Now generate the wrappers */
	FOR_EACH_VARIABLE (node)
	{
		tree decl = node->symbol.decl;
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
	gimple call;
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

	gimple call = NULL;


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
	gimple stmt;
	gimple_stmt_iterator gsi;

	FOR_EACH_BB (bb)
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
	
	if( !getenv("MPC_DYN_PRIV_INSERT") )
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
	basic_block cur = ENTRY_BLOCK_PTR;
	
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
	gimple stmt;
	gimple_stmt_iterator gsi;

	printf("PROCESSING %s ========================\n", fname);

	FOR_EACH_BB (bb)
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
static struct opt_pass dyn_tls_pass;

void plugin_release(void *gcc_data, void *user_data)
{

}


bool TLS_dyn_gate()
{
	/* You can do some checks here
	 * to enable or disable the pass */
	if( !getenv("MPC_DYN_PRIV_ENABLED") )
	{
		return false;
	}
	 
	return true;
}


int plugin_init (struct plugin_name_args *plugin_ctx,
		struct plugin_gcc_version *version)
{
	if( getenv("MPC_DYN_PRIV_ENABLED") )
	{
		fprintf(stderr,"(gcc-DYNTLS) Loading MPC Dynamic Privatization Plugin...\n");
	}
	
	
	/* Check GCC version */
	if( strncmp( version->basever, "4.7", strlen("4.7") ) ) 
	{
		if( strncmp( version->basever, "4.6", strlen( "4.6" ) ) )
		{
			if( strncmp( version->basever, "4.8", strlen( "4.8" ) ) )
			{
				fprintf(stderr,"(gcc-DYNTLS) Error bad GCC version (%s) instead of 4.{6,7,8}.*\n",
						version->basever);
				return -1;
			}
		}
	}

	/* Fill in new pass informations */
	struct register_pass_info new_pass;

	dyn_tls_pass.type = GIMPLE_PASS;
	dyn_tls_pass.name = "TLS dyn initializer privatization"; 
	dyn_tls_pass.gate = TLS_dyn_gate;
	dyn_tls_pass.execute = TLS_dyn_exec;

	new_pass.pass = &dyn_tls_pass;
	new_pass.reference_pass_name = "cfg";
	new_pass.ref_pass_instance_number = 1;
	new_pass.pos_op = PASS_POS_INSERT_AFTER;

	/* Register the pass */
	register_callback(plugin_ctx->base_name, PLUGIN_PASS_MANAGER_SETUP,
			NULL,  &new_pass );

	/* Register plugin infos */
	register_callback(plugin_ctx->base_name, PLUGIN_INFO, NULL, &TLS_dyn_plugin_infos );

	/* Register a callback for cleanup */
	register_callback(plugin_ctx->base_name, PLUGIN_FINISH, plugin_release, NULL );

	return 0;
}

