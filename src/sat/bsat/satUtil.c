/**CFile****************************************************************

  FileName    [satUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [C-language MiniSat solver.]

  Synopsis    [Additional SAT solver procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: satUtil.c,v 1.4 2005/09/16 22:55:03 casem Exp $]

***********************************************************************/

#include <stdio.h>
#include <assert.h>
#include "satSolver.h"
#include "satSolver2.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Sat_SolverClauseWriteDimacs( FILE * pFile, clause * pC, int fIncrement );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Writes the given clause in a file in DIMACS format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverClauseWriteDimacs( FILE * pFile, clause * pC, int fIncrement )
{
    int i;
    for ( i = 0; i < (int)pC->size; i++ )
        fprintf( pFile, "%s%d ", (lit_sign(pC->lits[i])? "-": ""),  lit_var(pC->lits[i]) + (fIncrement>0) );
    if ( fIncrement )
        fprintf( pFile, "0" );
    fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    [Write the clauses in the solver into a file in DIMACS format.]

  Description [This procedure by default does not print binary clauses. To enable 
  printing of binary clauses, please set fUseBinaryClauses to 0 in the body of 
  procedure sat_solver_clause_new(sat_solver* s, lit* begin, lit* end, int learnt)
  in file "satSolver.c".]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverWriteDimacs( sat_solver * p, char * pFileName, lit* assumpBegin, lit* assumpEnd, int incrementVars )
{
    Sat_Mem_t * pMem = &p->Mem;
    FILE * pFile;
    clause * c;
    int i, k, nUnits;

    // count the number of unit clauses
    nUnits = 0;
    for ( i = 0; i < p->size; i++ )
        if ( p->levels[i] == 0 && p->assigns[i] != 3 )
            nUnits++;

    // start the file
    pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Sat_SolverWriteDimacs(): Cannot open the ouput file.\n" );
        return;
    }
//    fprintf( pFile, "c CNF generated by ABC on %s\n", Extra_TimeStamp() );
    fprintf( pFile, "p cnf %d %d\n", p->size, Sat_MemEntryNum(&p->Mem, 0)-1+Sat_MemEntryNum(&p->Mem, 1)+nUnits+(int)(assumpEnd-assumpBegin) );

    // write the original clauses
    Sat_MemForEachClause( pMem, c, i, k )
        Sat_SolverClauseWriteDimacs( pFile, c, incrementVars );

    // write the learned clauses
//    Sat_MemForEachLearned( pMem, c, i, k )
//        Sat_SolverClauseWriteDimacs( pFile, c, incrementVars );

    // write zero-level assertions
    for ( i = 0; i < p->size; i++ )
        if ( p->levels[i] == 0 && p->assigns[i] != 3 ) // varX
            fprintf( pFile, "%s%d%s\n",
                     (p->assigns[i] == 1)? "-": "",    // var0
                     i + (int)(incrementVars>0),
                     (incrementVars) ? " 0" : "");

    // write the assump
    if (assumpBegin) {
        for (; assumpBegin != assumpEnd; assumpBegin++) {
            fprintf( pFile, "%s%d%s\n",
                     lit_sign(*assumpBegin)? "-": "",
                     lit_var(*assumpBegin) + (int)(incrementVars>0),
                     (incrementVars) ? " 0" : "");
        }
    }

    fprintf( pFile, "\n" );
    fclose( pFile );
} 
void Sat_Solver2WriteDimacs( sat_solver2 * p, char * pFileName, lit* assumpBegin, lit* assumpEnd, int incrementVars )
{
    Sat_Mem_t * pMem = &p->Mem;
    FILE * pFile;
    clause * c;
    int i, k, nUnits;

    // count the number of unit clauses
    nUnits = 0;
    for ( i = 0; i < p->size; i++ )
        if ( p->levels[i] == 0 && p->assigns[i] != 3 )
            nUnits++;

    // start the file
    pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Sat_SolverWriteDimacs(): Cannot open the ouput file.\n" );
        return;
    }
//    fprintf( pFile, "c CNF generated by ABC on %s\n", Extra_TimeStamp() );
    fprintf( pFile, "p cnf %d %d\n", p->size, Sat_MemEntryNum(&p->Mem, 0)-1+Sat_MemEntryNum(&p->Mem, 1)+nUnits+(int)(assumpEnd-assumpBegin) );

    // write the original clauses
    Sat_MemForEachClause2( pMem, c, i, k )
        Sat_SolverClauseWriteDimacs( pFile, c, incrementVars );

    // write the learned clauses
//    Sat_MemForEachLearned( pMem, c, i, k )
//        Sat_SolverClauseWriteDimacs( pFile, c, incrementVars );

    // write zero-level assertions
    for ( i = 0; i < p->size; i++ )
        if ( p->levels[i] == 0 && p->assigns[i] != 3 ) // varX
            fprintf( pFile, "%s%d%s\n",
                     (p->assigns[i] == 1)? "-": "",    // var0
                     i + (int)(incrementVars>0),
                     (incrementVars) ? " 0" : "");

    // write the assump
    if (assumpBegin) {
        for (; assumpBegin != assumpEnd; assumpBegin++) {
            fprintf( pFile, "%s%d%s\n",
                     lit_sign(*assumpBegin)? "-": "",
                     lit_var(*assumpBegin) + (int)(incrementVars>0),
                     (incrementVars) ? " 0" : "");
        }
    }

    fprintf( pFile, "\n" );
    fclose( pFile );
}   
  

