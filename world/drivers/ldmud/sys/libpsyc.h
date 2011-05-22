#ifndef LPC_LIBPSYC_H
# define LPC_LIBPSYC_H 1

# define PACKET_ROUTING 0	// 2-dimensional mapping of routing modifiers
# define PACKET_ENTITY  1	// 2-dimensional mapping of entity modifiers
# define PACKET_METHOD  2	// PSYC method string
# define PACKET_BODY    3	// packet body.. possibly binary data

// error codes returned by psyc_parse
# define PSYC_PARSE_ERROR_AMOUNT 1
# define PSYC_PARSE_ERROR_DEGREE 2
# define PSYC_PARSE_ERROR_DATE 3
# define PSYC_PARSE_ERROR_TIME 4
# define PSYC_PARSE_ERROR_FLAG 5
# define PSYC_PARSE_ERROR_LIST 6
# define PSYC_PARSE_ERROR_LIST_TOO_LARGE 7

#endif
