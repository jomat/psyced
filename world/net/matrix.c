// $Id: matrix.c,v 1.10 2005/03/14 10:23:26 lynx Exp $ // vim:syntax=lpc
//
// just a joke, really
//
#include <net.h>

#define CL_INDEX        (#'[)
#define CL_IF           (#'?)
#define CL_NIF          (#'?!)
#define CL_RANGE        (#'[..])
#define CL_L_RANGE      (#'[..)


protected int *dim(mixed M);
protected mixed TRANS(mixed M);
protected varargs mixed MpM(mixed A, mixed B, mixed C);
protected varargs mixed MxM(mixed A, mixed B, mixed C);
// delta(ij)
protected mixed E(int n);
// multipliziere die i-te Zeile mit x.. Ri(x) element M(nxn,K)
protected mixed R(int n, int i, int x);
// addiere das xfache der i-ten Zeile zur j-ten Zeile.. Qij(x) element M(nxn,K)
// WATCH OUT: von links!
protected mixed Q(int n, int i, int x, int j);

// vertauscht i-te und j-te zeile
protected mixed P(int n, int i, int j);

// BEWARE!! the mysterious sparcematrix optimizer! (M -> L)
//
// transforms a matrix into its aequivalent transformation.
// afterwards it can be used one-way.. funcall(L, some-matrix);
// the transformed matrix is much faster in case it contains many zeros
// and zero-lines.
//
// 
//
protected closure matrix2closure(mixed matrix);
// reverse
protected mixed closure2matrix(closure c);

protected string printMatrix(mixed matrix);
protected int spur(mixed matrix);
/*
 * Matrizen in der Form M[i][j].. , das ist per
 * Definition Mij. Also 
 * i Zeilen
 * j Spalten
 *
 *
 */
protected int *dim(mixed M) {
        int *dim, *row;

	if (closurep(M)) return funcall(M,"dim");
	dim = ({ 0,0 });
        dim[0] = sizeof(M);
        dim[1] = sizeof(M[0]);
        if(dim[0] == 0 || dim[1] == 0) return ({ -1,-1 });
	
        foreach (row : M) {
                if (sizeof(row) != dim[1])
                                        return ({ -1,-1 });
        }
        return dim;
}

protected mixed TRANS(mixed M) {
	mixed *T;
	int *dimM, wantClosure, i, j;
	
	if(closurep(M)) {
	    dimM = funcall(M,"dim");
	    M = closure2matrix(M);
	    wantClosure = 1;
	} else dimM = dim(M);
	
	if(dimM[0] == -1) return ({ -1,-1 }); 
	T = allocate( ({ dimM[1],dimM[0] }) );
    
	for (i = 0; i <= dimM[0] - 1; i++) { // loop rows of M
		for (j = 0; j <= dimM[1] - 1; j++) { // loop cols of M
		    T[j][i] = M[i][j];
		}
	}
	unless (wantClosure) return T;
	return matrix2closure(T);
}

// can be used as A * B = C
protected varargs mixed MxM(mixed A, mixed B, mixed C) {
	int *dimA, *dimB, *dimC, sum, i, j, k;
	
	if(closurep(B)) {
	    dimB = funcall(B,"dim");
	    B = closure2matrix(B);
	} else dimB = dim(B);
	
	if(closurep(A)) {
	    return funcall(A,B);
	}
	
	dimA = dim(A); // get dimensions to check whether 
	if (pointerp(C)) {
		dimC = dim(C);
		if (dimC[0] != dimB[0] || dimC[1] != dimA[1] || dimC[0] == -1) return C;
	} else {
		C = allocate( ({ dimA[0], dimB[1] }) );
	}
	if (dimA[1] != dimB[0] || dimB[0] == -1 || dimA[0] == -1) return C;
	// matrices are valid, we can start multiplication
	for (i = 0; i <= dimA[0] - 1; i++) { // loop rows of A
		for (j = 0; j <= dimB[1] - 1; j++) { // loop cols of B
			sum = 0;
			for (k = 0; k <= dimA[1] - 1; k++) { // vektor-produkt
				sum += A[i][k] * B[k][j];		
			}
			C[i][j] = sum;
		}
	}
	return C;
}

protected varargs mixed MpM(mixed A, mixed B, mixed C) {
	int *dimA, *dimB, *dimC, j, i;
	
	if (closurep(A)) {
	    dimA = funcall(A,"dim");
	    A = closure2matrix(A);
	} else dimA = dim(A); 
	
	if (closurep(B)) {
	    dimB = funcall(B,"dim");
	    B = closure2matrix(B);
	} else dimB = dim(B); 
	
	if (pointerp(C)) {
		dimC = dim(C);
		if (dimC[0] != dimB[0] || dimC[1] != dimB[1]) return C;
	} else {
		C = allocate( ({ dimB[0], dimB[1] }) );
	}
	if (dimA[0] != dimB[0] || dimA[1] != dimB[1]) return C;
	// matrices are valid, lets start add
	for (i = 0; i <= dimA[0] - 1; i++) { // loop
		for (j = 0; j <= dimB[1] - 1; j++) { // loop
		    C[i][j] = B[i][j] + A[i][j];
		} 
	}
	return C;
}


