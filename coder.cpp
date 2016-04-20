#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "coder.h"

#define GET_BLOCK_SIZE(_SIZE,_NUM) ((_SIZE+_NUM-(_SIZE%_NUM))/_NUM)

const int BUF_LEN = 256;
const int NAME_LEN = 32;
const char *FORMAT = "%s/%s_%d.dat";
matrix_t g_matrix;

void matrixDisPlay (matrix_t *A);

/************************************************************************
 * initMatrix - ��ʼ�����ɾ���g_matrix
 * @n : ���������
 * @m : ���������
 ************************************************************************/
void initMatrix (int n, int m) {
	int i, j;
	g_matrix.m_row = m;
	g_matrix.m_col = n;
	for (i = 0; i < m; ++i) {
		for (j = 0; j < n; ++j) {
			g_matrix.m_data[i][j] = galoisPow(j+1, i);
		}
	}
}

/************************************************************************
 * xorMulArr - ��src������Ԫ�س���ϵ��numȻ��ӵ�dest��ȥ
 * @num : src����Ҫ�˵�ϵ��
 * @src : һ������
 * @dest : һ������
 * @size : Ҫ�ӵ�dest����ȥ��Ԫ�صĸ���
 ************************************************************************/
void xorMulArr (UInt8_t num, UInt8_t *src, UInt8_t *dest, int size) {
	int i;
	if (!num) {
		return;
	}
	for (i = 0; i < size; ++i) {
		(*dest++) ^= galoisMul(num, *src++);
	}
}

/************************************************************************
 * dealbuffer - ������ļ��ж����ڴ��һ������
 * @context : ԭʼ�ļ���������
 * @c : �ڱ��������ʹ�õ����ڴ���Դ��m��������Ӧ����m������
 * @buf : ����Ŵ�ԭʼ�ļ��ж��뵽�ڴ��������
 * @m : Ҫ����������ĸ���
 * @buflen : buf�ĳ���
 * @offset : ��ǰbuf�е��������ļ��е�ƫ��λ��
 * @blen: ÿһ����ĳ���
 ************************************************************************/
void dealbuffer (const Context *context, UInt8_t **c, UInt8_t* buf, 
				 int m, int buflen, long offset, int blen) {
	int i, cursize, pos = 0;
	char name[NAME_LEN];
	FILE *fp;
	UInt8_t coef;

	// ���㵱ǰλ�ô����ĸ��飬���ڿ��е�ƫ����
	int bnum = offset / blen;
	int boff = offset % blen;

	while (pos < buflen) {
		// ��ȡһ��Ҫ���뵽��ǰblock�е����ݳ���
		// Ҫôbuffer������ Ҫô��ǰ�����
		if (blen - boff > buflen - pos) {
			cursize = buflen - pos;
		}
		else {
			cursize = blen - boff;
		}

		// ��buf��cursize������ͬ��٤�޻����㵽��Ӧ�Ŀ���
		for (i = 0; i < m; ++i) {
			coef = g_matrix.m_data[i][bnum];
			xorMulArr(coef, buf+pos, c[i]+boff, cursize);
		}
		
		// ��pos��pos+cursize������׷�ӵ�bnum�ļ���
		sprintf (name, FORMAT, context->dname, context->fname, bnum);
		fp = fopen(name, "ab+");
		fwrite (buf+pos, 1, cursize, fp);
		fclose (fp);

		// pos bnum boff ��Ӧ�ı���������������
		pos += cursize;
		bnum ++;
		boff = 0;
	}
}

/************************************************************************
 * clearBlock - �����һ���ļ���ص����п�
 * @context : ԭʼ�ļ���������
 * @num : �ļ���صķָ����������Ŀ�ܺ�
 ************************************************************************/
void clearBlock (const Context *context, int num) {
	int i;
	char name[NAME_LEN];
	for (i = 0; i < num; ++i) {
		sprintf (name, FORMAT, context->dname, context->fname, i);
		remove (name);
	}
}

