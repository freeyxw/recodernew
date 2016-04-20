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
 * initMatrix - 初始化生成矩阵g_matrix
 * @n : 矩阵的行数
 * @m : 矩阵的列数
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
 * xorMulArr - 将src数组中元素乘以系数num然后加到dest中去
 * @num : src数据要乘的系数
 * @src : 一组数据
 * @dest : 一组数据
 * @size : 要加到dest数组去的元素的个数
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
 * dealbuffer - 处理从文件中读入内存的一块数据
 * @context : 原始文件的上下文
 * @c : 在编码过程中使用到的内存资源，m个冗余块对应分配m个数组
 * @buf : 存放着从原始文件中读入到内存里的数据
 * @m : 要编码的冗余块的个数
 * @buflen : buf的长度
 * @offset : 当前buf中的数据在文件中的偏移位置
 * @blen: 每一个块的长度
 ************************************************************************/
void dealbuffer (const Context *context, UInt8_t **c, UInt8_t* buf, 
				 int m, int buflen, long offset, int blen) {
	int i, cursize, pos = 0;
	char name[NAME_LEN];
	FILE *fp;
	UInt8_t coef;

	// 计算当前位置处于哪个块，及在块中的偏移量
	int bnum = offset / blen;
	int boff = offset % blen;

	while (pos < buflen) {
		// 获取一次要编码到当前block中的数据长度
		// 要么buffer结束， 要么当前块结束
		if (blen - boff > buflen - pos) {
			cursize = buflen - pos;
		}
		else {
			cursize = blen - boff;
		}

		// 将buf中cursize的数据同个伽罗华计算到对应的块中
		for (i = 0; i < m; ++i) {
			coef = g_matrix.m_data[i][bnum];
			xorMulArr(coef, buf+pos, c[i]+boff, cursize);
		}
		
		// 将pos到pos+cursize的数据追加到bnum文件中
		sprintf (name, FORMAT, context->dname, context->fname, bnum);
		fp = fopen(name, "ab+");
		fwrite (buf+pos, 1, cursize, fp);
		fclose (fp);

		// pos bnum boff 相应改变继续处理后续数据
		pos += cursize;
		bnum ++;
		boff = 0;
	}
}

/************************************************************************
 * clearBlock - 清除和一个文件相关的所有块
 * @context : 原始文件的上下文
 * @num : 文件相关的分割块和冗余块数目总和
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
 * encodeFile - 编码一个文件
 * @context : 原始文件的上下文
 * @n : 原始文件要分割的块数目
 * @m : 原始文件要编码的冗余块数目
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
	
	// 在编码过程中使用到的内存资源，m个冗余块对应分配m个数组
	UInt8_t **c = (UInt8_t**)malloc(sizeof(int*)*m);
	for (i = 0; i < m; ++i) {
		c[i] = (UInt8_t*)malloc(sizeof(UInt8_t)*blen);
		memset (c[i], 0, sizeof(UInt8_t)*blen);
	}

	// 每次编码前清除原有的块
	clearBlock (context, m+n);
	for (i = 0; i < m+n; ++i) {
		sprintf (name, FORMAT, context->dname, context->fname, i);
		pm = fopen(name, "wb");
		fwrite (&context->flen, sizeof(int), 1, pm);
		fclose (pm);
	}
	// 从原始文件里读数据，并做分割和编码处理
	// 每次读入到buffer中的数据，由dealbuffer来处理
	offset = 0;
	while (num = fread(buffer, sizeof(UInt8_t), BUF_LEN, pf)) {
		dealbuffer(context, c, buffer, m, num, offset, blen);
		offset += num*sizeof(UInt8_t);
	}

	// 将编码得到的冗余块写入磁盘
	for (i = 0; i < m; ++i) {
		sprintf (name, FORMAT, context->dname, context->fname, n+i);
		pm = fopen(name, "ab+");
		fwrite (c[i], sizeof(UInt8_t), blen, pm);
		fclose (pm);
	}

	// 所有编码工作完毕，释放资源
	for (i = 0; i < m; ++i) {
		free (c[i]);
	}
	free (c);
	fclose (pf);
}

/************************************************************************
 * encodeFile - 编码一个文件
 * @context : 原始文件的上下文
 * @n : 原始文件要分割的块数目
 * @m : 原始文件要编码的冗余块数目
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
 * getIndexInfo - 获取所有已丢失的原始块编号，和未丢失的冗余块编号
 * @fname : 原始文件名
 * @ib : 存放所有已丢失的原始块的编号的数组
 * @ic : 存放所有未丢失的冗余块的编号的数组
 * @lenb : 数组ib的长度
 * @lenc : 暑促ic的长度
 * @m : 编码方案中冗余块的个数
 * @n : 编码方案中原始块的个数
 ************************************************************************/
