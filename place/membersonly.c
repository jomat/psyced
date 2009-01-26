/* owned room which is accessible by members only
 * the members are managed as aides of the room */

#define PLACE_OWNED ADMINISTRATORS

// TODO: could use a simplification...
#ifdef PLACE_OWNED
#define REQUEST_ENTER return objectp(source) && qOwner(source -> qName()) || qAide(source);
#endif

#include <place.gen>
