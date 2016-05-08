/**CFile****************************************************************

  FileName    [acecUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [Various utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acecUtil.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acecInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_PolynCollectXors_rec( Gia_Man_t * pGia, int iObj, Vec_Int_t * vXors )
{
    Gia_Obj_t * pObj = Gia_ManObj( pGia, iObj );
    if ( Gia_ObjIsTravIdCurrent(pGia, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(pGia, pObj);
    if ( !Gia_ObjIsAnd(pObj) || !Gia_ObjIsXor(pObj) )
        return;
    Vec_IntPush( vXors, iObj );
    Gia_PolynCollectXors_rec( pGia, Gia_ObjFaninId0(pObj, iObj), vXors );
    Gia_PolynCollectXors_rec( pGia, Gia_ObjFaninId1(pObj, iObj), vXors );
}
void Gia_PolynAnalyzeXors( Gia_Man_t * pGia, int fVerbose )
{
    int i, iDriver, Count = 0;
    Vec_Int_t * vXors = Vec_IntAlloc( 100 );
    if ( pGia->pMuxes == NULL )
    {
        printf( "AIG does not have XORs extracted.\n" );
        return;
    }
    assert( pGia->pMuxes );
    Gia_ManForEachCoDriverId( pGia, iDriver, i )
    {
        Vec_IntClear( vXors );
        Gia_ManIncrementTravId( pGia );
        Gia_PolynCollectXors_rec( pGia, iDriver, vXors );
        //printf( "%3d : ", i );
        //Vec_IntPrint( vXors );
        printf( "%d=%d  ", i, Vec_IntSize(vXors) );
        Count += Vec_IntSize(vXors);
    }
    printf( "Total = %d.\n", Count );
    Vec_IntFree( vXors );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

