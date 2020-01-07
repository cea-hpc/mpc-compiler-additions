#ifndef EXTLS_INIT_H
#define EXTLS_INIT_H

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
	int is_main_bin;    /**< True if this is the main binary */
	void * dso_start;     /**< Start of the DSO address space */
	void * dso_end;       /**< End of the DSO address space */
	void * init_start;    /**< Start of the constructor array (main exe) */
	void * init_end;	  /**< End of the constructor array (main exe) */
	struct dsos_s * next; /**< the next discovered DSO in the list (if any) */
};

#endif /* EXTLS_INIT_H */