// contains utils for getting names of syscall and other things

/* Returns string description of syscall, or NULL */
const char* get_syscall_name(long id);

/* returns string description of signal or NULL */
const char* get_signal_name(long id);

/* returns string description of ptraceevent or NULL */
const char* get_ptraceevent_name(long id);
