/*
Author���w����
Date��  Aug., 2010
usage:  ��ꇲ�����G_J������
*/

#include <stdio.h>
#include <stdlib.h>

#include <math.h>

#include "mem.h"
#include "matrix.h"

/*
��˹�s����ⷽ��

a:  ���̽M��˂S�����,
b:  ���̽M�Ҷ˽���  (����������ꇣ��tb�錦��a�Ć�λ���)
arow: ���a���Д�
bcol: ���b���Д�
��õ����ꇴ����a��
*/
int gaussjordan(double **a, double **b, int arow, int bcol)
{
	double tmp;
	int i,j,k,l,ll;
	int icol, irow;
	int n = arow;
	int m = bcol;
	double big, dum, pivinv;
	int *indxc = (int *)calloc(n, sizeof(int));
	int *indxr = (int *)calloc(n, sizeof(int));
	int *ipiv  = (int *)calloc(n, sizeof(int));

	for (k=0; k<n; k++)
	{
		indxc[k] = 0;
		indxr[k] = 0;
		ipiv[k] = 0;		
	}

	for (i=0; i<n; i++)
	{
		big = 0.0;
		//get pivot element
		for (j=0; j<n; j++)
		{
			if (ipiv[j] != 1)
			{
				for (k=0; k<n; k++)
				{
					if (ipiv[k]==0)
						if (fabs(a[j][k]) >= big)
						{
							big = fabs(a[j][k]);
							irow= j;
							icol= k;
						}
				}
			}//if(ipiv[j]!=1)
		}//for(j=0; j<n; j++)
		++(ipiv[icol]);

		//interchange rows
		if (irow != icol)
		{
			for (l=0; l<n; l++)
			{
				//swap a[irow][l] and a[icol][l];
				tmp = a[irow][l];
				a[irow][l] = a[icol][l];
				a[icol][l] = tmp;
			}
			for (l=0; l<m; l++)
			{
				//swap b[irow][l] and b[icol][l];
				tmp = b[irow][l];
				b[irow][l] = b[icol][l];
				b[icol][l] = tmp;
			}
		}//if (irow != icol)
		indxr[i] = irow;
		indxc[i] = icol;

		//in case is a singular matrix
		if (a[icol][icol] == 0.0) 
		{
			if (indxc)
				free(indxc);
			if (indxr)
				free(indxr);
			if (ipiv)
				free(ipiv);
			return 0;
		}

		pivinv = 1.0/a[icol][icol];
		a[icol][icol] = 1.0;
		for (l=0; l<n; l++)
			a[icol][l] *= pivinv;
		for (l=0; l<m; l++)
			b[icol][l] *= pivinv;
		//a[icol][icol] = 1.0;

		//reduce the rows, except for the pivot element
		for (ll=0; ll<n; ll++)
		{
			if (ll!=icol)
			{
				dum = a[ll][icol];
				a[ll][icol] = 0.0;
				for (l=0; l<n; l++)
					a[ll][l] -= a[icol][l]*dum;
				for (l=0; l<m; l++)
					b[ll][l] -= b[icol][l]*dum;
			}
		}//for (ll=0; ll<n; ll++)
	}

	//this is the end of the main loop over columns of the reduction. it only remains to unscramble the solution
	//in view of the column interchanges. we do this by interchanging pairs of columns in the reverse order that 
	//the permutation was built up.
	for (l=n-1; l>=0; l--)
	{
		if (indxr[l]!=indxc[l])
			for (k=0; k<n; k++)
			{
				tmp = a[k][indxr[l]];
				a[k][indxr[l]] = a[k][indxc[l]];
				a[k][indxc[l]] = tmp;
			}
	}// and we are done

	if (indxc)
		free(indxc);
	if (indxr)
		free(indxr);
	if (ipiv)
		free(ipiv);

	return 1;
}

/*
Ӌ���ꇵ�����

srcMatrix:	ݔ��Դ��ꇣ���K��õ�����Ҳ���춴�
D; ��ꇌ���
*/
int inverse(double **srcMatrix, int D)
{
	int ret;
	double **iMatrix ;

	//Ӌ�������Ć�λ���
	mem2D_alloc_double(&iMatrix, D, D);
	identity(iMatrix, D);

	//GJ���
	ret = gaussjordan(srcMatrix, iMatrix, D, D);
		
	mem2D_free_double(iMatrix);

	return ret;
}

/*
Ӌ���λ���

iMatrix: Ҫ�õ��Ć�λ���
D;		 ��ꇌ���
*/
void identity(double **imatrix, int D)
{
	int i, j;
	for (j=0; j<D; j++)
		for (i=0; i<D; i++)
		{
			if (j==i)
				imatrix[j][i] = 1.0;
			else
				imatrix[j][i] = 0.0;
		}
}
