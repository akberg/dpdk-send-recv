#include <rte_atomic.h>

static void lcore_fwrd(rte_atomic64_t *rx_counter);

__rte_noreturn int lcore_fwrd_main(void *args);