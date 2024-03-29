#ifndef _IO_GADGET4_H_
#define _IO_GADGET4_H_
#ifdef ENABLE_HDF5
#include <stdint.h>
#include "../particle.h"

void load_particles_gadget4(char *filename, struct particle **p, int64_t *num_p);

#endif /* ENABLE_HDF5 */
#endif /* _IO_GADGET4_H_ */