protected mixed E(int n) {
	int i;
	mixed matrix;
	
	matrix = allocate(({ n,n }));
	for(i = 0; i < n; i++) {
		matrix[i][i] = 1;
	}
	return matrix;
}

protected mixed R(int n, int i, int x) {
	mixed matrix;
	
	i--;
	matrix = E(n);
	if (i < 0 || i >= n)
		    return matrix;
	matrix[i][i] = x;
	return matrix;
}

protected mixed Q(int n, int i, int x, int j) {
	mixed matrix;
	
	i--; j--;
	matrix = E(n);
	if (i == j || i < 0 || j < 0 || j >= n || i >= n)
		    return matrix;
	// zeile j, spalte i.. Qij(x)
	matrix[j][i] = x;
	return matrix;
}

// exchanges j'th and i'th rows of the matrix
protected mixed P(int n, int i, int j) {
	mixed matrix;
	
	i--; j--;
	matrix = E(n);
	if (i == j || i < 0 || j < 0 || j >= n || i >= n)
		    return matrix;
	matrix[i][i] = 0;
	matrix[j][i] = 1;
	matrix[i][j] = 1;
	matrix[j][j] = 0;
	return matrix;
}


// closure gets the matrix.. we are working on a symbol!
// 'matrix
// in column 'j
// 
protected closure matrix2closure(mixed matrix) {
	int *dim = dim(matrix);
	int flag, i, j;
	mixed *c_array = ({ (#',) });
	mixed *temp;
	
	for (i = 0; i <= dim[0] - 1; i++) {
		flag = 0;
		temp = 0;
		for (j = 0; j <= dim[1] - 1; j++) {
			switch (matrix[i][j]) {
			case 0:
			    break;
			case 1:
			    flag = 1;
			    temp = ({ (#'+), ({ CL_INDEX, ({ CL_INDEX, 'matrix, j }), 'j }), temp });
			    break;
			default:
			    flag = 1;
			    temp = ({ (#'+), ({ #'*, matrix[i][j], ({ CL_INDEX, ({ CL_INDEX, 'matrix, j }), 'j }) }), temp });
			    break;
			}
		}
		if (flag == 1) {
		    c_array += ({ ({ #'=, ({ CL_INDEX, ({ CL_INDEX, 'matrix_out, i }), 'j }), temp }) });
		}
	}
//	P2(("%O",c_array))
	return lambda(({ 'matrix }),
		      ({ (#',),
		       ({ CL_IF, ({ #'==, 'matrix, "dim" }), ({ #'return, quote(dim) }) }),
		       ({ #'=, 'dim, ({ symbol_function("dim", ME), 'matrix }) }),
		       ({ #'=, 'alloc_dim, ({ #'({, dim[0], ({ CL_INDEX, 'dim, 1 }) }) }),
//		       ({ #'=, ({ CL_INDEX, 'alloc_dim, 1 }), ({ CL_INDEX, 'dim, 1 }) }),
		       ({ #'=, 'matrix_out, ({ #'allocate, 'alloc_dim }) }),
		       ({ CL_IF, ({ #'||, ({ #'==, ({ CL_INDEX, 'dim, 0 }), -1 }),
                                  ({ #'!=, ({ CL_INDEX, 'dim, 0 }), dim[1] }) }),
                        ({ #'return, 'matrix_out })
                        }),
		       ({ #'=, 'j, 0}),
		       ({ #'while,
			({ #'<= , 'j, ({ #'-, ({ CL_INDEX, 'dim, 1 }), 1 }) }),
			({ #'return, 'matrix_out }),
			c_array,
			({ #'++, 'j})
			})
		       })
		      );
}

// just multiply with E
protected mixed closure2matrix(closure c) {
    return funcall(c, E( funcall(c,"dim")[1] ) );
}

protected string printMatrix(mixed matrix) {
    string output;
    int *row;
    
    if (closurep(matrix)) matrix = closure2matrix(matrix);
    
    output = "";
    foreach (row : matrix) {
	output += "|\t"+ implode(map(row,#'to_string), "\t") +"\t|\n";
    }
    return output;
}

protected int spur(mixed matrix) {
    int *dim, c, n, spur;
    
    spur = 0;
    dim = dim(matrix);
    n = min(dim[0], dim[1]) - 1;
    for (c = 0; c <= n; c++) {
	spur += matrix[c][c];
    }
    return spur;
}

