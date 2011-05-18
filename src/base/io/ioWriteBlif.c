/**CFile****************************************************************

  FileName    [ioWriteBlif.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to write BLIF files.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioWriteBlif.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ioAbc.h"
#include "main.h"
#include "mio.h"
#include "kit.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Io_NtkWrite( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches, int fBb2Wb, int fSeq );
static void Io_NtkWriteOne( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches, int fBb2Wb, int fSeq );
static void Io_NtkWritePis( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches );
static void Io_NtkWritePos( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches );
static void Io_NtkWriteSubckt( FILE * pFile, Abc_Obj_t * pNode );
static void Io_NtkWriteAsserts( FILE * pFile, Abc_Ntk_t * pNtk );
static void Io_NtkWriteNodeGate( FILE * pFile, Abc_Obj_t * pNode, int Length );
static void Io_NtkWriteNodeFanins( FILE * pFile, Abc_Obj_t * pNode );
static void Io_NtkWriteNode( FILE * pFile, Abc_Obj_t * pNode, int Length );
static void Io_NtkWriteLatch( FILE * pFile, Abc_Obj_t * pLatch );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Write the network into a BLIF file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteBlifLogic( Abc_Ntk_t * pNtk, char * FileName, int fWriteLatches )
{
    Abc_Ntk_t * pNtkTemp;
    // derive the netlist
    pNtkTemp = Abc_NtkToNetlist(pNtk);
    if ( pNtkTemp == NULL )
    {
        fprintf( stdout, "Writing BLIF has failed.\n" );
        return;
    }
    Io_WriteBlif( pNtkTemp, FileName, fWriteLatches, 0, 0 );
    Abc_NtkDelete( pNtkTemp );
}

/**Function*************************************************************

  Synopsis    [Write the network into a BLIF file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteBlif( Abc_Ntk_t * pNtk, char * FileName, int fWriteLatches, int fBb2Wb, int fSeq )
{
    FILE * pFile;
    Abc_Ntk_t * pNtkTemp;
    int i;
    assert( Abc_NtkIsNetlist(pNtk) );
    // start writing the file
    pFile = fopen( FileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Io_WriteBlif(): Cannot open the output file.\n" );
        return;
    }
    fprintf( pFile, "# Benchmark \"%s\" written by ABC on %s\n", pNtk->pName, Extra_TimeStamp() );
    // write the master network
    Io_NtkWrite( pFile, pNtk, fWriteLatches, fBb2Wb, fSeq );
    // make sure there is no logic hierarchy
    assert( Abc_NtkWhiteboxNum(pNtk) == 0 );
    // write the hierarchy if present
    if ( Abc_NtkBlackboxNum(pNtk) > 0 )
    {
        Vec_PtrForEachEntry( Abc_Ntk_t *, pNtk->pDesign->vModules, pNtkTemp, i )
        {
            if ( pNtkTemp == pNtk )
                continue;
            fprintf( pFile, "\n\n" );
            Io_NtkWrite( pFile, pNtkTemp, fWriteLatches, fBb2Wb, fSeq );
        }
    }
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Write the network into a BLIF file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWrite( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches, int fBb2Wb, int fSeq )
{
    Abc_Ntk_t * pExdc;
    assert( Abc_NtkIsNetlist(pNtk) );
    // write the model name
    fprintf( pFile, ".model %s\n", Abc_NtkName(pNtk) );
    // write the network
    Io_NtkWriteOne( pFile, pNtk, fWriteLatches, fBb2Wb, fSeq );
    // write EXDC network if it exists
    pExdc = Abc_NtkExdc( pNtk );
    if ( pExdc )
    {
        fprintf( pFile, "\n" );
        fprintf( pFile, ".exdc\n" );
        Io_NtkWriteOne( pFile, pExdc, fWriteLatches, fBb2Wb, fSeq );
    }
    // finalize the file
    fprintf( pFile, ".end\n" );
}

/**Function*************************************************************

  Synopsis    [Write one network.]

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
void Io_NtkWriteConvertedBox( FILE * pFile, Abc_Ntk_t * pNtk, int fSeq )
{
    Abc_Obj_t * pObj;
    int i, v;
    if ( fSeq )
    {
        fprintf( pFile, ".attrib white box seq\n" );
    }
    else
    {
        fprintf( pFile, ".attrib white box comb\n" );
        fprintf( pFile, ".delay 1\n" );
    }
    Abc_NtkForEachPo( pNtk, pObj, i )
    { 
        // write the .names line
        fprintf( pFile, ".names" );
        Io_NtkWritePis( pFile, pNtk, 1 );
        if ( fSeq )
            fprintf( pFile, " %s_in\n", Abc_ObjName(Abc_ObjFanin0(pObj)) );
        else
            fprintf( pFile, " %s\n", Abc_ObjName(Abc_ObjFanin0(pObj)) );
        for ( v = 0; v < Abc_NtkPiNum(pNtk); v++ )
            fprintf( pFile, "1" );
        fprintf( pFile, " 1\n" );
        if ( fSeq )
            fprintf( pFile, ".latch %s_in %s 1\n", Abc_ObjName(Abc_ObjFanin0(pObj)), Abc_ObjName(Abc_ObjFanin0(pObj)) );
    }
}

/**Function*************************************************************

  Synopsis    [Write one network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteOne( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches, int fBb2Wb, int fSeq )
{
    ProgressBar * pProgress;
    Abc_Obj_t * pNode, * pLatch;
    int i, Length;

    // write the PIs
    fprintf( pFile, ".inputs" );
    Io_NtkWritePis( pFile, pNtk, fWriteLatches );
    fprintf( pFile, "\n" );

    // write the POs
    fprintf( pFile, ".outputs" );
    Io_NtkWritePos( pFile, pNtk, fWriteLatches );
    fprintf( pFile, "\n" );

    // write the blackbox
    if ( Abc_NtkHasBlackbox( pNtk ) )
    {
        if ( fBb2Wb )
            Io_NtkWriteConvertedBox( pFile, pNtk, fSeq );
        else
            fprintf( pFile, ".blackbox\n" );
        return;
    }

    // write the timing info
    Io_WriteTimingInfo( pFile, pNtk );

    // write the latches
    if ( fWriteLatches && !Abc_NtkIsComb(pNtk) )
    {
        fprintf( pFile, "\n" );
        Abc_NtkForEachLatch( pNtk, pLatch, i )
            Io_NtkWriteLatch( pFile, pLatch );
        fprintf( pFile, "\n" );
    }

    // write the subcircuits
    assert( Abc_NtkWhiteboxNum(pNtk) == 0 );
    if ( Abc_NtkBlackboxNum(pNtk) > 0 )
    {
        fprintf( pFile, "\n" );
        Abc_NtkForEachBlackbox( pNtk, pNode, i )
            Io_NtkWriteSubckt( pFile, pNode );
        fprintf( pFile, "\n" );
    }

    // write each internal node
    Length = Abc_NtkHasMapping(pNtk)? Mio_LibraryReadGateNameMax((Mio_Library_t *)pNtk->pManFunc) : 0;
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        Io_NtkWriteNode( pFile, pNode, Length );
    }
    Extra_ProgressBarStop( pProgress );
}


/**Function*************************************************************

  Synopsis    [Writes the primary input list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWritePis( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches )
{
    Abc_Obj_t * pTerm, * pNet;
    int LineLength;
    int AddedLength;
    int NameCounter;
    int i;

    LineLength  = 7;
    NameCounter = 0;

    if ( fWriteLatches )
    {
        Abc_NtkForEachPi( pNtk, pTerm, i )
        {
            pNet = Abc_ObjFanout0(pTerm);
            // get the line length after this name is written
            AddedLength = strlen(Abc_ObjName(pNet)) + 1;
            if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
            { // write the line extender
                fprintf( pFile, " \\\n" );
                // reset the line length
                LineLength  = 0;
                NameCounter = 0;
            }
            fprintf( pFile, " %s", Abc_ObjName(pNet) );
            LineLength += AddedLength;
            NameCounter++;
        }
    }
    else
    {
        Abc_NtkForEachCi( pNtk, pTerm, i )
        {
            pNet = Abc_ObjFanout0(pTerm);
            // get the line length after this name is written
            AddedLength = strlen(Abc_ObjName(pNet)) + 1;
            if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
            { // write the line extender
                fprintf( pFile, " \\\n" );
                // reset the line length
                LineLength  = 0;
                NameCounter = 0;
            }
            fprintf( pFile, " %s", Abc_ObjName(pNet) );
            LineLength += AddedLength;
            NameCounter++;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Writes the primary input list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWritePos( FILE * pFile, Abc_Ntk_t * pNtk, int fWriteLatches )
{
    Abc_Obj_t * pTerm, * pNet;
    int LineLength;
    int AddedLength;
    int NameCounter;
    int i;

    LineLength  = 8;
    NameCounter = 0;

    if ( fWriteLatches )
    {
        Abc_NtkForEachPo( pNtk, pTerm, i )
        {
            pNet = Abc_ObjFanin0(pTerm);
            // get the line length after this name is written
            AddedLength = strlen(Abc_ObjName(pNet)) + 1;
            if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
            { // write the line extender
                fprintf( pFile, " \\\n" );
                // reset the line length
                LineLength  = 0;
                NameCounter = 0;
            }
            fprintf( pFile, " %s", Abc_ObjName(pNet) );
            LineLength += AddedLength;
            NameCounter++;
        }
    }
    else
    {
        Abc_NtkForEachCo( pNtk, pTerm, i )
        {
            pNet = Abc_ObjFanin0(pTerm);
            // get the line length after this name is written
            AddedLength = strlen(Abc_ObjName(pNet)) + 1;
            if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
            { // write the line extender
                fprintf( pFile, " \\\n" );
                // reset the line length
                LineLength  = 0;
                NameCounter = 0;
            }
            fprintf( pFile, " %s", Abc_ObjName(pNet) );
            LineLength += AddedLength;
            NameCounter++;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Write the latch into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteSubckt( FILE * pFile, Abc_Obj_t * pNode )
{
    Abc_Ntk_t * pModel = (Abc_Ntk_t *)pNode->pData;
    Abc_Obj_t * pTerm;
    int i;
    // write the subcircuit
//    fprintf( pFile, ".subckt %s %s", Abc_NtkName(pModel), Abc_ObjName(pNode) );
    fprintf( pFile, ".subckt %s", Abc_NtkName(pModel) );
    // write pairs of the formal=actual names
    Abc_NtkForEachPi( pModel, pTerm, i )
    {
        fprintf( pFile, " %s", Abc_ObjName(Abc_ObjFanout0(pTerm)) );
        pTerm = Abc_ObjFanin( pNode, i );
        fprintf( pFile, "=%s", Abc_ObjName(Abc_ObjFanin0(pTerm)) );
    }
    Abc_NtkForEachPo( pModel, pTerm, i )
    {
        fprintf( pFile, " %s", Abc_ObjName(Abc_ObjFanin0(pTerm)) );
        pTerm = Abc_ObjFanout( pNode, i );
        fprintf( pFile, "=%s", Abc_ObjName(Abc_ObjFanout0(pTerm)) );
    }
    fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    [Write the latch into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteLatch( FILE * pFile, Abc_Obj_t * pLatch )
{
    Abc_Obj_t * pNetLi, * pNetLo;
    int Reset;
    pNetLi = Abc_ObjFanin0( Abc_ObjFanin0(pLatch) );
    pNetLo = Abc_ObjFanout0( Abc_ObjFanout0(pLatch) );
    Reset  = (int)(ABC_PTRUINT_T)Abc_ObjData( pLatch );
    // write the latch line
    fprintf( pFile, ".latch" );
    fprintf( pFile, " %10s",    Abc_ObjName(pNetLi) );
    fprintf( pFile, " %10s",    Abc_ObjName(pNetLo) );
    fprintf( pFile, "  %d\n",   Reset-1 );
}


/**Function*************************************************************

  Synopsis    [Write the node into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteNode( FILE * pFile, Abc_Obj_t * pNode, int Length )
{
    if ( Abc_NtkHasMapping(pNode->pNtk) )
    {
        // write the .gate line
        fprintf( pFile, ".gate" );
        Io_NtkWriteNodeGate( pFile, pNode, Length );
        fprintf( pFile, "\n" );
    }
    else
    {
        // write the .names line
        fprintf( pFile, ".names" );
        Io_NtkWriteNodeFanins( pFile, pNode );
        fprintf( pFile, "\n" );
        // write the cubes
        fprintf( pFile, "%s", (char*)Abc_ObjData(pNode) );
    }
}

/**Function*************************************************************

  Synopsis    [Writes the primary input list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteNodeGate( FILE * pFile, Abc_Obj_t * pNode, int Length )
{
    Mio_Gate_t * pGate = (Mio_Gate_t *)pNode->pData;
    Mio_Pin_t * pGatePin;
    int i;
    // write the node
    fprintf( pFile, " %-*s ", Length, Mio_GateReadName(pGate) );
    for ( pGatePin = Mio_GateReadPins(pGate), i = 0; pGatePin; pGatePin = Mio_PinReadNext(pGatePin), i++ )
        fprintf( pFile, "%s=%s ", Mio_PinReadName(pGatePin), Abc_ObjName( Abc_ObjFanin(pNode,i) ) );
    assert ( i == Abc_ObjFaninNum(pNode) );
    fprintf( pFile, "%s=%s", Mio_GateReadOutName(pGate), Abc_ObjName( Abc_ObjFanout0(pNode) ) );
}

/**Function*************************************************************

  Synopsis    [Writes the primary input list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteNodeFanins( FILE * pFile, Abc_Obj_t * pNode )
{
    Abc_Obj_t * pNet;
    int LineLength;
    int AddedLength;
    int NameCounter;
    char * pName;
    int i;

    LineLength  = 6;
    NameCounter = 0;
    Abc_ObjForEachFanin( pNode, pNet, i )
    {
        // get the fanin name
        pName = Abc_ObjName(pNet);
        // get the line length after the fanin name is written
        AddedLength = strlen(pName) + 1;
        if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
        { // write the line extender
            fprintf( pFile, " \\\n" );
            // reset the line length
            LineLength  = 0;
            NameCounter = 0;
        }
        fprintf( pFile, " %s", pName );
        LineLength += AddedLength;
        NameCounter++;
    }

    // get the output name
    pName = Abc_ObjName(Abc_ObjFanout0(pNode));
    // get the line length after the output name is written
    AddedLength = strlen(pName) + 1;
    if ( NameCounter && LineLength + AddedLength > 75 )
    { // write the line extender
        fprintf( pFile, " \\\n" );
        // reset the line length
        LineLength  = 0;
        NameCounter = 0;
    }
    fprintf( pFile, " %s", pName );
}

/**Function*************************************************************

  Synopsis    [Writes the timing info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteTimingInfo( FILE * pFile, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    Abc_Time_t * pTime, * pTimeDef;
    int i;

    if ( pNtk->pManTime == NULL )
        return;

    pTimeDef = Abc_NtkReadDefaultArrival( pNtk );
    fprintf( pFile, ".default_input_arrival %g %g\n", pTimeDef->Rise, pTimeDef->Fall );
    Abc_NtkForEachPi( pNtk, pNode, i )
    {
        pTime = Abc_NodeReadArrival(pNode);
        if ( pTime->Rise == pTimeDef->Rise && pTime->Fall == pTimeDef->Fall )
            continue;
//        fprintf( pFile, ".input_arrival %s %g %g\n", Abc_ObjName(pNode), pTime->Rise, pTime->Fall );
        fprintf( pFile, ".input_arrival %s %g %g\n", Abc_ObjName(Abc_ObjFanout0(pNode)), pTime->Rise, pTime->Fall );
    }
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkConvertBb2Wb( char * pFileNameIn, char * pFileNameOut, int fSeq, int fVerbose )
{
    FILE * pFile;
    Abc_Ntk_t * pNetlist;
    // check the files
    pFile = fopen( pFileNameIn, "rb" );
    if ( pFile == NULL )
    {
        printf( "Input file \"%s\" cannot be opened.\n", pFileNameIn );
        return;
    }
    fclose( pFile );
    // check the files
    pFile = fopen( pFileNameOut, "wb" );
    if ( pFile == NULL )
    {
        printf( "Output file \"%s\" cannot be opened.\n", pFileNameOut );
        return;
    }
    fclose( pFile );
    // derive AIG for signal correspondence
    pNetlist = Io_ReadNetlist( pFileNameIn, Io_ReadFileType(pFileNameIn), 1 );
    if ( pNetlist == NULL )
    {
        printf( "Reading input file \"%s\" has failed.\n", pFileNameIn );
        return;
    }
    Io_WriteBlif( pNetlist, pFileNameOut, 1, 1, fSeq );
    Abc_NtkDelete( pNetlist );
}






/**Function*************************************************************

  Synopsis    [Transforms truth table into an SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Io_NtkDeriveSop( Mem_Flex_t * pMem, unsigned uTruth, int nVars )
{
    char * pSop;
    Vec_Int_t * vCover = Vec_IntAlloc( 100 );
    int RetValue = Kit_TruthIsop( &uTruth, nVars, vCover, 1 );
    assert( RetValue == 0 || RetValue == 1 );
    // check the case of constant cover
    assert( !(Vec_IntSize(vCover) == 0 || (Vec_IntSize(vCover) == 1 && Vec_IntEntry(vCover,0) == 0)) );
    // derive the AIG for that tree
    pSop = Abc_SopCreateFromIsop( pMem, nVars, vCover );
    if ( RetValue )
        Abc_SopComplement( pSop );
    Vec_IntFree( vCover );
    return pSop;
}

/**Function*************************************************************

  Synopsis    [Write the node into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteNodeInt( FILE * pFile, Abc_Obj_t * pNode )
{
    Abc_Obj_t * pNet;
    int i, nVars = Abc_ObjFaninNum(pNode);
    if ( nVars > 7 )
    {
        printf( "Node \"%s\" has more than 7 inputs. Writing BLIF has failed.\n", Abc_ObjName(Abc_ObjFanout0(pNode)) );
        return;
    }
    if ( nVars <= 4 )
    {
        // write the .names line
        fprintf( pFile, ".names" );
        Abc_ObjForEachFanin( pNode, pNet, i )
            fprintf( pFile, " %s", Abc_ObjName(pNet) );
        // get the output name
        fprintf( pFile, " %s\n", Abc_ObjName(Abc_ObjFanout0(pNode)) );
        // write the cubes
        fprintf( pFile, "%s", (char*)Abc_ObjData(pNode) );
    }
    else
    {
        extern int  If_Dec6PickBestMux( word t, word Cofs[2] );
        extern int  If_Dec7PickBestMux( word t[2], word c0r[2], word c1r[2] );
        extern word If_Dec6MinimumBase( word uTruth, int * pSupp, int nVarsAll, int * pnVars );
        extern void If_Dec7MinimumBase( word uTruth[2], int * pSupp, int nVarsAll, int * pnVars );
        extern word If_Dec6Perform( word t, int fDerive );
        extern word If_Dec7Perform( word t[2], int fDerive );

        char * pSop;
        word z, uTruth6, uTruth7[2], Cofs6[2], Cofs7[2][2];
        int c, iVar, nVarsMin[2], pVars[2][10];

        // collect variables
        Abc_ObjForEachFanin( pNode, pNet, i )
            pVars[0][i] = pVars[1][i] = i;

        // derive truth table
        if ( nVars == 7 )
        {
            Abc_SopToTruth7( (char*)Abc_ObjData(pNode), nVars, uTruth7 );
            iVar = If_Dec7PickBestMux( uTruth7, Cofs7[0], Cofs7[1] );
        }
        else
        {
            uTruth6 = Abc_SopToTruth( (char*)Abc_ObjData(pNode), nVars );
            iVar = If_Dec6PickBestMux( uTruth6, Cofs6 );
        }
        // perform MUX decomposition
        if ( iVar >= 0 )
        {
            if ( nVars == 7 )
            {
                If_Dec7MinimumBase( Cofs7[0], pVars[0], nVars, &nVarsMin[0] );
                If_Dec7MinimumBase( Cofs7[1], pVars[1], nVars, &nVarsMin[1] );
            }
            else
            {
                Cofs6[0] = If_Dec6MinimumBase( Cofs6[0], pVars[0], nVars, &nVarsMin[0] );
                Cofs6[1] = If_Dec6MinimumBase( Cofs6[1], pVars[1], nVars, &nVarsMin[1] );
            }
            assert( nVarsMin[0] < 5 );
            assert( nVarsMin[1] < 5 );
            // write cofactors
            for ( c = 0; c < 2; c++ )
            {
                pSop = Io_NtkDeriveSop( (Mem_Flex_t *)Abc_ObjNtk(pNode)->pManFunc, 
                    (unsigned)(nVars == 7 ? Cofs7[c][0] : Cofs6[c]), nVarsMin[c] );
                fprintf( pFile, ".names" );
                for ( i = 0; i < nVarsMin[c]; i++ )
                    fprintf( pFile, " %s", Abc_ObjName(Abc_ObjFanin(pNode,pVars[c][i])) );
                fprintf( pFile, " %s_cascade%d\n", Abc_ObjName(Abc_ObjFanout0(pNode)), c );
                fprintf( pFile, "%s", pSop );
            }
            // write MUX
            fprintf( pFile, ".names" );
            fprintf( pFile, " %s", Abc_ObjName(Abc_ObjFanin(pNode,iVar)) );
            fprintf( pFile, " %s_cascade0", Abc_ObjName(Abc_ObjFanout0(pNode)) );
            fprintf( pFile, " %s_cascade1", Abc_ObjName(Abc_ObjFanout0(pNode)) );
            fprintf( pFile, " %s\n", Abc_ObjName(Abc_ObjFanout0(pNode)) );
            fprintf( pFile, "1-1 1\n01- 1\n" );
            return;
        }

        // try cascade decomposition
        if ( nVars == 7 )
            z = If_Dec7Perform( uTruth7, 1 );
        else
            z = If_Dec6Perform( uTruth6, 1 );
        if ( z == 0 )
        {
            printf( "Node \"%s\" is not decomposable. Writing BLIF has failed.\n", Abc_ObjName(Abc_ObjFanout0(pNode)) );
            return;
        }

        // collect the nodes
        for ( c = 0; c < 2; i++ )
        {
            uTruth7[c]  = ((c ? z >> 32 : z) & 0xffff);
            uTruth7[c] |= (uTruth7[c] << 16);
            uTruth7[c] |= (uTruth7[c] << 32);
            for ( i = 0; i < 4; i++ )
                pVars[c][i] = (z >> (c*32+16+4*i)) & 7;

            // minimize truth table
            Cofs6[i] = If_Dec6MinimumBase( uTruth7[i], pVars[i], 4, &nVarsMin[i] );
            assert( c ? nVarsMin[0] == 4 : nVarsMin[1] <= 4 );

            // write the nodes
            pSop = Io_NtkDeriveSop( (Mem_Flex_t *)Abc_ObjNtk(pNode)->pManFunc, (unsigned)Cofs6[c], nVarsMin[c] );
            fprintf( pFile, ".names" );
            for ( i = 0; i < nVarsMin[c]; i++ )
                if ( pVars[c][i] == 7 )
                    fprintf( pFile, " %s_cascade", Abc_ObjName(Abc_ObjFanout0(pNode)) );
                else
                    fprintf( pFile, " %s", Abc_ObjName(Abc_ObjFanin(pNode,pVars[c][i])) );
            fprintf( pFile, " %s%s\n", Abc_ObjName(Abc_ObjFanout0(pNode)), c? "_cascade" : "" );
            fprintf( pFile, "%s", pSop );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Write the network into a BLIF file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteBlifInt( Abc_Ntk_t * pNtk, char * FileName )
{
    FILE * pFile;
    Abc_Obj_t * pNode, * pLatch;
    int i;
    assert( Abc_NtkIsNetlist(pNtk) );
    // start writing the file
    pFile = fopen( FileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Io_WriteBlifInt(): Cannot open the output file.\n" );
        return;
    }
    fprintf( pFile, "# Benchmark \"%s\" written by ABC on %s\n", pNtk->pName, Extra_TimeStamp() );
    // write the model name
    fprintf( pFile, ".model %s\n", Abc_NtkName(pNtk) );
    // write the PIs
    fprintf( pFile, ".inputs" );
    Io_NtkWritePis( pFile, pNtk, 1 );
    fprintf( pFile, "\n" );
    // write the POs
    fprintf( pFile, ".outputs" );
    Io_NtkWritePos( pFile, pNtk, 1 );
    fprintf( pFile, "\n" );
    // write the latches
    if ( Abc_NtkLatchNum(pNtk) )
        fprintf( pFile, "\n" );
    Abc_NtkForEachLatch( pNtk, pLatch, i )
        Io_NtkWriteLatch( pFile, pLatch );
    if ( Abc_NtkLatchNum(pNtk) )
        fprintf( pFile, "\n" );
    // write each internal node
    Abc_NtkForEachNode( pNtk, pNode, i )
        Io_NtkWriteNodeInt( pFile, pNode );
    // write the end
    fprintf( pFile, ".end\n\n" );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Write the network into a BLIF file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteBlifSpecial( Abc_Ntk_t * pNtk, char * FileName )
{
    Abc_Ntk_t * pNtkTemp;
    assert( Abc_NtkIsLogic(pNtk) );
    Abc_NtkToSop( pNtk, 0 );
    // derive the netlist
    pNtkTemp = Abc_NtkToNetlist(pNtk);
    if ( pNtkTemp == NULL )
    {
        fprintf( stdout, "Writing BLIF has failed.\n" );
        return;
    }
    Io_WriteBlifInt( pNtkTemp, FileName );
    Abc_NtkDelete( pNtkTemp );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

