/*
   Copyright (c) 2009-2013, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef ELEM_LAPACK_CHOLESKY_LVAR2_HPP
#define ELEM_LAPACK_CHOLESKY_LVAR2_HPP

#include "elemental/blas-like/level3/Gemm.hpp"
#include "elemental/blas-like/level3/Herk.hpp"
#include "elemental/blas-like/level3/Trsm.hpp"

#include "elemental/lapack-like/Cholesky/LVar3.hpp"

// TODO: Reverse variants

namespace elem {
namespace cholesky {

template<typename F> 
inline void
LVar2( Matrix<F>& A )
{
#ifndef RELEASE
    CallStackEntry entry("cholesky::LVar2");
    if( A.Height() != A.Width() )
        LogicError("Can only compute Cholesky factor of square matrices");
#endif
    const Int n = A.Height();
    const Int bsize = Blocksize();
    for( Int k=0; k<n; k+=bsize )
    {
        const Int nb = std::min(bsize,n-k);
        auto A10 = View( A, k,    0, nb,       k  );
        auto A11 = View( A, k,    k, nb,       nb );
        auto A20 = View( A, k+nb, 0, n-(k+nb), k  );
        auto A21 = View( A, k+nb, k, n-(k+nb), nb );

        Herk( LOWER, NORMAL, F(-1), A10, F(1), A11 );
        cholesky::LVar3Unb( A11 );
        Gemm( NORMAL, ADJOINT, F(-1), A20, A10, F(1), A21 );
        Trsm( RIGHT, LOWER, ADJOINT, NON_UNIT, F(1), A11, A21 );
    }
}

template<typename F> 
inline void
LVar2( DistMatrix<F>& A )
{
#ifndef RELEASE
    CallStackEntry entry("cholesky::LVar2");
    if( A.Height() != A.Width() )
        LogicError("Can only compute Cholesky factor of square matrices");
#endif
    const Grid& g = A.Grid();
    DistMatrix<F,MR,  STAR> A10Adj_MR_STAR(g);
    DistMatrix<F,STAR,STAR> A11_STAR_STAR(g);
    DistMatrix<F,VC,  STAR> A21_VC_STAR(g);
    DistMatrix<F,MC,  STAR> X11_MC_STAR(g);
    DistMatrix<F,MC,  STAR> X21_MC_STAR(g);

    const Int n = A.Height();
    const Int bsize = Blocksize();
    for( Int k=0; k<n; k+=bsize )
    {
        const Int nb = std::min(bsize,n-k);
        auto A10 = View( A, k,    0,    nb,       k        );
        auto A11 = View( A, k,    k,    nb,       nb       );
        auto A20 = View( A, k+nb, 0,    n-(k+nb), k        );
        auto A21 = View( A, k+nb, k+nb, n-(k+nb), n-(k+nb) );
 
        A10Adj_MR_STAR.AlignWith( A10 );
        A10Adj_MR_STAR.AdjointFrom( A10 );
        X11_MC_STAR.AlignWith( A10 );
        LocalGemm( NORMAL, NORMAL, F(1), A10, A10Adj_MR_STAR, X11_MC_STAR );
        A11.SumScatterUpdate( F(-1), X11_MC_STAR );

        A11_STAR_STAR = A11;
        LocalCholesky( LOWER, A11_STAR_STAR );
        A11 = A11_STAR_STAR;

        X21_MC_STAR.AlignWith( A20 );
        LocalGemm( NORMAL, NORMAL, F(1), A20, A10Adj_MR_STAR, X21_MC_STAR );
        A21.SumScatterUpdate( F(-1), X21_MC_STAR );

        A21_VC_STAR = A21;
        LocalTrsm
        ( RIGHT, LOWER, ADJOINT, NON_UNIT, F(1), A11_STAR_STAR, A21_VC_STAR );
        A21 = A21_VC_STAR;
    }
}

} // namespace cholesky
} // namespace elem

#endif // ifndef ELEM_LAPACK_CHOLESKY_LVAR2_HPP
