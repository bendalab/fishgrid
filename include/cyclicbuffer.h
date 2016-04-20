/*
  cyclicbuffer.h
  A template defining an one-dimensional cyclic array of data.

  FishGrid
  Copyright (C) 2009 Jan Benda <benda@bio.lmu.de> & Joerg Henninger <henninger@bio.lmu.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.
  
  RELACS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _CYCLICBUFFER_H_
#define _CYCLICBUFFER_H_ 1

#include <cstdlib>
#include <cassert>
#include <iostream>
using namespace std;


/*!
\class CyclicBuffer
\brief A template defining an one-dimensional cyclic array of data.
\author Jan Benda

This class is very similar to the vector class from 
the standard template library, in that it is a
random access container of objects of type \a T.
The size() of CyclicBuffer, however, can exceed its capacity().
Data elements below size()-capacity() are therefore not accessible.
*/

template < class T = double >
class CyclicBuffer
{

  public:

    /*! Creates an empty CyclicBuffer. */
  CyclicBuffer( void );
    /*! Creates an empty array with capacity \a n data elements. */
  CyclicBuffer( int n );
    /*! Copy constructor.
        Creates an CyclicBuffer with the same size and content as \a ca. */
  CyclicBuffer( const CyclicBuffer< T > &ca );
    /*! The destructor. */
  virtual ~CyclicBuffer( void );

    /*! Assigns \a a to *this. */
  const CyclicBuffer<T> &operator=( const CyclicBuffer<T> &a );
    /*! Assigns \a a to *this. */
  const CyclicBuffer<T> &assign( const CyclicBuffer<T> &a );

    /*! The size of the array, 
        i.e. the total number of added data elements.
        Can be larger than capacity()!
        \sa accessibleSize(), readSize(), empty() */
  long long size( void ) const;
    /*! The number of data elements that are actually stored in the array
        and therefore are accessible.
        Less or equal than capacity() and size()!
        \sa minIndex(), readSize(), empty() */
  int accessibleSize( void ) const;
    /*! The index of the first accessible data element.
        \sa accessibleSize() */
  long long minIndex( void ) const;
    /*! True if the array does not contain any data elements,
        i.e. size() equals zero.
        \sa size(), accessibleSize(), readSize() */
  bool empty( void ) const;
    /*! Resize the array to \a n data elements
        such that the size() of the array equals \a n.
        Data values are preserved and new data values
	are initialized with \a val.
	The capacity is not changed!
        If, however, the capacity() of the array is zero,
        then memory for \a n data elements is allocated
        and initialized with \a val. */
  virtual void resize( long long n, const T &val=0 );
    /*! Resize the array to zero length.
        The capacity() remains unchanged. */
  virtual void clear( void );

    /*! The capacity of the array, i.e. the number of data
        elements for which memory has been allocated. */
  int capacity( void ) const;
    /*! If \a n is less than or equal to capacity(), 
        this call has no effect. 
	Otherwise, it is a request for allocation 
	of additional memory. 
	If the request is successful, 
	then capacity() is greater than or equal to \a n; 
	otherwise, capacity() is unchanged. 
	In either case, size() is unchanged and the content
	of the array is preserved. */
  virtual void reserve( int n );

    /*! Returns a const reference to the data element at index \a i.
        No range checking is performed. */
  inline const T &operator[]( long long i ) const;
    /*! Returns a reference to the data element at index \a i.
        No range checking is performed. */
  inline T &operator[]( long long i );
    /*! Returns a const reference to the data element at index \a i.
        If \a i is an invalid index
	a reference to a variable set to zero is returned. */
  const T &at( long long i ) const;
    /*! Returns a reference to the data element at index \a i.
        If \a i is an invalid index
	a reference to a variable set to zero is returned. */
  T &at( long long i );

    /*! Returns a const reference to the first data element.
        If the array is empty or the first element is 
	not accesible
	a reference to a variable set to zero is returned. */
  const T &front( void ) const;
    /*! Returns a reference to the first data element.
        If the array is empty or the first element is 
	not accesible
	a reference to a variable set to zero is returned. */
  T &front( void );
    /*! Returns a const reference to the last data element.
        If the array is empty
	a reference to a variable set to zero is returned. */
  const T &back( void ) const;
    /*! Returns a reference to the last data element.
        If the array is empty
	a reference to a variable set to zero is returned. */
  T &back( void );

