/*
   FALCON - The Falcon Programming Language.
   FILE: itemlist.h

   List of Falcon Items
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: 2007-12-01
   Last modified because:

   -------------------------------------------------------------------
   (C) Copyright 2004: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
   In order to use this file in its compiled form, this source or
   part of it you have to read, understand and accept the conditions
   that are stated in the LICENSE file that comes boundled with this
   package.
*/

/** \file
   List of Falcon Items definition
*/

#ifndef flc_itemlist_H
#define flc_itemlist_H

#include <falcon/setup.h>
#include <falcon/basealloc.h>
#include <falcon/userdata.h>
#include <falcon/sequence.h>
#include <falcon/item.h>
#include <falcon/citerator.h>

namespace Falcon {

class ItemListIterator;
class ItemListElement;
class ItemList;

/** Element of a standard list of Falcon items. */
class FALCON_DYN_CLASS ItemListElement: public BaseAlloc
{
   Item m_item;

   ItemListElement *m_next;
   ItemListElement *m_prev;

public:

   /** Create the element by copying an item.
      The item is shallow copied.
   */
   ItemListElement( const Item &itm, ItemListElement *p = 0, ItemListElement *n = 0 ):
      m_next( n ),
      m_prev( p ),
      m_item( itm )
   {}

   /** Deletes the element.
      Any iterator pointing to this element is invalidated.
   */
   ~ItemListElement()
   {
   }

   const Item &item() const { return m_item; }
   Item &item() { return m_item; }

   void next( ItemListElement *n ) { m_next = n; }
   ItemListElement *next() const { return m_next; }

   void prev( ItemListElement *p ) { m_prev = p; }
   ItemListElement *prev() const { return m_prev; }

};


class FALCON_DYN_CLASS ItemListIterator: public CoreIterator
{
   ItemList *m_owner;
   ItemListElement *m_element;

   ItemListIterator *m_next;
   ItemListIterator *m_prev;

   friend class ItemList;
public:

   ItemListIterator( ItemList *owner, ItemListElement *elem );
   ~ItemListIterator();

   virtual bool next();
   virtual bool prev();
   virtual bool hasNext() const;
   virtual bool hasPrev() const;

   virtual Item &getCurrent() const;

   virtual bool isValid() const;
   virtual bool isOwner( void *collection ) const;
   virtual bool equal( const CoreIterator &other ) const;
   virtual bool erase();
   virtual bool insert( const Item &other );

   virtual void invalidate();

   virtual UserData *clone();

   // specific interface
   ItemListElement *getCurrentElement() const { return m_element; }
   void setCurrentElement( ItemListElement *e );
};


/** List of Falcon items.
   This class is designed to work together with Falcon object
   as a UserData, but it can be also used for other reasons,
   when an Array is not the best way to represent data.
*/

class FALCON_DYN_CLASS ItemList: public Sequence
{
private:
   uint32 m_size;
   ItemListElement *m_head;
   ItemListElement *m_tail;

   ItemListIterator *m_iters;
   void addIterator( ItemListIterator *iter );
   void removeIterator( ItemListIterator *iter );
   void notifyDeletion( ItemListElement *elem );

   friend class ItemListIterator;

public:
   /** Builds an empty list. */
   ItemList():
      m_size(0),
      m_head(0),
      m_tail(0),
      m_iters(0)
   {}

   /** Clones a list. */
   ItemList( const ItemList &l );

   ~ItemList() {
      clear();
   }

   /** Deletes the list.
      Items are shallowly destroyed.
   */
   virtual UserData *clone() const;

   /** Gets the first item in the list.
      If the list is empty, you will crash, so use this only when the list is
      NOT empty.
      \return a reference to the first item in the list or a spectacular crash.
   */
   const Item &front() const;

   /** Gets the last item in the list.
      If the list is empty, you will crash, so use this only when the list is
      NOT empty.
      \return a reference to the last item in the list or a spectacular crash.
   */
   const Item &back() const;

   /** Gets the pointer to the first element for list traversal.
      The list element is just an item with previous and next pointers.
      If the list is empty, this method will return 0.
      \return the pointer to the first element pointer, or 0.
   */
   ItemListElement *first() const;

   /** Gets the pointer to the last element for list traversal.
      The list element is just an item with previous and next pointers.
      If the list is empty, this method will return 0.
      \return the pointer to the last element pointer, or 0.
   */
   ItemListElement *last() const;

   /** Creates an iterator item for the object.
      The ListIterator is an instance of the CoreIterator class and can be used
      as a part of the VM iterator system.
      This method returns a newly created ListIterator pointing to the first or
      last element of the list (depending on the tail parameter).
      If the list is empty, the returned iterator will be created invalid.

      \param tail true to get the iterator to the last element in the list.
      \return a newly created iterator.
   */
   virtual ItemListIterator *getIterator( bool tail=false );

   /** Pushes a shallow copy of the item to the end of the list.
      \param itm the item to be pushed.
   */
   void push_back( const Item &itm );

   /** Removes the last element from the list.
      The item is shallowly removed. Deep content will be reclaimed through GC.
      Calling pop_back() on an empty list will have no effect.
   */
   void pop_back();

   /** Pushes a shallow copy of the item in front of the list.
      \param itm the item to be pushed.
   */
   void push_front( const Item &itm );

   /** Removes the first element from the list.
      The item is shallowly removed. Deep content will be reclaimed by GC.
      Calling pop_front() on an empty list will have no effect.
   */
   void pop_front();

   /** Removes all the elements in the list. */
   virtual void clear();

   /** Remove given element.
      If this is the last element of the list, the method returns 0,
      else it return the element that was following the delete element
      in the list, and that now has its place.
      \param elem an element from this list (or you'll witness psychedelic crashes)
   */
   ItemListElement *erase( ItemListElement *elem );

   /** Implementing sequence interface */
   virtual bool erase( CoreIterator *iter );

   /** Insert an item after given before given element.
      To insert an item past the last element, use 0 as element pointer (last->next);
      this will work also to insert an item in an empty list.

      \param elem the element before which to insert the item, or 0 to apped at tail.
      \param item the item to be inserted.
   */
   void insert( ItemListElement *elem, const Item &item );

   /** Implementing sequence interface */
   virtual bool insert( CoreIterator *iter, const Item &item );

   /** Tells if the list is empty.
      \return true if the list is empty.
   */
   virtual bool empty() const { return m_size == 0; }

   /** Return the number of the items in the list.
      \return count of items in the list
   */
   uint32 size() const { return m_size; }

   /** Perform marking of items stored in the list.
   */
   virtual void gcMark( MemPool *mp );
};


}

#endif

/* end of itemlist.h */