/*
   FALCON - The Falcon Programming Language.
   FILE: itemarray.cpp

   Basic item array structure (sequence).
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Mon, 27 Jul 2009 20:48:24 +0200

   -------------------------------------------------------------------
   (C) Copyright 2009: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/

/** \file
   Implementation of core arrays.
   Core arrays are meant to hold item pointers.
*/

#include <falcon/carray.h>
#include <falcon/memory.h>
#include <falcon/vm.h>
#include <falcon/lineardict.h>
#include <falcon/coretable.h>

#include <string.h>


namespace Falcon
{

ItemArray::ItemArray():
   m_alloc(0),
   m_size(0),
   m_data(0)
{}

ItemArray::ItemArray( const ItemArray& other )
{
   if( other.m_size != 0 )
   {
      m_alloc = other.m_size;
      m_size = other.m_size;
      m_data = (Item *) memAlloc( esize(other.m_size) );
      memcpy( m_data, other.m_data, esize(other.m_size) );
   }
   else
   {
      m_alloc = 0;
      m_size = 0;
      m_data = 0;
   }
}


ItemArray::ItemArray( uint32 prealloc )
{
   m_data = (Item *) memAlloc( esize(prealloc) );
   m_alloc = prealloc;
   m_size = 0;
}


ItemArray::ItemArray( Item *buffer, uint32 size, uint32 alloc ):
   m_alloc(alloc),
   m_size(size),
   m_data(buffer)
{}


ItemArray::~ItemArray()
{
   if ( m_data != 0 )
      memFree( m_data );
}


void ItemArray::gcMark( uint32 mark )
{
   for( uint32 pos = 0; pos < m_size; pos++ ) {
      memPool->markItem( m_data[pos] );
   }
}


void ItemArray::append( const Item &ndata )
{
   // create enough space to hold the data
   if ( m_alloc <= m_size )
   {
      m_alloc = m_size + flc_ARRAY_GROWTH;
      m_data = (Item *) memRealloc( m_data, esize( m_alloc ) );
   }
   m_data[ m_size ] = ndata;
   m_size++;
}


void ItemArray::merge( const ItemArray &other )
{
   if ( other.m_size == 0 )
      return;

   if ( m_alloc < m_size + other.m_size ) {
      m_alloc = m_size + other.m_size;
      m_data = (Item *) memRealloc( m_data, esize( m_alloc ) );
   }

   memcpy( m_data + m_size, other.m_data, esize( other.m_size ) );
   m_size += other.m_size;
}

void ItemArray::prepend( const Item &ndata )
{
   // create enough space to hold the data
   Item *mem = (Item *) memAlloc( esize(m_size + 1) );
   m_alloc = m_size + 1;
   if ( m_size != 0 )
      memcpy( mem + 1, m_data, esize( m_size ) );
   mem[0] = ndata;
   if ( m_size != 0 )
      memFree( m_data );
   m_data = mem;
   m_size++;
}

void ItemArray::merge_front( const ItemArray &other )
{
   if ( other.m_size == 0 )
      return;

   if ( m_alloc < m_size + other.m_size ) {
      m_alloc = m_size + other.m_size;
      Item *mem = (Item *) memAlloc( esize( m_alloc ) );
      memcpy( mem , other.m_data, esize( other.m_size ) );
      if ( m_size > 0 ) {
         memcpy( mem + other.m_size, m_data, esize( m_size ) );
         memFree( m_data );
      }

      m_size = m_alloc;
      m_data = mem;
   }
   else {
      memmove( m_data + other.m_size, m_data, esize( m_size ) );
      memcpy( m_data, other.m_data, esize( other.m_size ) );
      m_size += other.m_size;
   }
}

bool ItemArray::insert( const Item &ndata, int32 pos )
{
   if ( pos < 0 )
      pos = m_size + pos;
   if ( pos < 0 || pos > (int32) m_size )
      return false;

   if ( m_alloc <= m_size ) {
      m_alloc = m_size + flc_ARRAY_GROWTH;
      Item *mem = (Item *) memAlloc( esize( m_alloc ) );
      if ( pos > 0 )
         memcpy( mem , m_data, esize( pos ) );
      if ( pos < (int32)m_size )
         memcpy( mem + pos + 1, m_data + pos , esize(m_size - pos) );

      mem[ pos ] = ndata;
      m_size++;
      memFree( m_data );
      m_data = mem;
   }
   else {
      if ( pos < (int32)m_size )
         memmove( m_data + pos + 1, m_data+pos, esize( m_size - pos) );
      m_data[pos] = ndata;
      m_size ++;
   }
   return true;
}

bool ItemArray::insert( const ItemArray &other, int32 pos )
{
   if ( other.m_size == 0 )
      return true;

   if ( pos < 0 )
      pos = m_size + pos;
   if ( pos < 0 || pos > (int32)m_size )
      return false;

   if ( m_alloc < m_size + other.m_size ) {
      m_alloc = m_size + other.m_size;
      Item *mem = (Item *) memAlloc( esize( m_alloc ) );
      if ( pos > 0 )
         memcpy( mem , m_data, esize( pos ) );

      if ( pos < (int32)m_size )
         memcpy( mem + pos + other.m_size, m_data + pos , esize(m_size - pos) );

      memcpy( mem + pos , other.m_data, esize( other.m_size ) );

      m_size = m_alloc;
      memFree( m_data );
      m_data = mem;
   }
   else {
      if ( pos < (int32)m_size )
         memmove( m_data + other.m_size + pos, m_data + pos, esize(m_size - pos ) );
      memcpy( m_data + pos , other.m_data, esize( other.m_size ) );
      m_size += other.m_size;
   }

   return true;
}

bool ItemArray::remove( int32 first, int32 last )
{
   if ( first < 0 )
      first = m_size + first;
   if ( first < 0 || first >= (int32)m_size )
      return false;

   if ( last < 0 )
      last = m_size + last;
   if ( last < 0 || last > (int32)m_size )
      return false;

   if( first > last ) {
      int32 temp = first;
      first = last;
      // last can't be < first if it was == size.
      last = temp+1;
   }

   uint32 rsize = last - first;
   if ( last < (int32)m_size )
      memmove( m_data + first, m_data + last, esize(m_size - last) );
   m_size -= rsize;
   return true;
}

int32 ItemArray::find( const Item &itm ) const
{
   for( uint32 i = 0; i < m_size; i ++ )
   {
      if ( itm == m_data[ i ] )
         return (int32) i;
   }

   return -1;
}


bool ItemArray::remove( int32 first )
{
   if ( first < 0 )
      first = m_size + first;
   if ( first < 0 || first >= (int32)m_size )
      return false;

   if ( first < (int32)m_size - 1 )
      memmove( m_data + first, m_data + first + 1, esize(m_size - first) );
   m_size --;
   return true;
}

bool ItemArray::change( const ItemArray &other, int32 begin, int32 end )
{
   if ( begin < 0 )
      begin = m_size + begin;
   if ( begin < 0 || begin > (int32)m_size )
      return false;

   if ( end < 0 )
      end = m_size + end;
   if ( end < 0 || end > (int32)m_size )
      return false;

   if( begin > end ) {
      int32 temp = begin;
      begin = end;
      end = temp+1;
   }

   int32 rsize = end - begin;

   // we're considering end as "included" from now on.
   // this considers also negative range which already includes their extreme.
   if ( m_size - rsize + other.m_size > m_alloc )
   {
      m_alloc =  m_size - rsize + other.m_size;
      Item *mem = (Item *) memAlloc( esize( m_alloc ) );
      if ( begin > 0 )
         memcpy( mem, m_data, esize( begin ) );
      if ( other.m_size > 0 )
         memcpy( mem + begin, other.m_data, esize( other.m_size ) );
      if ( end < (int32) m_size )
         memcpy( mem + begin + other.m_size, m_data + end, esize(m_size - end) );

      memFree( m_data );
      m_data = mem;
      m_size = m_alloc;
   }
   else {
      if ( end < (int32)m_size )
         memmove( m_data + begin + other.m_size, m_data + end, esize(m_size - end) );

      if ( other.m_size > 0 )
         memcpy( m_data + begin, other.m_data, esize( other.m_size ) );
      m_size = m_size - rsize + other.m_size;
   }

   return true;
}

bool ItemArray::insertSpace( uint32 pos, uint32 size )
{
   if ( size == 0 )
      return true;

   if ( pos < 0 )
      pos = m_size + pos;
   if ( pos < 0 || pos > m_size )
      return false;

   if ( m_alloc < m_size + size ) {
      m_alloc = ((m_size + size)/flc_ARRAY_GROWTH+1)*flc_ARRAY_GROWTH;
      Item *mem = (Item *) memAlloc( esize( m_alloc ) );
      if ( pos > 0 )
         memcpy( mem , m_data, esize( pos ) );

      if ( pos < m_size )
         memcpy( mem + pos + size, m_data + pos , esize(m_size - pos) );

      for( uint32 i = pos; i < pos + size; i ++ )
         m_data[i] = Item();

      m_size += size;
      memFree( m_data );
      m_data = mem;
   }
   else {
      if ( pos < m_size )
      memmove( m_data + size + pos, m_data + pos, esize(m_size - pos) );
      for( uint32 i = pos; i < pos + size; i ++ )
         m_data[i] = Item();
      m_size += size;
   }

   return true;
}


ItemArray *ItemArray::partition( int32 start, int32 end ) const
{
   int32 size;
   Item *buffer;

   if ( start < 0 )
      start = m_size + start;
   if ( start < 0 || start >= (int32)m_size )
      return 0;

   if ( end < 0 )
      end = m_size + end;
   if ( end < 0 || end > (int32)m_size )
      return 0;

   if( end < start ) {
      size = start - end + 1;
      buffer = (Item *) memAlloc( esize( size ) );

      for( int i = 0; i < size; i ++ )
         buffer[i] = m_data[start - i];
   }
   else {
      if( end == start ) {
         return new ItemArray();
      }
      size = end - start;
      buffer = (Item *) memAlloc( esize( size ) );
      memcpy( buffer, m_data + start, esize( size )  );
   }

   return new ItemArray( buffer, size, size );
}


FalconData *ItemArray::clone() const
{
   return new ItemArray( *this );
}


void ItemArray::resize( uint32 size ) {
   if ( size == 0 ) {
      if ( m_data != 0 ) {
         memFree( m_data );
         m_data = 0;
      }
      m_alloc = 0;
   }
   // use this request also to force size in shape with alloc.
   else if ( size > m_alloc ) {
      m_alloc = (size/flc_ARRAY_GROWTH + 1) *flc_ARRAY_GROWTH;
      m_data = (Item *) memRealloc( m_data, esize( m_alloc ) );
      memset( m_data + m_size, 0, esize( m_alloc - m_size ) );
   }
   else if ( size > m_size )
      memset( m_data + m_size, 0, esize( size - m_size ) );

   m_size = size;
}

void ItemArray::compact() {
   if ( m_size == 0 ) {
      if ( m_data != 0 ) {
         memFree( m_data );
         m_data = 0;
      }
      m_alloc = 0;
   }
   else if ( m_size < m_alloc )
   {
      m_alloc = m_size;
      m_data = (Item *) memRealloc( m_data, esize( m_alloc ) );
      memset( m_data + m_size, 0, esize( m_alloc - m_size ) );
   }
}

void ItemArray::reserve( uint32 size ) {
   if ( size > m_alloc ) {
      m_data = (Item *) memRealloc( m_data, esize( size ) );
      m_alloc = size;
   }
}

bool ItemArray::copyOnto( uint32 from, const ItemArray& src, uint32 first, uint32 amount )
{
   if( first > src.length() )
      return false;

   if ( first + amount > src.length() )
      amount = src.length() - first;

   if ( from > length() )
      return false;

   if ( from + amount > length() )
      resize( from + amount );

  memcpy( m_data + from, src.m_data + first, esize( amount ) );
}

}

/* end of itemarray.cpp */