    /*! Add \a val as a new element to the array. */
  inline void push( const T &val );
    /*! Remove the last element of the array
        and return its value. */
  inline T pop( void );
    /*! Maximum number of data elements allowed to be added to the buffer 
        at once. 
        \sa pushBuffer(), push() */
  int maxPush( void ) const;
    /*! Pointer into the buffer where to add data.
        \sa maxPush(), push() */
  T *pushBuffer( void );
    /*! Tell CyclicBuffer that \a n data elements have been added to
        pushBuffer().
        \sa maxPush() */
  void push( int n );

    /*! The number of data elements available to be read from the array. 
        \sa read(), readIndex(), size(), accessibleSize() */
  long long readSize( void ) const;
    /*! The index of the data element to be read next from the array. 
        \sa read(), readSize() */
  long long readIndex( void ) const;
    /*! Return value of the first to be read data element
        and increment read index.
        \sa readSize(), readIndex() */
  inline T read( void );

    /*! The type of object, T, stored in the arry. */
  typedef T value_type;
    /*! Pointer to the type of object, T, stored in the array. */
  typedef T* pointer;
    /*! Reference to the type of object, T, stored in the array. */
  typedef T& reference;
    /*! Const reference to the type of object, T, stored in the array. */
  typedef const T& const_reference;
    /*! The type used for sizes and indices. */
  typedef long long size_type;

    /*! Save binary data to stream \a os starting at index \a index upto size().
        \return the number of saved data elements. */
  int saveBinary( ostream &os, long long index ) const;
    /*! Save binary data to stream \a os starting at index \a from upto index \a upto.
        Assumes \a from and \a upto to be valid indices, i.e. <= size() and >= minIndex().
        \return the number of saved data elements. */
  int saveBinary( ostream &os, long long from, long long upto ) const;

  template < typename TT > 
  friend ostream &operator<<( ostream &str, const CyclicBuffer<TT> &ca );


  private:

  T *Buffer;
  int NBuffer;
  int RCycles;
  int R;
  int LCycles;
  int L;
  T Val;  // for pop()
  mutable T Dummy;
  
};


template < class T >
CyclicBuffer< T >::CyclicBuffer( void )
  : Buffer( 0 ),
    NBuffer( 0 ),
    RCycles( 0 ),
    R( 0 ),
    LCycles( 0 ),
    L( 0 ),
    Val( 0 ),
    Dummy( 0 )
{
}


template < class T >
CyclicBuffer< T >::CyclicBuffer( int n )
  : Buffer( 0 ),
    NBuffer( 0 ),
    RCycles( 0 ),
    R( 0 ),
    LCycles( 0 ),
    L( 0 ),
    Val( 0 ),
    Dummy( 0 )
{
  if ( n > 0 ) {
    Buffer = new T[ n ];
    NBuffer = n;
  }
}


template < class T >
CyclicBuffer< T >::CyclicBuffer( const CyclicBuffer< T > &ca )
  : Buffer( 0 ),
    NBuffer( 0 ),
    RCycles( ca.RCycles ),
    R( ca.R ),
    LCycles( ca.LCycles ),
    L( ca.L ),
    Val( ca.Val ),
    Dummy( ca.Dummy )
{
  if ( ca.capacity() > 0 ) {
    Buffer = new T[ ca.capacity() ];
    NBuffer = ca.capacity();
    memcpy( Buffer, ca.Buffer, NBuffer * sizeof( T ) );
  }
}


template < class T >
CyclicBuffer< T >::~CyclicBuffer( void )
{
  if ( Buffer != 0 )
    delete [] Buffer;
}


template < class T >
const CyclicBuffer<T> &CyclicBuffer<T>::operator=( const CyclicBuffer<T> &a )
{
  return assign( a );
}


template < class T >
const CyclicBuffer<T> &CyclicBuffer<T>::assign( const CyclicBuffer<T> &a )
{
  if ( &a == this )
    return *this;

  if ( Buffer != 0 )
    delete [] Buffer;
  Buffer = 0;
  NBuffer = 0;
  Val = 0;

  if ( a.capacity() > 0 ) {
    Buffer = new T[ a.capacity() ];
    NBuffer = a.capacity();
    R = a.size();
    memcpy( Buffer, a.Buffer, NBuffer * sizeof( T ) );
    RCycles = a.RCycles;
    R = a.R;
    LCycles = a.LCycles;
    L = a.L;
  }
  else {
    RCycles = 0;
    R = 0;
    LCycles = 0;
    L = 0;
  }

  return *this;
}


template < class T >
long long CyclicBuffer< T >::size( void ) const
{
  long long n = RCycles;
  n *= NBuffer;
  return n + R;
}


template < class T >
int CyclicBuffer< T >::accessibleSize( void ) const
{
  return RCycles == 0 ? R : NBuffer;
}


