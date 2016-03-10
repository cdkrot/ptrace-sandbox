#ifndef PTSANDBOX_UTILS_ATTRIBUTES_H
#define PTSANDBOX_UTILS_ATTRIBUTES_H

/** defines many decorations for templates and other things
 *  all names in form decorated_ are reserved for names in here
 */

/**
 * Compilator yells, that "variable unused", but there are reasons to be it there ?
 * Wrap it's inside unused and problem gone
 * int do_smth(int UNUSED(cmd));
 */
#define UNUSED(NAME) decorated_unused__ ## NAME __attribute__((unused))

/**
 * For definition of internal symbols inside headers
 * #define INTERNAL(NAME) INTERNAL_BASE(MY_MODULE, NAME)
 *
 * typedef int INTERNAL(my_int);
 *
 * #undef INTERNAL
 */
#define INTERNAL_BASE(MODULE, NAME) decorated_internal__ ## MODULE ## __ ## NAME

/**
 * Template author writes:
 * #define DECLARE_TEMPLATED(my_name, TYPE) // code here
 * 
 * Template user writes (after including template header):
 * DECLARE_TEMPLATED(my_name, TYPE); // now can use templated objects.
 *
 * The purpose of this macro is to provide uniform templating interface.
 */
#define DECLARE_TEMPLATED(NAME, TYPE) decorated_declare_templated__ ## NAME ## __ ## TYPE
#define DECLARE_TEMPLATED2(NAME, TYPE1, TYPE2) DECLARE_TEMPLATED(DECLARE_TEMPLATED(NAME, TYPE1), TYPE2)
#define DECLARE_TEMPLATED3(NAME, TYPE1, TYPE2, TYPE3) DECLARE_TEMPLATED(DECLARE_TEMPLATED2(NAME, TYPE1, TYPE2), TYPE3)

/**
 * Decorates symbols, so multiple templated instance can coexist.
 * Where in C++'s you write MyClass<Some_type>
 * You should write TEMPLATED(MyClass, Some_type);
 *
 * {Not only limited to class}
 *
 * known problem:
 * Currently, will not accept types like "void*",
 * you are encouraged to use typedef names like void_p / p_void instead
 * Or provide better solution =).
 */
#define TEMPLATED(NAME, TYPE) decorated_templated__ ## NAME ## _ ## TYPE
#define TEMPLATED2(NAME, TYPE1, TYPE2) TEMPLATED(TEMPLATED(NAME, TYPE1), TYPE2)
#define TEMPLATED3(NAME, TYPE1, TYPE2, TYPE3) TEMPLATED(TEMPLATED2(NAME, TYPE1, TYPE2), TYPE3)

#endif