/************************************************************************
 * encodeFile - ����һ���ļ�
 * @context : ԭʼ�ļ���������
 * @n : ԭʼ�ļ�Ҫ�ָ�Ŀ���Ŀ
 * @m : ԭʼ�ļ�Ҫ������������Ŀ
 ************************************************************************/
void encodeFile (const Context *context, int n, int m) {
	int i, num;
    long blen, offset;
	UInt8_t buffer[BUF_LEN];
	char name[NAME_LEN];
	FILE *pm;
	FILE *pf = fopen(context->fname, "rb");
	if (!pf) {
		return;
	}
	initMatrix(n, m);
	blen = GET_BLOCK_SIZE(context->flen,n);
	
	// �ڱ��������ʹ�õ����ڴ���Դ��m��������Ӧ����m������
	UInt8_t **c = (UInt8_t**)malloc(sizeof(int*)*m);
	for (i = 0; i < m; ++i) {
		c[i] = (UInt8_t*)malloc(sizeof(UInt8_t)*blen);
		memset (c[i], 0, sizeof(UInt8_t)*blen);
	}

	// ÿ�α���ǰ���ԭ�еĿ�
	clearBlock (context, m+n);
	for (i = 0; i < m+n; ++i) {
		sprintf (name, FORMAT, context->dname, context->fname, i);
		pm = fopen(name, "wb");
		fwrite (&context->flen, sizeof(int), 1, pm);
		fclose (pm);
	}
	// ��ԭʼ�ļ�������ݣ������ָ�ͱ��봦��
	// ÿ�ζ��뵽buffer�е����ݣ���dealbuffer������
	offset = 0;
	while (num = fread(buffer, sizeof(UInt8_t), BUF_LEN, pf)) {
		dealbuffer(context, c, buffer, m, num, offset, blen);
		offset += num*sizeof(UInt8_t);
	}

	// ������õ��������д�����
	for (i = 0; i < m; ++i) {
		sprintf (name, FORMAT, context->dname, context->fname, n+i);
		pm = fopen(name, "ab+");
		fwrite (c[i], sizeof(UInt8_t), blen, pm);
		fclose (pm);
	}

	// ���б��빤����ϣ��ͷ���Դ
	for (i = 0; i < m; ++i) {
		free (c[i]);
	}
	free (c);
	fclose (pf);
}

/************************************************************************
 * encodeFile - ����һ���ļ�
 * @context : ԭʼ�ļ���������
 * @n : ԭʼ�ļ�Ҫ�ָ�Ŀ���Ŀ
 * @m : ԭʼ�ļ�Ҫ������������Ŀ
 ************************************************************************/
void fillerase (UInt8_t **e, Context *context, int begin, int end, int n, int m) {
	int i, j, pos, num, coef;
	char name[NAME_LEN];
	UInt8_t buf[BUF_LEN];
	FILE *fp;
	for (i = begin; i < end; ++i) {
		sprintf (name, FORMAT, context->dname, context->fname, i);
		fp = fopen (name, "rb+");
		if (!fp) {
			continue;
		}
		fseek(fp, sizeof(int), SEEK_SET);
		pos = 0;
		while (num = fread(buf, 1, BUF_LEN, fp)) {
			for (j = 0; j < m; ++j) {
				if (i >= n && j != i-n) {
					continue;
				}
				coef = (i >= n) ? 1 : g_matrix.m_data[j][i];
				xorMulArr(coef, buf, e[j]+pos, num);
			}
			pos += num;
		}
	}
}

/************************************************************************
 * getIndexInfo - ��ȡ�����Ѷ�ʧ��ԭʼ���ţ���δ��ʧ���������
 * @fname : ԭʼ�ļ���
 * @ib : ��������Ѷ�ʧ��ԭʼ��ı�ŵ�����
 * @ic : �������δ��ʧ�������ı�ŵ�����
 * @lenb : ����ib�ĳ���
 * @lenc : ���ic�ĳ���
 * @m : ���뷽���������ĸ���
 * @n : ���뷽����ԭʼ��ĸ���
 ************************************************************************/