template < class T >
long long CyclicBuffer< T >::minIndex( void ) const
{
  if ( RCycles == 0 )
    return 0;

  long long n = RCycles-1;
  n *= NBuffer;
  return n + R;
}


template < class T >
bool CyclicBuffer< T >::empty( void ) const
{
  return RCycles == 0 && R == 0;
}


template < class T >
void CyclicBuffer< T >::resize( long long n, const T &val )
{
  if ( n <= 0 ) {
    clear();
    return;
  }

  if ( NBuffer <= 0 ) {
    reserve( n );
    for ( int k=0; k<NBuffer; k++ )
      Buffer[k] = val;
    RCycles = 0;
    R = n;
    LCycles = 0;
    L = 0;
    return;
  }

  if ( n < size() ) {
    RCycles = (n-1) / NBuffer;
    R = 1 + (n-1) % NBuffer;
    if ( RCycles * NBuffer + R < LCycles * NBuffer + L ) {
      RCycles = LCycles;
      R = L;
    }
  }
  else if ( n > size() ) {
    if ( n - size() >= NBuffer ) {
      for ( int k=0; k<NBuffer; k++ )
	Buffer[k] = val;
      RCycles = (n-1) / NBuffer;
      R = 1 + (n-1) % NBuffer;
    }
    else {
      int orc = RCycles;
      int ori = R;
      RCycles = (n-1) / NBuffer;
      R = 1 + (n-1) % NBuffer;
      if ( RCycles > orc ) {
	for ( int k=ori; k<NBuffer; k++ )
	  Buffer[k] = val;
      }
      for ( int k=0; k<R; k++ )
	Buffer[k] = val;
    }
    if ( (LCycles+1)*NBuffer + L < RCycles*NBuffer + R ) {
      LCycles = RCycles-1;
      L = R;
    }
  }
}


template < class T >
void CyclicBuffer< T >::clear( void )
{
  RCycles = 0;
  R = 0;
  LCycles = 0;
  L = 0;
}


template < class T >
int CyclicBuffer< T >::capacity( void ) const
{
  return NBuffer;
}


template < class T >
void CyclicBuffer< T >::reserve( int n )
{
  if ( n > NBuffer ) {
    T *newbuf = new T[ n ];
    if ( Buffer != 0 && NBuffer > 0 ) {
      int ori = R;
      long long on = size();
      RCycles = (on-1) / n;
      R = 1 + (on-1) % n;
      int j = ori;
      int k = R;
      for ( int i=0; i < NBuffer; i++ ) {
	if ( j == 0 )
	  j = NBuffer;
	if ( k == 0 )
	  k = n;
	j--;
	k--;
	newbuf[k] = Buffer[j];
      }
      delete [] Buffer;
      int oln = LCycles*NBuffer + L;
      LCycles = (oln-1) / n;
      L = 1 + (oln-1) % n;
    }
    Buffer = newbuf;
    NBuffer = n;
  }
}


template < class T >
const T &CyclicBuffer< T >::operator[]( long long i ) const
{
  return Buffer[ i % NBuffer ];
}


template < class T >
T &CyclicBuffer< T >::operator[]( long long i )
{
  return Buffer[ i % NBuffer ];
}


template < typename T > 
const T &CyclicBuffer<T>::at( long long i ) const
{
  if ( Buffer != 0 &&
       i >= minIndex() && i < size() ) {
    return Buffer[ i % NBuffer ];
  }
  else {
    Dummy = 0;
    return Dummy;
  }
}


template < typename T > 
T &CyclicBuffer<T>::at( long long i )
{
  if ( Buffer != 0 &&
       i >= minIndex() && i < size() ) {
    return Buffer[ i % NBuffer ];
  }
  else {
    Dummy = 0;
    return Dummy;
  }
}


template < typename T > 
const T &CyclicBuffer<T>::front( void ) const
{
  if ( Buffer != 0 && size() > 0 && minIndex() == 0 ) {
    return Buffer[0];
  }
  else {
    Dummy = 0;
    return Dummy;
  }
}


template < typename T > 
T &CyclicBuffer<T>::front( void )
{
  if ( Buffer != 0 && size() > 0 && minIndex() == 0 ) {
    return Buffer[0];
  }
  else {
    Dummy = 0;
    return Dummy;
  }
}


template < typename T > 
const T &CyclicBuffer<T>::back( void ) const
{
  if ( Buffer != 0 && size() > 0 ) {
    return Buffer[R-1];
  }
  else {
    Dummy = 0;
    return Dummy;
  }
}