void getIndexInfo(Context *context, int *ib, int *ic, int *lenb, 
				 int *lenc, int m, int n) {
	int i;
	FILE *fp;
	char name[NAME_LEN];
	for (i = 0; i < n+m; ++i) {
		sprintf (name, FORMAT,context->dname, context->fname, i);
		fp = fopen (name, "rb+");
		// lenb 丢失的普通块个数
		if (!fp) {
			if (i < n) {
				ib[(*lenb)++] = i;
			}
			continue;
		}
		// lenc 存在的编码块个数
		if (i >= n) {
			ic[(*lenc)++] = i;
		}
		fclose (fp);
	}
}

/************************************************************************
 * createMatrix - 根据已丢失的原始块编号和未丢失冗余块编号构造系数矩阵
 * @ib : 存放所有已丢失的原始块的编号的数组
 * @ic : 存放所有未丢失的冗余块的编号的数组
 * @lenb : 数组ib的长度
 * @lenc : 暑促ic的长度
 * @n : 编码方案中原始块的个数
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
 * createMatrix - 根据系数矩阵的逆矩阵A和era，来解码求解已丢失的块
 * @A : 系数矩阵的逆矩阵
 * @era : 每个冗余块对应的已经消项的数组
 * @fname : 原始文件名
 * @ic : 存放所有未丢失的冗余块的编号的数组
 * @ib : 存放所有已丢失的原始块的编号的数组
 * @lenb : 数组ib的长度
 * @lenc : 暑促ic的长度
 * @blen : 每个block的长度
 * @n : 编码方案中原始块的个数
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
 * buildFile - 将已经解码出的所有块组合成原始文件
 * @context : 原始文件相关信息
 * @n : 编码方案中要分割出的原始块的个数
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
 * createMatrix - 如果有冗余块丢失，再编码出丢失的冗余块
 * @context : 原始文件上下文
 * @era : 每个冗余块对应的已经消项的数组
 * @ib : 存放所有已丢失的原始块的编号的数组
 * @ic : 存放所有未丢失的冗余块的编号的数组
 * @lenb : 数组ib的长度
 * @lenc : 数组ic的长度
 * @m : 编码方案中冗余块的个数
 * @n : 编码方案中原始块的个数
 * @blen : 每个block的长度
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
		// 判断第i个冗余块是否已经存在
		for (j = 0; j < lenc; ++j) {
			if (i == ic[j] - n) {
				flag = 1;
			}
		}
		if (flag) {
			continue;
		}
		// 如果不存在，则将丢失的普通块都编码进对应的era处
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
 * decodeFile - 解码指定上下文的文件
 * @context : 原始文件上下文
 * @m : 编码方案中冗余块的个数
 * @n : 编码方案中原始块的个数
 ************************************************************************/
void decodeFile (Context *context, int n, int m) {
	int blen, i, lenb, lenc;
	matrix_t A,B;

	// ib数组存放丢失的普通块编号
	// ic数组存放存在的冗余块编号
	int *ib = (int*)malloc(sizeof(int)*(n+m));
	int *ic = (int*)malloc(sizeof(int)*(n+m));
	UInt8_t **era;
	lenb = lenc = 0;

	// 初始化生成矩阵
	initMatrix(n, m);
	
	// 获取丢失的普通块编号，和存在的冗余块编号
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
	// 丢失的普通块个数大于存在的编码块个数，无法编码
	if (lenb > lenc) {
		printf ("can not decode!");
		return;
	}

	// 分配消项过程中使用到的内存，并通过fillerase对冗余块进行消项
	era = (UInt8_t**)malloc(sizeof(UInt8_t*)*m);
	for (i = 0; i < m; ++i) {
		era[i] = (UInt8_t*)malloc(sizeof(UInt8_t)*blen);
		memset (era[i], 0, sizeof(UInt8_t)*blen);
	}
	fillerase(era, context, 0, n, n, m);
	fillerase(era, context, n, n+m, n, m);

	// 通过丢失的普通块编号和存在的冗余块编号，来确定小矩阵A
	// 通过高斯约旦消去法求出矩阵A的逆矩阵
	A = createMatrix(ib, ic, lenb, lenc, n);
	B = matrixGauss(&A);

	// 逆矩阵和era数组相乘，可计算得到丢失的普通快
	buildBlock(&B, era, context, ic, ib, lenb, lenc, blen, n);
	
	// 构造原始文件
	buildFile(context, n);
	// 如果有被删除的冗余块，则需要构造冗余块
	if (lenc != m) {
		buildBackups(context, era, ib, ic, lenb, lenc, m, n, blen);
	}
	// 解码完毕，释放资源
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

