#ifndef GDK_PIXBUF_HPP
#define GDK_PIXBUF_HPP

#include "modgtk.hpp"

#include <gdk-pixbuf/gdk-pixbuf.h>

#define GET_PIXBUF( item ) \
        (((Gdk::Pixbuf*) (item).asObjectSafe())->getObject())


namespace Falcon {
namespace Gdk {

/**
 *  \class Falcon::Gdk::Pixbuf
 */
class Pixbuf
    :
    public Gtk::CoreGObject
{
public:

    Pixbuf( const Falcon::CoreClass*, const GdkPixbuf* = 0 );

    static Falcon::CoreObject* factory( const Falcon::CoreClass*, void*, bool );

    static void modInit( Falcon::Module* );

    GdkPixbuf* getObject() const { return (GdkPixbuf*) m_obj; }

    static FALCON_FUNC version( VMARG );

    //static FALCON_FUNC init( VMARG );

};


} // Gdk
} // Falcon

#endif // !GDK_PIXBUF_HPP