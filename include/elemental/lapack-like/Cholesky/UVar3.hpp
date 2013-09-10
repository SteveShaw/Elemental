/*
   Copyright (c) 2009-2013, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef ELEM_LAPACK_CHOLESKY_UVAR3_HPP
#define ELEM_LAPACK_CHOLESKY_UVAR3_HPP

#include "elemental/blas-like/level3/Herk.hpp"
#include "elemental/blas-like/level3/Trsm.hpp"

namespace elem {
namespace cholesky {

template<typename F>
inline void
UVar3Unb( Matrix<F>& A )
{
#ifndef RELEASE
    CallStackEntry entry("cholesky::UVar3Unb");
    if( A.Height() != A.Width() )
        LogicError("Can only compute Cholesky factor of square matrices");
#endif
    typedef BASE(F) R;

    const Int n = A.Height();
    const Int lda = A.LDim();
    F* ABuffer = A.Buffer();
    for( Int j=0; j<n; ++j )
    {
        R alpha = RealPart(ABuffer[j+j*lda]);
        if( alpha <= R(0) )
            LogicError("A was not numerically HPD");
        alpha = Sqrt( alpha );
        ABuffer[j+j*lda] = alpha;
        
        for( Int k=j+1; k<n; ++k )
            ABuffer[j+k*lda] /= alpha;

        for( Int k=j+1; k<n; ++k )
            for( Int i=j+1; i<=k; ++i )
                ABuffer[i+k*lda] -= Conj(ABuffer[j+i*lda])*ABuffer[j+k*lda];
    }
}

template<typename F>
inline void
ReverseUVar3Unb( Matrix<F>& A )
{
#ifndef RELEASE
    CallStackEntry entry("cholesky::ReverseUVar3Unb");
    if( A.Height() != A.Width() )
        LogicError("Can only compute Cholesky factor of square matrices");
#endif
    typedef BASE(F) R;

    const Int n = A.Height();
    const Int lda = A.LDim();
    F* ABuffer = A.Buffer();
    for( Int j=n-1; j>=0; --j )
    {
        R alpha = RealPart(ABuffer[j+j*lda]);
        if( alpha <= R(0) )
            LogicError("A was not numerically HPD");
        alpha = Sqrt( alpha );
        ABuffer[j+j*lda] = alpha;
        
        for( Int i=0; i<j; ++i )
            ABuffer[i+j*lda] /= alpha;

        for( Int i=0; i<j; ++i )
            for( Int k=i; k<j; ++k )
                ABuffer[i+k*lda] -= Conj(ABuffer[k+j*lda])*ABuffer[i+j*lda];
    }
}

template<typename F> 
inline void
UVar3( Matrix<F>& A )
{
#ifndef RELEASE
    CallStackEntry entry("cholesky::UVar3");
    if( A.Height() != A.Width() )
        LogicError("Can only compute Cholesky factor of square matrices");
#endif
    const Int n = A.Height();
    const Int bsize = Blocksize();
    for( Int k=0; k<n; k+=bsize )
    {
        const Int nb = std::min(bsize,n-k);
        auto A11 = View( A, k,    k,    nb,       nb       );
        auto A12 = View( A, k,    k+nb, nb,       n-(k+nb) );
        auto A22 = View( A, k+nb, k+nb, n-(k+nb), n-(k+nb) );

        cholesky::UVar3Unb( A11 );
        Trsm( LEFT, UPPER, ADJOINT, NON_UNIT, F(1), A11, A12 );
        Herk( UPPER, ADJOINT, F(-1), A12, F(1), A22 );
    }
}

template<typename F> 
inline void
ReverseUVar3( Matrix<F>& A )
{
#ifndef RELEASE
    CallStackEntry entry("cholesky::ReverseUVar3");
    if( A.Height() != A.Width() )
        LogicError("Can only compute Cholesky factor of square matrices");
#endif
    const Int n = A.Height();
    const Int bsize = Blocksize();
    for( Int k=0; k<n; k+=bsize )
    {
        const Int nb = std::min(bsize,n-k);
        auto A00 = View( A, 0,        0,        n-(k+nb), n-(k+nb) );
        auto A01 = View( A, 0,        n-(k+nb), n-(k+nb), nb       );
        auto A11 = View( A, n-(k+nb), n-(k+nb), nb,       nb       );

        cholesky::ReverseUVar3Unb( A11 );
        Trsm( RIGHT, UPPER, NORMAL, NON_UNIT, F(1), A11, A01 );
        Herk( UPPER, NORMAL, F(-1), A01, F(1), A00 );
    }
}


template<typename F> 
inline void
UVar3( DistMatrix<F>& A )
{
#ifndef RELEASE
    CallStackEntry entry("cholesky::UVar3");
    if( A.Height() != A.Width() )
        LogicError("Can only compute Cholesky factor of square matrices");
#endif
    const Grid& g = A.Grid();
    DistMatrix<F,STAR,STAR> A11_STAR_STAR(g);
    DistMatrix<F,STAR,VR  > A12_STAR_VR(g);
    DistMatrix<F,STAR,MC  > A12_STAR_MC(g);
    DistMatrix<F,STAR,MR  > A12_STAR_MR(g);

    const Int n = A.Height();
    const Int bsize = Blocksize();
    for( Int k=0; k<n; k+=bsize )
    {
        const Int nb = std::min(bsize,n-k);
        auto A11 = View( A, k,    k,    nb,       nb       );
        auto A12 = View( A, k,    k+nb, nb,       n-(k+nb) );
        auto A22 = View( A, k+nb, k+nb, n-(k+nb), n-(k+nb) );

        A11_STAR_STAR = A11;
        LocalCholesky( UPPER, A11_STAR_STAR );
        A11 = A11_STAR_STAR;

        A12_STAR_VR.AlignWith( A22 );
        A12_STAR_VR = A12;
        LocalTrsm
        ( LEFT, UPPER, ADJOINT, NON_UNIT, F(1), A11_STAR_STAR, A12_STAR_VR );

        A12_STAR_MC.AlignWith( A22 );
        A12_STAR_MC = A12_STAR_VR;
        A12_STAR_MR.AlignWith( A22 );
        A12_STAR_MR = A12_STAR_VR;
        LocalTrrk
        ( UPPER, ADJOINT, F(-1), A12_STAR_MC, A12_STAR_MR, F(1), A22 );
        A12 = A12_STAR_MR;
    }
}

template<typename F> 
inline void
ReverseUVar3( DistMatrix<F>& A )
{
#ifndef RELEASE
    CallStackEntry entry("cholesky::ReverseUVar3");
    if( A.Height() != A.Width() )
        LogicError("Can only compute Cholesky factor of square matrices");
#endif
    const Grid& g = A.Grid();
    DistMatrix<F,STAR,STAR> A11_STAR_STAR(g);
    DistMatrix<F,VC,  STAR> A01_VC_STAR(g);
    DistMatrix<F,VR,  STAR> A01_VR_STAR(g);
    DistMatrix<F,STAR,MC  > A01Trans_STAR_MC(g);
    DistMatrix<F,STAR,MR  > A01Adj_STAR_MR(g);

    const Int n = A.Height();
    const Int bsize = Blocksize();
    for( Int k=0; k<n; k+=bsize )
    {
        const Int nb = std::min(bsize,n-k);
        auto A00 = View( A, 0,        0,        n-(k+nb), n-(k+nb) );
        auto A01 = View( A, 0,        n-(k+nb), n-(k+nb), nb       );
        auto A11 = View( A, n-(k+nb), n-(k+nb), nb,       nb       );

        A11_STAR_STAR = A11;
        LocalReverseCholesky( UPPER, A11_STAR_STAR );
        A11 = A11_STAR_STAR;

        A01_VC_STAR.AlignWith( A00 );
        A01_VC_STAR = A01;
        LocalTrsm
        ( RIGHT, UPPER, NORMAL, NON_UNIT, F(1), A11_STAR_STAR, A01_VC_STAR );

        A01_VR_STAR.AlignWith( A00 );
        A01_VR_STAR = A01_VC_STAR; 
        A01Trans_STAR_MC.AlignWith( A00 );
        A01Trans_STAR_MC.TransposeFrom( A01_VC_STAR );
        A01Adj_STAR_MR.AlignWith( A00 );
        A01Adj_STAR_MR.AdjointFrom( A01_VR_STAR );
        LocalTrrk
        ( UPPER, TRANSPOSE, 
          F(-1), A01Trans_STAR_MC, A01Adj_STAR_MR, F(1), A00 );
        A01.TransposeFrom( A01Trans_STAR_MC );
    }
}

} // namespace cholesky
} // namespace elem

#endif // ifndef ELEM_LAPACK_CHOLESKY_UVAR3_HPP
