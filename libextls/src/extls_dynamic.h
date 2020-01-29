#ifndef EXTLS_DYNAMIC_H
#define EXTLS_DYNAMIC_H

/**
 * Represent a dynamic symbol for the dynamic initializer interface.
 * Built as a chained list.
 */
struct dyn_sym_s
{
	char * name;              /**< symbol's name */
	void * addr;              /**< current mapped address */
	struct dyn_sym_s * next;  /**< next symbol in the list (if any) */
};

/**
 * Represents a dynamic shared object (like library object) mapped in /proc/self/maps.
 * built and used as a chained list.
 */
struct dsos_s
{
	char * name;          /**< the DSO's name */
	void * dso_start;     /**< Start of the DSO address space */
	void * dso_end;       /**< End of the DSO address space */
	void * cdtor_start;    /**< Start of the constructor array (main exe) */
	void * cdtor_end;	  /**< End of the constructor array (main exe) */
	struct dsos_s * next; /**< the next discovered DSO in the list (if any) */
};

extls_ret_t extls_locate_dynamic_initializers();
extls_ret_t extls_call_dynamic_initializers();
extls_ret_t extls_call_static_constructors(); 


#endif /* EXTLS_DYNAMIC_H */