void getIndexInfo(Context *context, int *ib, int *ic, int *lenb, 
				 int *lenc, int m, int n) {
	int i;
	FILE *fp;
	char name[NAME_LEN];
	for (i = 0; i < n+m; ++i) {
		sprintf (name, FORMAT,context->dname, context->fname, i);
		fp = fopen (name, "rb+");
		// lenb ��ʧ����ͨ�����
		if (!fp) {
			if (i < n) {
				ib[(*lenb)++] = i;
			}
			continue;
		}
		// lenc ���ڵı�������
		if (i >= n) {
			ic[(*lenc)++] = i;
		}
		fclose (fp);
	}
}

/************************************************************************
 * createMatrix - �����Ѷ�ʧ��ԭʼ���ź�δ��ʧ������Ź���ϵ������
 * @ib : ��������Ѷ�ʧ��ԭʼ��ı�ŵ�����
 * @ic : �������δ��ʧ�������ı�ŵ�����
 * @lenb : ����ib�ĳ���
 * @lenc : ���ic�ĳ���
 * @n : ���뷽����ԭʼ��ĸ���
 ************************************************************************/
matrix_t createMatrix (int *ib, int *ic, int lenb ,int lenc, int n) {
	int i, j;
	matrix_t A;
	A.m_row = lenb;
	A.m_col = lenb;
	for (j = 0; j < lenb; ++j) {
		for (i = 0; i < lenb; ++i) {
			A.m_data[j][i] = g_matrix.m_data[ic[j]-n][ib[i]];
		}
	}
	return A;
}

/************************************************************************
 * createMatrix - ����ϵ������������A��era������������Ѷ�ʧ�Ŀ�
 * @A : ϵ������������
 * @era : ÿ��������Ӧ���Ѿ����������
 * @fname : ԭʼ�ļ���
 * @ic : �������δ��ʧ�������ı�ŵ�����
 * @ib : ��������Ѷ�ʧ��ԭʼ��ı�ŵ�����
 * @lenb : ����ib�ĳ���
 * @lenc : ���ic�ĳ���
 * @blen : ÿ��block�ĳ���
 * @n : ���뷽����ԭʼ��ĸ���
 ************************************************************************/
void buildBlock(matrix_t *A, UInt8_t **era, Context *context, int *ic, int *ib, 
			int lenb, int lenc, int blen, int n) {
	int i, j, coef;
	char name[NAME_LEN];
	FILE *fp;
	UInt8_t *buf = (UInt8_t*)malloc(sizeof(UInt8_t)*blen);
	for (i = 0; i < lenb; ++i) {
		memset (buf, 0, sizeof(UInt8_t)*blen);
		sprintf (name, FORMAT, context->dname, context->fname, ib[i]);
		for (j = 0; j < lenc; ++j) {
			coef = A->m_data[i][j];
			xorMulArr(coef, era[ic[j]-n], buf, blen);
		}
		fp = fopen(name, "wb");
		fwrite (&(context->flen), sizeof(int), 1, fp);
		fwrite (buf, 1, blen, fp);
		fclose (fp);
	}
}

/************************************************************************
 * buildFile - ���Ѿ�����������п���ϳ�ԭʼ�ļ�
 * @context : ԭʼ�ļ������Ϣ
 * @n : ���뷽����Ҫ�ָ����ԭʼ��ĸ���
 ************************************************************************/
