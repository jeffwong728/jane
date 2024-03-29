/** @file
 * @brief Compatibility wrapper for unordered containers.
 */
/* Authors:
 *   Jon A. Cruz <jon@joncruz.org>
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2010 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_INK_UTIL_UNORDERED_CONTAINERS_H
#define SEEN_INK_UTIL_UNORDERED_CONTAINERS_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glibmm/ustring.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <unordered_set>
#include <unordered_map>
#define INK_UNORDERED_SET std::unordered_set
#define INK_UNORDERED_MAP std::unordered_map
#define INK_HASH std::hash

namespace std {
template <>
struct hash<Glib::ustring> {
    std::size_t operator()(Glib::ustring const &s) const {
        return hash<std::string>()(s.raw());
    }
};
} // namespace std

#else
/// Name (with namespace) of the unordered set template.
#define INK_UNORDERED_SET
/// Name (with namespace) of the unordered map template.
#define INK_UNORDERED_MAP
/// Name (with namespace) of the hash template.
#define INK_HASH

#endif

#endif // SEEN_SET_TYPES_H
/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
