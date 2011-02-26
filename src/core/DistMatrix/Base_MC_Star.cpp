/*
   Copyright (c) 2009-2011, Jack Poulson
   All rights reserved.

   This file is part of Elemental.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    - Neither the name of the owner nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/
#include "elemental/dist_matrix.hpp"
using namespace std;
using namespace elemental;
using namespace elemental::imports;
using namespace elemental::utilities;

// Template conventions:
//   G: general datatype
//
//   T: any ring, e.g., the (Gaussian) integers and the real/complex numbers
//   Z: representation of a real ring, e.g., the integers or real numbers
//   std::complex<Z>: representation of a complex ring, e.g. Gaussian integers
//                    or complex numbers
//
//   F: representation of real or complex number
//   R: representation of real number
//   std::complex<R>: representation of complex number

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::Print( const string& s ) const
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::Print");
#endif
    const elemental::Grid& g = this->Grid();
    if( g.VCRank() == 0 && s != "" )
        cout << s << endl;
        
    const int height      = this->Height();
    const int width       = this->Width();
    const int localHeight = this->LocalHeight();
    const int r           = g.Height();
    const int colShift    = this->ColShift();

    if( height == 0 || width == 0 )
    {
#ifndef RELEASE
        PopCallStack();
#endif
        return;
    }

    if( g.InGrid() )
    {
        // Only one process column needs to participate
        if( g.MRRank() == 0 )
        {
            vector<T> sendBuf(height*width,0);
            const T* thisLocalBuffer = this->LockedLocalBuffer();
            const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
            #pragma omp parallel for COLLAPSE(2)
#endif
            for( int iLocal=0; iLocal<localHeight; ++iLocal )
                for( int jLocal=0; jLocal<width; ++jLocal )
                    sendBuf[(colShift+iLocal*r)+jLocal*height] = 
                        thisLocalBuffer[iLocal+jLocal*thisLDim];

            // If we are the root, allocate a receive buffer
            vector<T> recvBuf;
            if( g.MCRank() == 0 )
                recvBuf.resize( height*width );

            // Sum the contributions and send to the root
            mpi::Reduce
            ( &sendBuf[0], &recvBuf[0], height*width, mpi::SUM, 0, g.MCComm() );

            if( g.MCRank() == 0 )
            {
                // Print the data
                for( int i=0; i<height; ++i )
                {
                    for( int j=0; j<width; ++j )
                        cout << recvBuf[i+j*height] << " ";
                    cout << "\n";
                }
                cout << endl;
            }
        }
        mpi::Barrier( g.VCComm() );
    }

#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::Align
( int colAlignment )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::Align");
    this->AssertFreeColAlignment();
#endif
    this->AlignCols( colAlignment );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::AlignCols
( int colAlignment )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::AlignCols");
    this->AssertFreeColAlignment();
#endif
    const elemental::Grid& g = this->Grid();
#ifndef RELEASE
    if( colAlignment < 0 || colAlignment >= g.Height() )
        throw std::runtime_error( "Invalid column alignment for [MC,* ]" );
#endif
    this->_colAlignment = colAlignment;
    this->_constrainedColAlignment = true;
    this->_height = 0;
    this->_width = 0;
    if( g.InGrid() )
    {
        this->_colShift = Shift( g.MCRank(), colAlignment, g.Height() );
        this->_localMatrix.ResizeTo( 0, 0 );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::AlignWith
( const DistMatrixBase<T,MC,MR>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::AlignWith([MC,MR])");
    this->AssertFreeColAlignment();
    this->AssertSameGrid( A );
#endif
    this->_colAlignment = A.ColAlignment();
    this->_constrainedColAlignment = true;
    this->_height = 0;
    this->_width = 0;
    if( this->Grid().InGrid() )
    {
        this->_colShift = A.ColShift();
        this->_localMatrix.ResizeTo( 0, 0 );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::AlignWith
( const DistMatrixBase<T,MC,Star>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::AlignWith([MC,* ])");
    this->AssertFreeColAlignment();
    this->AssertSameGrid( A );
#endif
    this->_colAlignment = A.ColAlignment();
    this->_constrainedColAlignment = true;
    this->_height = 0;
    this->_width = 0;
    if( this->Grid().InGrid() )
    {
        this->_colShift = A.ColShift();
        this->_localMatrix.ResizeTo( 0, 0 );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::AlignWith
( const DistMatrixBase<T,MR,MC>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::AlignWith([MR,MC])");
    this->AssertFreeColAlignment();
    this->AssertSameGrid( A );
#endif
    this->_colAlignment = A.RowAlignment();
    this->_constrainedColAlignment = true;
    this->_height = 0;
    this->_width = 0;
    if( this->Grid().InGrid() )
    {
        this->_localMatrix.ResizeTo( 0, 0 );
        this->_colShift = A.RowShift();
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::AlignWith
( const DistMatrixBase<T,Star,MC>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::AlignWith([* ,MC])");
    this->AssertFreeColAlignment();
    this->AssertSameGrid( A );
#endif
    this->_colAlignment = A.RowAlignment();
    this->_constrainedColAlignment = true;
    this->_height = 0;
    this->_width = 0;
    if( this->Grid().InGrid() )
    {
        this->_localMatrix.ResizeTo( 0, 0 );
        this->_colShift = A.RowShift();
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::AlignWith
( const DistMatrixBase<T,VC,Star>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::AlignWith([VC,* ])");
    this->AssertFreeColAlignment();
    this->AssertSameGrid( A );
#endif
    const elemental::Grid& g = this->Grid();
    this->_colAlignment = A.ColAlignment() % g.Height();
    this->_constrainedColAlignment = true;
    this->_height = 0;
    this->_width = 0;
    if( g.InGrid() )
    {
        this->_localMatrix.ResizeTo( 0, 0 );
        this->_colShift = 
            Shift( g.MCRank(), this->ColAlignment(), g.Height() );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::AlignWith
( const DistMatrixBase<T,Star,VC>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::AlignWith([* ,VC])");
    this->AssertFreeColAlignment();
    this->AssertSameGrid( A );
#endif
    const elemental::Grid& g = this->Grid();
    this->_colAlignment = A.RowAlignment() % g.Height();
    this->_constrainedColAlignment = true;
    this->_height = 0;
    this->_width = 0;
    if( g.InGrid() )
    {
        this->_localMatrix.ResizeTo( 0, 0 );
        this->_colShift = 
            Shift( g.MCRank(), this->ColAlignment(), g.Height() );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::AlignColsWith
( const DistMatrixBase<T,MC,MR>& A )
{ AlignWith( A ); }

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::AlignColsWith
( const DistMatrixBase<T,MC,Star>& A )
{ AlignWith( A ); }

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::AlignColsWith
( const DistMatrixBase<T,MR,MC>& A )
{ AlignWith( A ); }

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::AlignColsWith
( const DistMatrixBase<T,Star,MC>& A )
{ AlignWith( A ); }

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::AlignColsWith
( const DistMatrixBase<T,VC,Star>& A )
{ AlignWith( A ); }

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::AlignColsWith
( const DistMatrixBase<T,Star,VC>& A )
{ AlignWith( A ); }

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::View
( DistMatrixBase<T,MC,Star>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::View");
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( A );
#endif
    this->_height = A.Height();
    this->_width = A.Width();
    this->_colAlignment = A.ColAlignment();
    this->_viewing = true;
    this->_lockedView = false;
    if( this->Grid().InGrid() )
    {
        this->_colShift = A.ColShift();
        this->_localMatrix.View( A.LocalMatrix() );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::LockedView
( const DistMatrixBase<T,MC,Star>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::LockedView");
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( A );
#endif
    this->_height = A.Height();
    this->_width = A.Width();
    this->_colAlignment = A.ColAlignment();
    this->_viewing = true;
    this->_lockedView = true;
    if( this->Grid().InGrid() )
    {
        this->_colShift = A.ColShift();
        this->_localMatrix.LockedView( A.LockedLocalMatrix() );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::View
( DistMatrixBase<T,MC,Star>& A,
  int i, int j, int height, int width )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::View");
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( A );
    this->AssertValidSubmatrix( A, i, j, height, width );
#endif
    this->_height = height;
    this->_width = width;
    this->_viewing = true;
    this->_lockedView = false;

    const int r = this->Grid().Height();
    const int row = this->Grid().MCRank();

    this->_colAlignment = (A.ColAlignment()+i) % r;

    if( this->Grid().InGrid() )
    {
        this->_colShift = Shift( row, this->ColAlignment(), r );
        const int localHeightBefore = LocalLength( i, A.ColShift(), r );
        const int localHeight = LocalLength( height, this->_colShift, r );
        this->_localMatrix.View
        ( A.LocalMatrix(), localHeightBefore, j, localHeight, width );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::LockedView
( const DistMatrixBase<T,MC,Star>& A,
  int i, int j, int height, int width )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::LockedView");
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( A );
    this->AssertValidSubmatrix( A, i, j, height, width );
#endif
    this->_height = height;
    this->_width = width;
    this->_viewing = true;
    this->_lockedView = true;

    const int r   = this->Grid().Height();
    const int row = this->Grid().MCRank();

    this->_colAlignment = (A.ColAlignment()+i) % r;

    if( this->Grid().InGrid() )
    {
        this->_colShift = Shift( row, this->ColAlignment(), r );
        const int localHeightBefore = LocalLength( i, A.ColShift(), r );
        const int localHeight = LocalLength( height, this->_colShift, r );

        this->_localMatrix.LockedView
        ( A.LockedLocalMatrix(), localHeightBefore, j, localHeight, width );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::View1x2
( DistMatrixBase<T,MC,Star>& AL,
  DistMatrixBase<T,MC,Star>& AR )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::View1x2");    
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( AL );
    this->AssertSameGrid( AR );
    this->AssertConforming1x2( AL, AR );
#endif
    this->_height = AL.Height();
    this->_width = AL.Width() + AR.Width();
    this->_colAlignment = AL.ColAlignment();
    this->_viewing = true;
    this->_lockedView = false;
    if( this->Grid().InGrid() )
    {
        this->_colShift = AL.ColShift();
        this->_localMatrix.View1x2( AL.LocalMatrix(), AR.LocalMatrix() );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::LockedView1x2
( const DistMatrixBase<T,MC,Star>& AL,
  const DistMatrixBase<T,MC,Star>& AR )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::LockedView1x2");
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( AL );
    this->AssertSameGrid( AR );
    this->AssertConforming1x2( AL, AR );
#endif
    this->_height = AL.Height();
    this->_width = AL.Width() + AR.Width();
    this->_colAlignment = AL.ColAlignment();
    this->_viewing = true;
    this->_lockedView = true;
    if( this->Grid().InGrid() )
    {
        this->_colShift = AL.ColShift();
        this->_localMatrix.LockedView1x2
        ( AL.LockedLocalMatrix(), AR.LockedLocalMatrix() );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::View2x1
( DistMatrixBase<T,MC,Star>& AT,
  DistMatrixBase<T,MC,Star>& AB )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::View2x1");
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( AT );
    this->AssertSameGrid( AB );
    this->AssertConforming2x1( AT, AB );
#endif
    this->_height = AT.Height() + AB.Height();
    this->_width = AT.Width();
    this->_colAlignment = AT.ColAlignment();
    this->_viewing = true;
    this->_lockedView = false;
    if( this->Grid().InGrid() )
    {
        this->_colShift = AT.ColShift();
        this->_localMatrix.View2x1
        ( AT.LocalMatrix(),
          AB.LocalMatrix() );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::LockedView2x1
( const DistMatrixBase<T,MC,Star>& AT,
  const DistMatrixBase<T,MC,Star>& AB )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::LockedView2x1");
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( AT );
    this->AssertSameGrid( AB );
    this->AssertConforming2x1( AT, AB );
#endif
    this->_height = AT.Height() + AB.Height();
    this->_width = AT.Width();
    this->_colAlignment = AT.ColAlignment();
    this->_viewing = true;
    this->_lockedView = true;
    if( this->Grid().InGrid() )
    {
        this->_colShift = AT.ColShift();
        this->_localMatrix.LockedView2x1
        ( AT.LockedLocalMatrix(),
          AB.LockedLocalMatrix() );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::View2x2
( DistMatrixBase<T,MC,Star>& ATL,
  DistMatrixBase<T,MC,Star>& ATR,
  DistMatrixBase<T,MC,Star>& ABL,
  DistMatrixBase<T,MC,Star>& ABR )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::View2x2");
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( ATL );
    this->AssertSameGrid( ATR );
    this->AssertSameGrid( ABL );
    this->AssertSameGrid( ABR );
    this->AssertConforming2x2( ATL, ATR, ABL, ABR );
#endif
    this->_height = ATL.Height() + ABL.Height();
    this->_width = ATL.Width() + ATR.Width();
    this->_colAlignment = ATL.ColAlignment();
    this->_viewing = true;
    this->_lockedView = false;
    if( this->Grid().InGrid() )
    {
        this->_colShift = ATL.ColShift();
        this->_localMatrix.View2x2
        ( ATL.LocalMatrix(), ATR.LocalMatrix(),
          ABL.LocalMatrix(), ABR.LocalMatrix() );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::LockedView2x2
( const DistMatrixBase<T,MC,Star>& ATL,
  const DistMatrixBase<T,MC,Star>& ATR,
  const DistMatrixBase<T,MC,Star>& ABL,
  const DistMatrixBase<T,MC,Star>& ABR )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::LockedView2x2");
    this->AssertFreeColAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( ATL );
    this->AssertSameGrid( ATR );
    this->AssertSameGrid( ABL );
    this->AssertSameGrid( ABR );
    this->AssertConforming2x2( ATL, ATR, ABL, ABR );
#endif
    this->_height = ATL.Height() + ABL.Height();
    this->_width = ATL.Width() + ATR.Width();
    this->_colAlignment = ATL.ColAlignment();
    this->_viewing = true;
    this->_lockedView = true;
    if( this->Grid().InGrid() )
    {
        this->_colShift = ATL.ColShift();
        this->_localMatrix.LockedView2x2
        ( ATL.LockedLocalMatrix(), ATR.LockedLocalMatrix(),
          ABL.LockedLocalMatrix(), ABR.LockedLocalMatrix() );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::ResizeTo
( int height, int width )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::ResizeTo");
    this->AssertNotLockedView();
    if( height < 0 || width < 0 )
        throw logic_error( "Height and width must be non-negative." );
#endif
    this->_height = height;
    this->_width = width;
    if( this->Grid().InGrid() )
    {
        this->_localMatrix.ResizeTo
        ( LocalLength(height,this->ColShift(),this->Grid().Height()), width );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
T
elemental::DistMatrixBase<T,MC,Star>::Get
( int i, int j ) const
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::Get");
    this->AssertValidEntry( i, j );
#endif
    // We will determine the owner row of entry (i,j) and broadcast from that 
    // row within each process column
    const elemental::Grid& g = this->Grid();
    const int ownerRow = (i + this->ColAlignment()) % g.Height();

    T u;
    if( g.VCRank() == ownerRow )
    {
        const int iLoc = (i-this->ColShift()) / g.Height();
        u = this->GetLocalEntry(iLoc,j);
    }
    mpi::Broadcast( &u, 1, g.VCToViewingMap(ownerRow), g.ViewingComm() );

#ifndef RELEASE
    PopCallStack();
#endif
    return u;
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::Set
( int i, int j, T u )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::Set");
    this->AssertValidEntry( i, j );
#endif
    const elemental::Grid& g = this->Grid();
    const int ownerRow = (i + this->ColAlignment()) % g.Height();

    if( g.MCRank() == ownerRow )
    {
        const int iLoc = (i-this->ColShift()) / g.Height();
        this->SetLocalEntry(iLoc,j,u);
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::Update
( int i, int j, T u )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::Update");
    this->AssertValidEntry( i, j );
#endif
    const elemental::Grid& g = this->Grid();
    const int ownerRow = (i + this->ColAlignment()) % g.Height();

    if( g.MCRank() == ownerRow )
    {
        const int iLoc = (i-this->ColShift()) / g.Height();
        this->UpdateLocalEntry(iLoc,j,u);
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

//
// Utility functions, e.g., SetToIdentity and MakeTrapezoidal
//

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::MakeTrapezoidal
( Side side, Shape shape, int offset )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::MakeTrapezoidal");
    this->AssertNotLockedView();
#endif

    const int height = this->Height();
    const int width = this->Width();
    const int localHeight = this->LocalHeight();
    const int r = this->Grid().Height();
    const int colShift = this->ColShift();

    if( this->Grid().InGrid() )
    {
        if( shape == Lower )
        {
            T* thisLocalBuffer = this->LocalBuffer();
            const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
            #pragma omp parallel for
#endif
            for( int j=0; j<width; ++j )
            {
                int lastZeroRow = ( side==Left ? j-offset-1
                                               : j-offset+height-width-1 );
                if( lastZeroRow >= 0 )
                {
                    int boundary = min( lastZeroRow+1, height );
                    int numZeroRows = RawLocalLength( boundary, colShift, r );
                    T* thisCol = &thisLocalBuffer[j*thisLDim];
                    memset( thisCol, 0, numZeroRows*sizeof(T) );
                }
            }
        }
        else
        {
            T* thisLocalBuffer = this->LocalBuffer();
            const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
            #pragma omp parallel for
#endif
            for( int j=0; j<width; ++j )
            {
                int firstZeroRow = 
                    ( side==Left ? max(j-offset+1,0)
                      : max(j-offset+height-width+1,0) );
                int numNonzeroRows = RawLocalLength(firstZeroRow,colShift,r);
                if( numNonzeroRows < localHeight )
                {
                    T* thisCol = &thisLocalBuffer[numNonzeroRows+j*thisLDim];
                    memset
                    ( thisCol, 0, (localHeight-numNonzeroRows)*sizeof(T) );
                }
            }
        }
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::ScaleTrapezoidal
( T alpha, Side side, Shape shape, int offset )
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::ScaleTrapezoidal");
    this->AssertNotLockedView();
#endif

    const int height = this->Height();
    const int width = this->Width();
    const int localHeight = this->LocalHeight();
    const int r = this->Grid().Height();
    const int colShift = this->ColShift();

    if( shape == Upper )
    {
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int j=0; j<width; ++j )
        {
            int lastRow = ( side==Left ? j-offset : j-offset+height-width );
            int boundary = min( lastRow+1, height );
            int numRows = RawLocalLength( boundary, colShift, r );
            T* thisCol = &thisLocalBuffer[j*thisLDim];
            for( int iLocal=0; iLocal<numRows; ++iLocal )
                thisCol[iLocal] *= alpha;
        }
    }
    else
    {
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int j=0; j<width; ++j )
        {
            int firstRow = ( side==Left ? max(j-offset,0)
                                        : max(j+height-width-offset,0) );
            int numZeroRows = RawLocalLength( firstRow, colShift, r );
            T* thisCol = &thisLocalBuffer[numZeroRows+j*thisLDim];
            for( int iLocal=0; iLocal<(localHeight-numZeroRows); ++iLocal )
                thisCol[iLocal] *= alpha;
        }
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::SetToIdentity()
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::SetToIdentity");
    this->AssertNotLockedView();
#endif

    const int width = this->Width();
    const int localHeight = this->LocalHeight();
    const int r = this->Grid().Height();
    const int colShift = this->ColShift();

    this->_localMatrix.SetToZero();

    T* thisLocalBuffer = this->LocalBuffer();
    const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for( int iLocal=0; iLocal<localHeight; ++iLocal )
    {
        const int i = colShift + iLocal*r;
        if( i < width )
            thisLocalBuffer[iLocal+i*thisLDim] = 1;
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,MC,Star>::SetToRandom()
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::SetToRandom");
    this->AssertNotLockedView();
#endif

    if( this->Grid().InGrid() )
    {
        const int width = this->Width();
        const int localHeight = this->LocalHeight();
        const int bufSize = localHeight*width;

        this->_auxMemory.Require( bufSize );

        // Create random matrix on process column 0, then broadcast
        T* buffer = this->_auxMemory.Buffer();
        if( this->_g->MRRank() == 0 )
        {
            for( int j=0; j<width; ++j )
                for( int iLocal=0; iLocal<localHeight; ++iLocal )
                    buffer[iLocal+j*localHeight] = Random<T>();
        }
        mpi::Broadcast( buffer, bufSize, 0, this->Grid().MRComm() );

        // Unpack
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int j=0; j<width; ++j )
        {
            const T* bufferCol = &buffer[j*localHeight];
            T* thisCol = &thisLocalBuffer[j*thisLDim];
            memcpy( thisCol, bufferCol, localHeight*sizeof(T) );
        }
        this->_auxMemory.Release();
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void 
elemental::DistMatrixBase<T,MC,Star>::SumOverRow()
{
#ifndef RELEASE
    PushCallStack("[MC,* ]::SumOverRow");
    this->AssertNotLockedView();
#endif

    if( this->Grid().InGrid() )
    {
        const int localHeight = this->LocalHeight(); 
        const int localWidth = this->LocalWidth();
        const int localSize = max( localHeight*localWidth, mpi::MIN_COLL_MSG );

        this->_auxMemory.Require( 2*localSize );
        T* buffer = this->_auxMemory.Buffer();
        T* sendBuf = &buffer[0];
        T* recvBuf = &buffer[localSize];

        // Pack
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            const T* thisCol = &thisLocalBuffer[jLocal*thisLDim];
            T* sendBufCol = &sendBuf[jLocal*localHeight];
            memcpy( sendBufCol, thisCol, localHeight*sizeof(T) );
        }

        // AllReduce sum
        mpi::AllReduce
        ( sendBuf, recvBuf, localSize, mpi::SUM, this->Grid().MRComm() );

        // Unpack
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            const T* recvBufCol = &recvBuf[jLocal*localHeight];
            T* thisCol = &thisLocalBuffer[jLocal*thisLDim];
            memcpy( thisCol, recvBufCol, localHeight*sizeof(T) );
        }
        this->_auxMemory.Release();
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
const DistMatrixBase<T,MC,Star>&
elemental::DistMatrixBase<T,MC,Star>::operator=
( const DistMatrixBase<T,MC,MR>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ] = [MC,MR]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    if( !this->Viewing() )
    {
        if( !this->ConstrainedColAlignment() )
        {
            this->_colAlignment = A.ColAlignment();
            if( g.InGrid() )
            {
                this->_colShift = 
                    Shift( g.MCRank(), this->ColAlignment(), g.Height() );
            }
        }
        this->ResizeTo( A.Height(), A.Width() );
    }

    if( g.InGrid() )
    {
        if( this->ColAlignment() == A.ColAlignment() )
        {
            if( A.Width() == 1 )
            {
                if( g.MRRank() == A.RowAlignment() )
                    this->_localMatrix = A.LockedLocalMatrix();

                // Communicate
                mpi::Broadcast
                ( this->_localMatrix.Buffer(), this->LocalHeight(),
                  A.RowAlignment(), g.MRComm() );
            }
            else
            {
                const int c = g.Width();

                const int width = this->Width();
                const int localHeight = this->LocalHeight();
                const int localWidthOfA = A.LocalWidth();
                const int maxLocalWidth = MaxLocalLength(width,c);

                const int portionSize = 
                    max(localHeight*maxLocalWidth,mpi::MIN_COLL_MSG);

                this->_auxMemory.Require( (c+1)*portionSize );

                T* buffer = this->_auxMemory.Buffer();
                T* originalData = &buffer[0];
                T* gatheredData = &buffer[portionSize];

                // Pack
                const T* ALocalBuffer = A.LockedLocalBuffer();
                const int ALDim = A.LocalLDim();
#ifdef _OPENMP
                #pragma omp parallel for
#endif
                for( int jLocal=0; jLocal<localWidthOfA; ++jLocal )
                {
                    const T* ACol = &ALocalBuffer[jLocal*ALDim];
                    T* originalDataCol = &originalData[jLocal*localHeight];
                    memcpy( originalDataCol, ACol, localHeight*sizeof(T) );
                }

                // Communicate
                mpi::AllGather
                ( originalData, portionSize,
                  gatheredData, portionSize, g.MRComm() );

                // Unpack
                const int rowAlignmentOfA = A.RowAlignment();
                T* thisLocalBuffer = this->LocalBuffer();
                const int thisLDim = this->LocalLDim();
#if defined(_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
                #pragma omp parallel for
#endif
                for( int k=0; k<c; ++k )
                {
                    const T* data = &gatheredData[k*portionSize];

                    const int rowShift = RawShift( k, rowAlignmentOfA, c );
                    const int localWidth = RawLocalLength( width, rowShift, c );

#if defined(_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
                    #pragma omp parallel for
#endif
                    for( int jLocal=0; jLocal<localWidth; ++jLocal )
                    {
                        const T* dataCol = &data[jLocal*localHeight];
                        T* thisCol = &thisLocalBuffer[(rowShift+jLocal*c)*thisLDim];
                        memcpy( thisCol, dataCol, localHeight*sizeof(T) );
                    }
                }
                this->_auxMemory.Release();
            }
        }
        else
        {
#ifdef UNALIGNED_WARNINGS
            if( g.VCRank() == 0 )
                cerr << "Unaligned [MC,* ] <- [MC,MR]." << endl;
#endif
            const int r = g.Height();
            const int c = g.Width();
            const int row = g.MCRank();

            const int colAlignment = this->ColAlignment();
            const int colAlignmentOfA = A.ColAlignment();
            const int sendRow = (row+r+colAlignment-colAlignmentOfA) % r;
            const int recvRow = (row+r+colAlignmentOfA-colAlignment) % r;

            if( A.Width()==1 )
            {
                const int localHeight = this->LocalHeight();

                if( this->_g->MRRank() == A.RowAlignment() )
                {
                    const int localHeightOfA = A.LocalHeight();

                    this->_auxMemory.Require( localHeightOfA );
                    T* buffer = this->_auxMemory.Buffer();

                    // Pack
                    const T* ACol = A.LockedLocalBuffer(0,0);
                    memcpy( buffer, ACol, localHeightOfA*sizeof(T) );

                    // Communicate
                    mpi::SendRecv
                    ( buffer, localHeightOfA, sendRow, 0,
                      this->_localMatrix.Buffer(), localHeight, recvRow, 
                      mpi::ANY_TAG, g.MCComm() );

                    this->_auxMemory.Release();
                }

                // Communicate
                mpi::Broadcast
                ( this->_localMatrix.Buffer(), localHeight, A.RowAlignment(),
                  g.MRComm() );
            }
            else
            {
                const int height = this->Height();
                const int width = this->Width();
                const int localHeight = this->LocalHeight();
                const int localHeightOfA = A.LocalHeight();
                const int localWidthOfA  = A.LocalWidth();
                const int maxLocalHeight = MaxLocalLength(height,r);
                const int maxLocalWidth  = MaxLocalLength(width,c);

                const int portionSize = 
                    max(maxLocalHeight*maxLocalWidth,mpi::MIN_COLL_MSG);

                this->_auxMemory.Require( (c+1)*portionSize );

                T* buffer = this->_auxMemory.Buffer();
                T* firstBuffer = &buffer[0];
                T* secondBuffer = &buffer[portionSize];

                // Pack the currently owned local data of A into the second 
                // buffer
                const T* ALocalBuffer = A.LockedLocalBuffer();
                const int ALDim = A.LocalLDim();
#ifdef _OPENMP
                #pragma omp parallel for
#endif
                for( int jLocal=0; jLocal<localWidthOfA; ++jLocal )
                {
                    const T* ACol = &ALocalBuffer[jLocal*ALDim];
                    T* secondBufferCol = &secondBuffer[jLocal*localHeightOfA];
                    memcpy( secondBufferCol, ACol, localHeightOfA*sizeof(T) );
                }

                // Perform the SendRecv: puts the new data into the first buffer
                mpi::SendRecv
                ( secondBuffer, portionSize, sendRow, 0,
                  firstBuffer,  portionSize, recvRow, mpi::ANY_TAG, 
                  g.MCComm() );

                // Use the output of the SendRecv as the input to the AllGather
                mpi::AllGather
                ( firstBuffer,  portionSize, 
                  secondBuffer, portionSize, g.MRComm() );

                // Unpack the contents of each member of the process row
                const int rowAlignmentOfA = A.RowAlignment();
                T* thisLocalBuffer = this->LocalBuffer();
                const int thisLDim = this->LocalLDim();
#if defined(_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
                #pragma omp parallel for
#endif
                for( int k=0; k<c; ++k )
                {
                    const T* data = &secondBuffer[k*portionSize];

                    const int rowShift = RawShift( k, rowAlignmentOfA, c ); 
                    const int localWidth = RawLocalLength( width, rowShift, c );
#if defined(_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
                    #pragma omp parallel for
#endif
                    for( int jLocal=0; jLocal<localWidth; ++jLocal )
                    {
                        const T* dataCol = &data[jLocal*localHeight];
                        T* thisCol = &thisLocalBuffer[(rowShift+jLocal*c)*thisLDim];
                        memcpy( thisCol, dataCol, localHeight*sizeof(T) );
                    }
                }
                this->_auxMemory.Release();
            }
        }
    }
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

// LEFT OFF HERE FOR IN/OUT OF GRID MODIFICATIONS

template<typename T>
const DistMatrixBase<T,MC,Star>&
elemental::DistMatrixBase<T,MC,Star>::operator=
( const DistMatrixBase<T,MC,Star>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ] = [MC,* ]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    if( !this->Viewing() )
    {
        if( !this->ConstrainedColAlignment() )
        {
            this->_colAlignment = A.ColAlignment();
            this->_colShift = A.ColShift();
        }
        this->ResizeTo( A.Height(), A.Width() );
    }

    if( this->ColAlignment() == A.ColAlignment() )
    {
        this->_localMatrix = A.LockedLocalMatrix();
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.VCRank() == 0 )
            cerr << "Unaligned [MC,* ] <- [MC,* ]." << endl;
#endif
        const int rank = g.MCRank();
        const int r = g.Height();

        const int colAlignment = this->ColAlignment();
        const int colAlignmentOfA = A.ColAlignment();

        const int sendRank = (rank+r+colAlignment-colAlignmentOfA) % r;
        const int recvRank = (rank+r+colAlignmentOfA-colAlignment) % r;

        const int width = this->Width();
        const int localHeight = this->LocalHeight();
        const int localHeightOfA = A.LocalHeight();

        const int sendSize = localHeightOfA * width;
        const int recvSize = localHeight * width;

        this->_auxMemory.Require( sendSize + recvSize );

        T* buffer = this->_auxMemory.Buffer();
        T* sendBuffer = &buffer[0];
        T* recvBuffer = &buffer[sendSize];

        // Pack
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int j=0; j<width; ++j )
        {
            const T* ACol = &ALocalBuffer[j*ALDim];
            T* sendBufferCol = &sendBuffer[j*localHeightOfA];
            memcpy( sendBufferCol, ACol, localHeightOfA*sizeof(T) );
        }

        // Communicate
        mpi::SendRecv
        ( sendBuffer, sendSize, sendRank, 0,
          recvBuffer, recvSize, recvRank, mpi::ANY_TAG, g.MCComm() );

        // Unpack
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int j=0; j<width; ++j )
        {
            const T* recvBufferCol = &recvBuffer[j*localHeight];
            T* thisCol = &thisLocalBuffer[j*thisLDim];
            memcpy( thisCol, recvBufferCol, localHeight*sizeof(T) );
        }
        this->_auxMemory.Release();
    }
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MC,Star>&
elemental::DistMatrixBase<T,MC,Star>::operator=
( const DistMatrixBase<T,Star,MR>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ] = [* ,MR]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    DistMatrix<T,MC,MR> A_MC_MR(true,false,this->ColAlignment(),0,g);

    A_MC_MR = A;
    *this   = A_MC_MR;
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MC,Star>&
elemental::DistMatrixBase<T,MC,Star>::operator=
( const DistMatrixBase<T,MD,Star>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ] = [MD,* ]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    throw logic_error( "[MC,* ] = [MD,* ] not yet implemented." );
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MC,Star>&
elemental::DistMatrixBase<T,MC,Star>::operator=
( const DistMatrixBase<T,Star,MD>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ] = [* ,MD]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    throw logic_error( "[MC,* ] = [* ,MD] not yet implemented." );
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MC,Star>&
elemental::DistMatrixBase<T,MC,Star>::operator=
( const DistMatrixBase<T,MR,MC>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ] = [MR,MC]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    auto_ptr< DistMatrix<T,VR,Star> > A_VR_Star
    ( new DistMatrix<T,VR,Star>(g) );
    *A_VR_Star = A;

    auto_ptr< DistMatrix<T,VC,Star> > A_VC_Star
    ( new DistMatrix<T,VC,Star>(true,this->ColAlignment(),g) );
    *A_VC_Star = *A_VR_Star;
    delete A_VR_Star.release(); // lowers memory highwater

    *this = *A_VC_Star;
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MC,Star>&
elemental::DistMatrixBase<T,MC,Star>::operator=
( const DistMatrixBase<T,MR,Star>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ] = [MR,* ]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    auto_ptr< DistMatrix<T,VR,Star> > A_VR_Star
    ( new DistMatrix<T,VR,Star>(g) );
    *A_VR_Star = A;

    auto_ptr< DistMatrix<T,VC,Star> > A_VC_Star
    ( new DistMatrix<T,VC,Star>(true,this->ColAlignment(),g) );
    *A_VC_Star = *A_VR_Star;
    delete A_VR_Star.release(); // lowers memory highwater

    *this = *A_VC_Star;
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MC,Star>&
elemental::DistMatrixBase<T,MC,Star>::operator=
( const DistMatrixBase<T,Star,MC>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ] = [* ,MC]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    auto_ptr< DistMatrix<T,MR,MC> > A_MR_MC( new DistMatrix<T,MR,MC>(g) );
    *A_MR_MC = A;

    auto_ptr< DistMatrix<T,VR,Star> > A_VR_Star
    ( new DistMatrix<T,VR,Star>(g) );
    *A_VR_Star = *A_MR_MC;
    delete A_MR_MC.release(); // lowers memory highwater

    auto_ptr< DistMatrix<T,VC,Star> > A_VC_Star
    ( new DistMatrix<T,VC,Star>(true,this->ColAlignment(),g) );
    *A_VC_Star = *A_VR_Star;
    delete A_VR_Star.release(); // lowers memory highwater

    *this = *A_VC_Star;
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MC,Star>&
elemental::DistMatrixBase<T,MC,Star>::operator=
( const DistMatrixBase<T,VC,Star>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ] = [VC,* ]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
#ifdef VECTOR_WARNINGS
    if( A.Width() == 1 && g.VCRank() == 0 )
    {
        cerr << 
          "The vector version of [MC,* ] <- [VC,* ] is not yet written, but"
          " it only requires a modification of the vector version of "
          "[* ,MR] <- [* ,VR]" << endl;
    }
#endif
#ifdef CACHE_WARNINGS
    if( A.Width() != 1 && g.VCRank() == 0 )
    {
        cerr << 
          "[MC,* ] <- [VC,* ] potentially causes a large amount of cache-"
          "thrashing. If possible avoid it by performing the redistribution"
          " with a (conjugate)-transpose: " << endl << 
          "  [* ,MC].(Conjugate)TransposeFrom([VC,* ])" << endl;
    }
#endif
    if( !this->Viewing() )
    {
        if( !this->ConstrainedColAlignment() )
        {
            this->_colAlignment = A.ColAlignment() % g.Height();
            this->_colShift = 
                Shift( g.MCRank(), this->ColAlignment(), g.Height() );
        }
        this->ResizeTo( A.Height(), A.Width() );
    }

    if( this->ColAlignment() == A.ColAlignment() % g.Height() )
    {
        const int r = g.Height();
        const int c = g.Width();
        const int p = r * c;
        const int row = g.MCRank();

        const int height = this->Height();
        const int width = this->Width();
        const int localHeightOfA = A.LocalHeight();
        const int maxLocalHeightOfA = MaxLocalLength(height,p);

        const int portionSize = max(maxLocalHeightOfA*width,mpi::MIN_COLL_MSG);

        this->_auxMemory.Require( (c+1)*portionSize );

        T* buffer = this->_auxMemory.Buffer();
        T* originalData = &buffer[0];
        T* gatheredData = &buffer[portionSize];

        // Pack 
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int j=0; j<width; ++j )
        {
            const T* ACol = &ALocalBuffer[j*ALDim];
            T* originalDataCol = &originalData[j*localHeightOfA];
            memcpy( originalDataCol, ACol, localHeightOfA*sizeof(T) );
        }

        // Communicate 
        mpi::AllGather
        ( originalData, portionSize,
          gatheredData, portionSize, g.MRComm() );

        // Unpack
        const int colShift = this->ColShift();
        const int colAlignmentOfA = A.ColAlignment();
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#if defined(_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( int k=0; k<c; ++k )
        {
            const T* data = &gatheredData[k*portionSize];    

            const int colShiftOfA = RawShift( row+r*k, colAlignmentOfA, p );
            const int colOffset = (colShiftOfA-colShift) / r;
            const int localHeight = RawLocalLength( height, colShiftOfA, p );

#if defined(_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for COLLAPSE(2)
#endif
            for( int j=0; j<width; ++j )
                for( int iLocal=0; iLocal<localHeight; ++iLocal )
                    thisLocalBuffer[(colOffset+iLocal*c)+j*thisLDim] = 
                        data[iLocal+j*localHeight];
        }
        this->_auxMemory.Release();
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.VCRank() == 0 )
            cerr << "Unaligned [MC,* ] <- [VC,* ]." << endl;
#endif
        const int r = g.Height();
        const int c = g.Width();
        const int p = r * c;
        const int row = g.MCRank();
        const int rank = g.VCRank();

        // Perform the SendRecv to make A have the same colAlignment
        const int colAlignment = this->ColAlignment();
        const int colAlignmentOfA = A.ColAlignment();
        const int colShift = this->ColShift();

        const int sendRank = (rank+p+colAlignment-colAlignmentOfA) % p;
        const int recvRank = (rank+p+colAlignmentOfA-colAlignment) % p;

        const int height = this->Height();
        const int width = this->Width();
        const int localHeightOfA = A.LocalHeight();
        const int maxLocalHeightOfA = MaxLocalLength(height,p);

        const int portionSize = max(maxLocalHeightOfA*width,mpi::MIN_COLL_MSG);

        this->_auxMemory.Require( (c+1)*portionSize );

        T* buffer = this->_auxMemory.Buffer();
        T* firstBuffer = &buffer[0];
        T* secondBuffer = &buffer[portionSize];

        // Pack
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int j=0; j<width; ++j )
        {
            const T* ACol = &ALocalBuffer[j*ALDim];
            T* secondBufferCol = &secondBuffer[j*localHeightOfA];
            memcpy( secondBufferCol, ACol, localHeightOfA*sizeof(T) );
        }

        // Perform the SendRecv: puts the new data into the first buffer
        mpi::SendRecv
        ( secondBuffer, portionSize, sendRank, 0,
          firstBuffer,  portionSize, recvRank, mpi::ANY_TAG, g.VCComm() );

        // Use the SendRecv as input to the AllGather
        mpi::AllGather
        ( firstBuffer,  portionSize,
          secondBuffer, portionSize, g.MRComm() );

        // Unpack
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#if defined(_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( int k=0; k<c; ++k )
        {
            const T* data = &secondBuffer[k*portionSize];

            const int colShiftOfA = RawShift( row+r*k, colAlignment, p );
            const int colOffset = (colShiftOfA-colShift) / r;
            const int localHeight = RawLocalLength( height, colShiftOfA, p );

#if defined(_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for COLLAPSE(2)
#endif
            for( int j=0; j<width; ++j )
                for( int iLocal=0; iLocal<localHeight; ++iLocal )
                    thisLocalBuffer[(colOffset+iLocal*c)+j*thisLDim] = 
                        data[iLocal+j*localHeight];
        }
        this->_auxMemory.Release();
    }
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MC,Star>&
elemental::DistMatrixBase<T,MC,Star>::operator=
( const DistMatrixBase<T,Star,VC>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ] = [* ,VC]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    auto_ptr< DistMatrix<T,Star,VR> > A_Star_VR
    ( new DistMatrix<T,Star,VR>(g) );
    *A_Star_VR = A;

    auto_ptr< DistMatrix<T,MC,MR> > A_MC_MR
    ( new DistMatrix<T,MC,MR>(true,false,this->ColAlignment(),0,g) );
    *A_MC_MR = *A_Star_VR;
    delete A_Star_VR.release(); // lowers memory highwater

    *this = *A_MC_MR;
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MC,Star>&
elemental::DistMatrixBase<T,MC,Star>::operator=
( const DistMatrixBase<T,VR,Star>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ] = [VR,* ]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    DistMatrix<T,VC,Star> A_VC_Star(true,this->ColAlignment(),g);

    A_VC_Star = A;
    *this = A_VC_Star;
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MC,Star>&
elemental::DistMatrixBase<T,MC,Star>::operator=
( const DistMatrixBase<T,Star,VR>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ] = [* ,VR]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    DistMatrix<T,MC,MR> A_MC_MR(true,false,this->ColAlignment(),0,g);

    A_MC_MR = A;
    *this = A_MC_MR;
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,MC,Star>&
elemental::DistMatrixBase<T,MC,Star>::operator=
( const DistMatrixBase<T,Star,Star>& A )
{
#ifndef RELEASE
    PushCallStack("[MC,* ] = [* ,* ]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    if( !this->Viewing() )
        this->ResizeTo( A.Height(), A.Width() );

    const int r = this->Grid().Height(); 
    const int colShift = this->ColShift();

    const int localHeight = this->LocalHeight();
    const int width = this->Width();

    const T* ALocalBuffer = A.LockedLocalBuffer();
    const int ALDim = A.LocalLDim();
    T* thisLocalBuffer = this->LocalBuffer();
    const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
    #pragma omp parallel for COLLAPSE(2)
#endif
    for( int j=0; j<width; ++j )
        for( int iLocal=0; iLocal<localHeight; ++iLocal )
            thisLocalBuffer[iLocal+j*thisLDim] = 
                ALocalBuffer[(colShift+iLocal*r)+j*ALDim];
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template class elemental::DistMatrixBase<int,   MC,Star>;
template class elemental::DistMatrixBase<float, MC,Star>;
template class elemental::DistMatrixBase<double,MC,Star>;
#ifndef WITHOUT_COMPLEX
template class elemental::DistMatrixBase<scomplex,MC,Star>;
template class elemental::DistMatrixBase<dcomplex,MC,Star>;
#endif