void buildFile (const Context *context, int n) {
	int i, num, cursize, curlen;
	char name[NAME_LEN];
	char buf[BUF_LEN];
	FILE *fp = fopen (context->fname, "wb");
	FILE *fb;
	curlen = 0;
	for (i = 0; i < n; ++i) {
		sprintf (name, FORMAT, context->dname, context->fname, i);
		printf ("name = %s\n", name);
		fb = fopen (name, "rb+");
		if (!fb) {
			printf ("decode failed!\n");
			return;
		}
		fseek(fb, sizeof(int), SEEK_SET);
		cursize = 0;
		while (num = fread(buf, 1, BUF_LEN, fb)) {
			if (curlen >= context->flen) {
				break;
			}
			if (curlen + num > context->flen) {
				cursize = context->flen - curlen;
			}
			else {
				cursize = num;
			}
            curlen += cursize;
			fwrite(buf, 1, cursize, fp);
		}
		fclose(fb);
	}
	fclose (fp);
}
/************************************************************************
 * createMatrix - ���������鶪ʧ���ٱ������ʧ�������
 * @context : ԭʼ�ļ�������
 * @era : ÿ��������Ӧ���Ѿ����������
 * @ib : ��������Ѷ�ʧ��ԭʼ��ı�ŵ�����
 * @ic : �������δ��ʧ�������ı�ŵ�����
 * @lenb : ����ib�ĳ���
 * @lenc : ����ic�ĳ���
 * @m : ���뷽���������ĸ���
 * @n : ���뷽����ԭʼ��ĸ���
 * @blen : ÿ��block�ĳ���
 ************************************************************************/
void buildBackups(const Context *context, UInt8_t **era, int *ib, int *ic, 
				int lenb, int lenc, int m, int n, int blen) {
	int i, j, flag, num, pos;
	char name[NAME_LEN];
	UInt8_t buf[BUF_LEN];
	FILE *fc, *fb;
	UInt8_t coef;
	
	for (i = 0; i < m; ++i) {
		flag = 0;
		// �жϵ�i��������Ƿ��Ѿ�����
		for (j = 0; j < lenc; ++j) {
			if (i == ic[j] - n) {
				flag = 1;
			}
		}
		if (flag) {
			continue;
		}
		// ��������ڣ��򽫶�ʧ����ͨ�鶼�������Ӧ��era��
		for (j = 0; j < lenb; ++j) {
			coef = g_matrix.m_data[i][ib[j]];
			sprintf (name, FORMAT, context->dname, context->fname, ib[j]);
			fb = fopen (name, "rb");
			fseek(fb, sizeof(int), SEEK_SET);
			pos = 0;
			while (num = fread(buf, sizeof(UInt8_t), BUF_LEN, fb)) {
				xorMulArr(coef, buf, era[i]+pos, num);
				pos += num;
			}
			fclose (fb);
		}
		sprintf(name, FORMAT, context->dname, context->fname, n+i);
		fc = fopen (name, "wb+");
		fwrite(&(context->flen), sizeof(int), 1, fc);
		fwrite(era[i], sizeof(UInt8_t), blen, fc);
		fclose (fc);
	}
}

void getFileLen (Context *context, int *ib, int lenb, int n) {
	int i, j, flag;
	char name[NAME_LEN];
	FILE *fp;
	flag = 0;
	for (i = 0; i < n; ++i) {
		for (j = 0; j < lenb; ++j) {
			if (ib[j] == i) {
				flag = 1;
				break;
			}
			else {
				flag = 0;
			}
		}
		if (!flag) {
			break;
		}
	}
	sprintf (name, FORMAT, context->dname, context->fname, i);
	fp = fopen (name, "rb+");
	fread (&(context->flen), sizeof(int), 1, fp);
	fclose (fp);		
}

/************************************************************************
 * decodeFile - ����ָ�������ĵ��ļ�
 * @context : ԭʼ�ļ�������
 * @m : ���뷽���������ĸ���
 * @n : ���뷽����ԭʼ��ĸ���
 ************************************************************************/