template < typename T > 
T &CyclicBuffer<T>::back( void )
{
  if ( Buffer != 0 && size() > 0 ) {
    return Buffer[R-1];
  }
  else {
    Dummy = 0;
    return Dummy;
  }
}


template < class T >
void CyclicBuffer< T >::push( const T &val )
{
  assert( Buffer != 0 && NBuffer > 0 );
  if ( NBuffer <= 0 )
    reserve( 100 );

  if ( R >= NBuffer ) {
    R = 0;
    RCycles++;
  }

  Val = Buffer[ R ];
  Buffer[ R ] = val;

  R++;
}


template < class T >
T CyclicBuffer< T >::pop( void )
{
  if ( NBuffer <= 0 || R <= 0 )
    return 0;

  R--;

  T v = Buffer[ R ];
  Buffer[ R ] = Val;

  if ( R == 0 && RCycles > 0 ) {
    R = NBuffer;
    RCycles--;
  }

  return v;
}


template < class T >
int CyclicBuffer< T >::maxPush( void ) const
{
  return R < NBuffer ? NBuffer - R : NBuffer;
}


template < class T >
T *CyclicBuffer< T >::pushBuffer( void )
{
  return R < NBuffer ? Buffer + R : Buffer;
}


template < class T >
void CyclicBuffer< T >::push( int n )
{
  if ( R >= NBuffer ) {
    R = 0;
    RCycles++;
  }

  R += n;

#ifndef NDEBUG
  if ( !( R >= 0 && R <= NBuffer ) )
    cerr << "CyclicBuffer::push( int n ): R=" << R << " <0 or > NBuffer=" << NBuffer << endl; 
#endif
  assert( ( R >= 0 && R <= NBuffer ) );
}


template < class T >
long long CyclicBuffer< T >::readSize( void ) const
{ 
  long long n = (RCycles - LCycles)*NBuffer + R - L;
#ifndef NDEBUG
  if ( n > NBuffer )
    cerr << "overflow in CyclicBuffer!\n";
#endif
  assert ( n <= NBuffer );
  return n;
}


template < class T >
long long CyclicBuffer< T >::readIndex( void ) const
{
  return LCycles*NBuffer + L;
}


template < class T >
T CyclicBuffer< T >::read( void )
{
  if ( NBuffer <= 0 )
    return 0.0;

  int l = L++;
  if ( L >= NBuffer ) {
    L = 0;
    LCycles++;
  }

  return Buffer[ l ];
}


template < class T >
int CyclicBuffer< T >::saveBinary( ostream &os, long long index ) const
{
  // stream not open:
  if ( !os )
    return -1;

  // nothing to be saved:
  if ( index >= size() )
    return -1;

  assert( index >= minIndex() );

  long long buffinx = RCycles * NBuffer;
  long long li = index - buffinx;
  int ri = R;

  // write buffer:
  if ( li >= 0 )
    os.write( (char *)(Buffer+li), sizeof( T )*( ri - li ) );
  else {
    os.write( (char *)(Buffer + (li+NBuffer)), sizeof( T )*( -li ) );
    os.write( (char *)Buffer, sizeof( T )*ri );
  }
  os.flush();
  return ri - li;
}


template < class T >
int CyclicBuffer< T >::saveBinary( ostream &os, long long from, long long upto ) const
{
  // stream not open:
  if ( !os )
    return -1;

  // nothing to be saved:
  if ( from == upto )
    return -2;
  if ( from > upto )
    return -3;

  int fi = from % NBuffer;
  int ui = upto % NBuffer;

  // write buffer:
  if ( fi < ui ) {
    os.write( (char *)(Buffer+fi), sizeof( T )*( ui - fi ) );
    os.flush();
    return ui - fi;
  }
  else {
    if ( ui + NBuffer <=  fi )
      return -4;
    os.write( (char *)(Buffer+fi), sizeof( T )*( NBuffer-fi ) );
    os.write( (char *)Buffer, sizeof( T )*ui );
    os.flush();
    return ui + NBuffer - fi;
  }
  return 0;
}


template < class T >
ostream &operator<<( ostream &str, const CyclicBuffer< T > &ca )
{
  str << "Buffer: " << ca.Buffer << '\n';
  str << "NBuffer: " << ca.NBuffer << '\n';
  str << "RCycles: " << ca.RCycles << '\n';
  str << "R: " << ca.R << '\n';
  str << "LCycles: " << ca.LCycles << '\n';
  str << "L: " << ca.L << '\n';
  str << "Val: " << ca.Val << '\n';
  return str;
}


#endif /* ! _CYCLICBUFFER_H_ */

