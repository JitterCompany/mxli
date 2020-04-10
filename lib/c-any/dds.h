#ifndef __dds_h
#define __dds_h

/** DDS control parameters.
 */
struct DdsTuning256 {
	int	w;	///< frequency tuning word
	int	p;	///< phase shift
	int	a256;	///< amplitude in 1/256 units
};

typedef struct DdsTuning256 DdsTuning256;

/** DDS control parameters.
 */
struct DdsTuning {
	int	w;	///< frequency tuning word
	int	p;	///< phase shift
	int	a_e10;	///< amplitude in 1/256 units
};

typedef struct DdsTuning DdsTuning;

#endif