void decodeFile (Context *context, int n, int m) {
	int blen, i, lenb, lenc;
	matrix_t A,B;

	// ib�����Ŷ�ʧ����ͨ����
	// ic�����Ŵ��ڵ��������
	int *ib = (int*)malloc(sizeof(int)*(n+m));
	int *ic = (int*)malloc(sizeof(int)*(n+m));
	UInt8_t **era;
	lenb = lenc = 0;

	// ��ʼ�����ɾ���
	initMatrix(n, m);
	
	// ��ȡ��ʧ����ͨ���ţ��ʹ��ڵ��������
	getIndexInfo(context, ib, ic, &lenb, &lenc, m, n);

	printf ("ib = ");
	for (i = 0; i < lenb; ++i) {
		printf ("%d ", ib[i]);
	}
	printf ("\n");
	printf ("ic = ");
	for (i = 0; i < lenc; ++i) {
		printf ("%d ", ic[i]);
	}
	printf ("\n");

	getFileLen (context, ib, lenb, n);
	blen = GET_BLOCK_SIZE(context->flen, n);
	printf ("flen = %d, blen = %d\n", context->flen, blen);
	// ��ʧ����ͨ��������ڴ��ڵı����������޷�����
	if (lenb > lenc) {
		printf ("can not decode!");
		return;
	}

	// �������������ʹ�õ����ڴ棬��ͨ��fillerase��������������
	era = (UInt8_t**)malloc(sizeof(UInt8_t*)*m);
	for (i = 0; i < m; ++i) {
		era[i] = (UInt8_t*)malloc(sizeof(UInt8_t)*blen);
		memset (era[i], 0, sizeof(UInt8_t)*blen);
	}
	fillerase(era, context, 0, n, n, m);
	fillerase(era, context, n, n+m, n, m);

	// ͨ����ʧ����ͨ���źʹ��ڵ�������ţ���ȷ��С����A
	// ͨ����˹Լ����ȥ���������A�������
	A = createMatrix(ib, ic, lenb, lenc, n);
	B = matrixGauss(&A);

	// ������era������ˣ��ɼ���õ���ʧ����ͨ��
	buildBlock(&B, era, context, ic, ib, lenb, lenc, blen, n);
	
	// ����ԭʼ�ļ�
	buildFile(context, n);
	// ����б�ɾ��������飬����Ҫ���������
	if (lenc != m) {
		buildBackups(context, era, ib, ic, lenb, lenc, m, n, blen);
	}
	// ������ϣ��ͷ���Դ
	for (i = 0; i < m; ++i) { 
		printf ("i = %d\n", i);
		era[i];
		printf ("era[i] = %d\n",(void*)era[i]);
		free (era[i]);
	}
	free (era);
	free (ib);
	free (ic);
}

void help(void) {
	return;
}

int main (int argc, char **argv) {
	int ch;
	char *DIR_NAME = (char*)".";
	char *FILE_NAME;
	int flag = 0;
	int m = 8;
	int n = 2;
	galoisEightBitInit();
	while (-1 != (ch=getopt(argc, argv, "D:f:m:n:deh"))) {
		switch (ch) {
		case 'D': {
			DIR_NAME = optarg;
			break;
		}
		case 'f': {
			FILE_NAME = optarg;
			break;
		}
		case 'd': {
			flag = 1;
			break;
		}
		case 'e': {
			flag = 0;
			break;
		}
		case 'm': {
			m = atoi(optarg);
			break;
		}
		case 'n': {
			n = atoi(optarg);
			break;
		}
		default : {
			help();
			break;
		}
		}
	}
	Context context;
	context.fname = FILE_NAME;
	context.dname = DIR_NAME;
	if (!flag) {
		FILE *pf = fopen(context.fname, "rb");
		if (!pf) {
			return -1;
		}
		fseek (pf, 0, SEEK_END);
		context.flen = ftell(pf);
		fclose (pf);
		encodeFile (&context, m, n);
	}
	else {
		decodeFile (&context, m, n);
	}
	return 0;
}

