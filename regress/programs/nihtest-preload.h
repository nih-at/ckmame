#ifndef PRELOAD_H
#define PRELOAD_H

#ifdef __APPLE__
/**
 * Get name for replacement function.
 *
 * @param name name of the function to be replaced
 * @return name of the replacement function
 */
#define PRELOAD_NAME(name) name##_replacement

/**
 * Replace a function with a preloaded version.
 *
 * This must be used after the replacement function is defined.
 *
 * @param name name of the function to be replaced
 */
#define PRELOAD_REPLACE(name)                                              \
    __attribute__((used)) static struct {                                  \
        const void* PRELOAD_NAME(name);                                    \
        const void* name;                                                  \
    } _interpose_##name __attribute__((section("__DATA,__interpose"))) = { \
        (const void*)(unsigned long)&PRELOAD_NAME(name), (const void*)(unsigned long)&name}

void* get_original_function_pointer(const char* name) {
    /* dlsym(RTLD_NEXT, name) returns the interposed function on macOS, which causes infinite recursion. */
    fprintf(stderr, "get_original_function_pointer: not implemented on macOS\n");
    abort();
}

#else /* not __APPLE__ */
/**
 * Get name for replacement function.
 *
 * @param name name of the function to be replaced
 * @return name of the replacement function
 */
#define PRELOAD_NAME(name) name

/**
 * Replace a function with a preloaded version.
 *
 * This must be used after the replacement function is defined.
 *
 * @param name name of the function to be replaced
 */
#define PRELOAD_REPLACE(name) /* nothing to do */

void* get_original_function_pointer(const char* name) {
    void* ptr = dlsym(RTLD_NEXT, name);
    if (!ptr) {
        fprintf(stderr, "can't get original function pointer for '%s': %s\n", name, dlerror());
        abort();
    }
    return ptr;
}

#endif /* not __APPLE__ */

#endif /* PRELOAD_H */