/**Function*************************************************************

  Synopsis    [Writes the given clause in a file in DIMACS format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline double Sat_Wrd2Dbl( word w )  { return (double)(unsigned)(w&0x3FFFFFFF) + (double)(1<<30)*(unsigned)(w>>30); }

void Sat_SolverPrintStats( FILE * pFile, sat_solver * p )
{
    printf( "starts        : %16.0f\n", Sat_Wrd2Dbl(p->stats.starts) );
    printf( "conflicts     : %16.0f\n", Sat_Wrd2Dbl(p->stats.conflicts) );
    printf( "decisions     : %16.0f\n", Sat_Wrd2Dbl(p->stats.decisions) );
    printf( "propagations  : %16.0f\n", Sat_Wrd2Dbl(p->stats.propagations) );
//    printf( "inspects      : %10d\n", (int)p->stats.inspects );
//    printf( "inspects2     : %10d\n", (int)p->stats.inspects2 );
}

/**Function*************************************************************

  Synopsis    [Writes the given clause in a file in DIMACS format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_Solver2PrintStats( FILE * pFile, sat_solver2 * s )
{
    printf( "starts        : %10d\n", (int)s->stats.starts );
    printf( "conflicts     : %10d\n", (int)s->stats.conflicts );
    printf( "decisions     : %10d\n", (int)s->stats.decisions );
    printf( "propagations  : %10d\n", (int)s->stats.propagations );
//    printf( "inspects      : %10d\n", (int)s->stats.inspects );
//    printf( "inspects2     : %10d\n", (int)s->stats.inspects2 );
/*
    printf( "memory for variables %.1f MB (free %6.2f %%) and clauses %.1f MB (free %6.2f %%)\n", 
        1.0 * Sat_Solver2GetVarMem(s) * s->size / (1<<20),
        100.0 * (s->cap - s->size) / s->cap,
        4.0 * (s->clauses.cap + s->learnts.cap) / (1<<20),
        100.0 * (s->clauses.cap - s->clauses.size + 
                 s->learnts.cap - s->learnts.size) / 
                (s->clauses.cap + s->learnts.cap) );
*/
}

/**Function*************************************************************

  Synopsis    [Returns the number of bytes used for each variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sat_Solver2GetVarMem( sat_solver2 * s )
{
    int Mem = 0;
    Mem += (sizeof(s->activity[0]) == 4) ? sizeof(unsigned) : sizeof(double);  // activity
    Mem += 2 * sizeof(veci); // wlists
    Mem += sizeof(int);      // vi (variable info)
    Mem += sizeof(int);      // trail
    Mem += sizeof(int);      // orderpos
    Mem += sizeof(int);      // reasons
    Mem += sizeof(int);      // units
    Mem += sizeof(int);      // order
    Mem += sizeof(int);      // model
    return Mem;
}

/**Function*************************************************************

  Synopsis    [Returns a counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Sat_SolverGetModel( sat_solver * p, int * pVars, int nVars )
{
    int * pModel;
    int i;
    pModel = ABC_CALLOC( int, nVars+1 );
    for ( i = 0; i < nVars; i++ )
        pModel[i] = sat_solver_var_value(p, pVars[i]);
    return pModel;    
}

/**Function*************************************************************

  Synopsis    [Returns a counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Sat_Solver2GetModel( sat_solver2 * p, int * pVars, int nVars )
{
    int * pModel;
    int i;
    pModel = ABC_CALLOC( int, nVars+1 );
    for ( i = 0; i < nVars; i++ )
        pModel[i] = sat_solver2_var_value(p, pVars[i]);
    return pModel;    
}

/**Function*************************************************************

  Synopsis    [Duplicates all clauses, complements unit clause of the given var.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverDoubleClauses( sat_solver * p, int iVar )
{
    assert( 0 );
/*
    clause * pClause;
    lit Lit, * pLits;
    int RetValue, nClauses, nVarsOld, nLitsOld, nLits, c, v;
    // get the number of variables
    nVarsOld = p->size;
    nLitsOld = 2 * p->size;
    // extend the solver to depend on two sets of variables
    sat_solver_setnvars( p, 2 * p->size );
    // duplicate implications
    for ( v = 0; v < nVarsOld; v++ )
        if ( p->assigns[v] != l_Undef )
        {
            Lit = nLitsOld + toLitCond( v, p->assigns[v]==l_False );
            if ( v == iVar )
                Lit = lit_neg(Lit);
            RetValue = sat_solver_addclause( p, &Lit, &Lit + 1 );
            assert( RetValue );
        }
    // duplicate clauses
    nClauses = vecp_size(&p->clauses);
    for ( c = 0; c < nClauses; c++ )
    {
        pClause = (clause *)p->clauses.ptr[c];
        nLits = clause_size(pClause);
        pLits = clause_begin(pClause);
        for ( v = 0; v < nLits; v++ )
            pLits[v] += nLitsOld;
        RetValue = sat_solver_addclause( p, pLits, pLits + nLits );
        assert( RetValue );
        for ( v = 0; v < nLits; v++ )
            pLits[v] -= nLitsOld;
    }
*/
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

