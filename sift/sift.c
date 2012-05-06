/************************************************************************
* Author:	Zhao Lili <Beihang University, "zhao86.scholar@gmail.com">
* Version:	V3.3
* TIme:		Aug.,2010	  
************************************************************************/

#define _CRT_SECURE_NO_DEPRECATE

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#include "matrix.h"
#include "mem.h"
#include "global.h"

#define SIFT_API _declspec(dllexport)
#include "sift.h"

/*
sift

infile:		��������
imgdata:	ͼ������
width:		ͼ����
height:		ͼ��߶�

//information below can be input through UI
wid:		gauss���ģ��뾶Ϊgwid*sigma
db_img:		�Ƿ�doubleͼ��
sigma0:		������ͼ���ƽ������		
sigman:		����ԭʼͼ���Ѵ��ڵ�ƽ������
contr_thr:	�ͶԱȶ���ֵ=contr_thr/scales
sift_border_dist: ���߽����
ill_norm_thresh:  �wһ���ֵ
top_oct_reso:     �����ͼ��ֱ���

float wid=2, int db_img=0, float sigma0 =1.6f,\
float sigman=0.5f, float contr_thr=0.09f, int sift_border_dist=5, float ill_norm_thresh=0.2f
*/
FeatureVect* _sift_main(const char* infile, imgpix	*imgData, int width, int height, float wid, int db_img, float sigma0,\
						float sigman, float contr_thr, int sift_border_dist, float ill_norm_thresh, int top_oct_reso)
{
	int octave_num;								//����
	int w, h;
	int scales = SIFT_GAUSSPYR_INTVL;
	FeatureVect *featVect;

	ImgInfo *imgInf;
	float	****gaussimg;		//���gauss�˲�ͼ��,[octaves][scales][height][width]
	float	****dogimg;			//dogͼ��
	float	****magimg;
	float	****ortimg;
	
	//��ȡͼ������
	if (db_img)
	{
		imginfo_fill_dbl(imgData, &imgInf, width, height);
		w = width<<1;
		h = height<<1;
		sigman *= 2;
	}		
	else
	{
		imginfo_fill(imgData, &imgInf, width, height);
		w = width;
		h = height;
	}		
	
	//����߶ȿռ�����,���ͼ��ķֱ�����СΪtop_oct_reso
	octave_num = (int)(log(SIFT_MIN(w, h))/log(2)) - (int)(log(top_oct_reso)/log(2))+1;

	//Ϊgauss�˲�ͼ���DOGͼ��
	gauss_mem_alloc(octave_num, scales+3, imgInf, &gaussimg, &dogimg);
	
	//�ݶȷ��Ⱥͷ������ռ�
	mag_ort_mem_alloc(&magimg, &ortimg, octave_num, scales, w, h);
	
	//����õ�gauss�������ռ�
	gauss_space(wid, octave_num, scales, sigma0, sigman, gaussimg, imgInf);
	
	//����õ�dog�ռ�
	dog_space(octave_num, scales, dogimg, gaussimg, w, h);

	//�õ��ݶȷ��Ⱥͷ���
	cal_grad_mag_ori(magimg, ortimg, octave_num, scales, w, h, gaussimg);

	//���dog�ռ伫ֵ��
	featVect = scale_space_extrema(sift_border_dist, dogimg, octave_num, scales, w, h, contr_thr);

	//���㼫ֵ���ƽ������
	cal_feat_sigma(featVect, sigma0, scales);

	//�����һ���baseͼ��double��
	if (db_img)
		adjust_doubled_img(featVect);

	//����ؼ���������
	cal_feat_oris(featVect, magimg, ortimg, imgInf);

	//����������
	cal_descriptor(featVect, magimg, ortimg, imgInf, SIFT_HIST_WIDTH, SIFT_DES_HIST_BIN_NUM, ill_norm_thresh);

	//gauss�V���D���dog�D��ጷſ��g
	gauss_mem_free(octave_num, scales, gaussimg, dogimg);

	//�ͷ��ݶȷ��Ⱥͷ���ռ�
	mag_ort_mem_free(magimg, ortimg, octave_num, scales);

	//ጷ�ԭʼ�D�񣨻�double��ģ��ĈD����g
	imginfo_release(imgInf);

	////ጷ������c��l
	//free_feature_nodes(featVect);
	return featVect;
}

/*
ጷ������c��l

featVect: ��ጷ���l
*/
void free_feature_nodes(FeatureVect *featVect)
{
	FeatureNode *prev;
	FeatureNode *tail = featVect->cur_node;

	while(tail!=NULL)
	{
		free(tail->feat);
		prev = tail->prev;
		tail->prev = NULL;
		tail->next = NULL;
		free(tail);
		tail = prev;
	}
	featVect->cur_node = NULL;
	featVect->first = NULL;
	free(featVect);
}

/*
�����˹ͼ���ݶȷ��Ⱥͷ���

mag:  �ݶȿռ�
ort:  �ݶȷ���ռ�
octaves: ����
scales:  ʵ�����õĲ���,3
w:	  base���ͼ����
h:    base���ͼ��߶�
gaussimg: gaussͼ��
*/
int cal_grad_mag_ori(float ****magimg, float ****ortimg, int octaves, int scales, int w, int h, float ****gaussimg)
{
	int o, s, i, j;
	float dx, dy;
	w = w<<1;
	h = h<<1;
	for (o=0; o<octaves; o++)
	{
		w = w>>1;
		h = h>>1;
		for (s=1; s<=scales; s++)
		{
			for (j=1; j<h-1; j++)	//�߳���
			{
				for (i=1; i<w-1; i++)
				{
					dx = gaussimg[o][s][j][i+1]-gaussimg[o][s][j][i-1];
					dy = gaussimg[o][s][j-1][i]-gaussimg[o][s][j+1][i];
					magimg[o][s-1][j][i] = sqrt(dx*dx+dy*dy);
					if ((dx==0.0)&&(dy==0.0))
						ortimg[o][s-1][j][i] = 0.0f;
					else
					{
						ortimg[o][s-1][j][i] = atan2(dy, dx);
						//ortimg[o][s-1][j][i] = (ortimg[o][s-1][j][i]<0)?(ortimg[o][s-1][j][i]+SIFT_PI2):ortimg[o][s-1][j][i];
					}						
				}
			}
		}
		
	}
	return 0;
}

/*
Ϊ�ݶȺͷ������ռ�

mag:  �ݶȿռ�
ort:  �ݶȷ���ռ�
octaves: ����
scales:  ʵ�����õĲ���,3
w:	  base���ͼ����
h:    base���ͼ��߶�
*/
int mag_ort_mem_alloc(float *****mag, float *****ort, int octaves, int scales, int w, int h)
{
	int j;
	int mem = 0;

	w = w<<1;
	h = h<<1;

	*mag = (float ****)calloc(octaves, sizeof(float ***));
	*ort = (float ****)calloc(octaves, sizeof(float ***));

	for (j=0; j<octaves; j++)
	{
		w = w>>1;
		h = h>>1;
		mem += mem3D_alloc_float(&(*mag)[j], scales, h, w);
		mem += mem3D_alloc_float(&(*ort)[j], scales, h, w);
	}

	return mem;
}

/*
�ͷ��ݶȺͷ���ռ�

mag:  Ҫ�ͷŵ��ݶȿռ�
ort:  Ҫ�ͷŵ��ݶȷ���ռ�
octaves: ����
scales:  ʵ�����õĲ���,3
*/
void mag_ort_mem_free(float ****mag, float ****ort, int octaves, int scales)
{
	mem4D_free_float(mag, octaves, scales);
	mem4D_free_float(ort, octaves, scales);
}


/*
����������.����ṹ������Feature�ṹ���е�descriptor������

featVect:	�洢��ֵ��������Ϣ
magimg:    �ݶ�ֵͼ��
ortimg��   �ݶȷ���
imginf:    ԭʼͼ����Ϣ����ӱ���ģ�
d:		   ֱ��ͼ�����ȣ�Ĭ��Ϊ4
binnum:	   ÿ��ֱ��ͼ�ķ�������Ĭ��Ϊ8
*/
void cal_descriptor(FeatureVect *featVect, float ****magimg, float ****ortimg, ImgInfo *imginf, int d, int binnum, float thresh)
{
	int i;
	float ***hist;		//ֱ��ͼ��[r][c][octave]
	FeatureNode *node = featVect->first;
	Feature *feat;

	mem3D_alloc_float(&hist, d, d, binnum);
	while(node != NULL)
	{
		feat = node->feat;
		
		//����õ�hist
		cal_desc_hist(hist, magimg, ortimg, imginf, node, d, binnum);
		
		//3D->1D
		desc_1Dto3D(node, hist, d, binnum);
		
		//��descriptor�wһ��
		desc_norm(feat->descriptor, SIFT_DESC_LEN);
		
		//ȥ�����ݶ�ֵ5
		for (i=0; i<SIFT_DESC_LEN; i++)
			if (feat->descriptor[i]>thresh)
				feat->descriptor[i] = thresh;
		
		//�ٴΚwһ��
		desc_norm(feat->descriptor, SIFT_DESC_LEN);
		node = node->next;
	}

	mem3D_free_float(hist, d);
}
/*
�wһ��������,��descriptor�wһ������λ�L��

descriptor: ���wһ����������
len:        �������L�� Ĭ�J��128
*/
void desc_norm(float *descriptor, int len)
{
	int i;
	float sum=0.0f;
	for (i=0; i<len; i++)
		sum += descriptor[i]*descriptor[i];

	sum = fdsqrt(sum);
	for (i=0; i<len; i++)
		descriptor[i] /= sum;
}
/*
��3�Sֱ���D�b�Qλһ�S���惦��feature��

node:	��ǰ�P�I�c�Y�c
hist:	Ҫ�D�Q��ֱ���D
d:		ֱ���D����
binnum:	ֱ���D��bin�Ă���
*/
void desc_1Dto3D(FeatureNode *node, float ***hist, int d, int binnum)
{
	int j, i,t;
	Feature *feat = node->feat;

	for (j=0; j<d; j++)
	{
		for (i=0; i<d; i++)
		{
			t = (j*d+i)*binnum;
			memcpy(feat->descriptor+t, hist[j][i], binnum*sizeof(float));
		}
	}
}

/*
����õ�descriptor��ֱ��ͼ

hist:		�洢���[hang][lie][bin]		
magimg:     �ݶ�ֵͼ��
ortimg:     ����
imginf:     ԭʼͼ����Ϣ����ӱ���ģ�
Node:		��ǰ���
d:		    ֱ��ͼ�����ȣ�Ĭ��Ϊ4
binnum:	    ÿ��ֱ��ͼ�ķ�������Ĭ��Ϊ8
*/
void cal_desc_hist(float ***hist, float ****magimg, float ****ortimg, ImgInfo *imginf, FeatureNode *Node, int d, int binnum)
{
	int			i,j, times, radius;
	int			octave, s, x, y, w, h;
	int			r0, c0, o0, r, c, o, rb, cb, ob;
	double		cos_t, sin_t, r_rot, c_rot, rbin, cbin, obin;
	double		hist_width, mag, sigma, keyori,rot_ori, exp_denom, win, bins_per_rad;
	double		d_r, d_c, d_o, v_r, v_c, v_o;
	Feature *feat = Node->feat;

	octave = feat->octave;
	s = feat->scale;
	x = feat->x;
	y = feat->y;
	times = (int)fpow(2.0, octave);
	h = imginf->height/times;
	w = imginf->width/times;
	sigma		= feat->sig_octave;			//��ȷ���ǳ߶�
	keyori		= feat->orient;
	hist_width	= SIFT_DES_HIST_FCT*sigma;
	radius		= (int)(sqrt(2.0)*hist_width*(d+1.0)*0.5+0.5);	//��������ռͼ��뾶
	exp_denom	= d*d*0.5;					//2*0.5*d*0.5*d
	bins_per_rad= binnum/SIFT_PI2;

#define fast_floor(x) ((int)( x - ((x>=0)?0:1) ))

	//reset hist
	for (j=0; j<d; j++)
		for (i=0; i<d; i++)
			memset(hist[j][i], 0, sizeof(float)*binnum);

	//����������Ƕȵ����Ǻ���ֵ
	cos_t = cos(keyori);
	sin_t = sin(keyori);

	for (j=-radius; j<=radius; j++)
	{
		if ((y-j>h-2)||(y-j<1))
			continue;
		for (i=-radius; i<=radius; i++)
		{
			if ((x+i>w-2)||(x+i<1))   //����
				continue;

			//������ת������꣬�Թؼ���ΪԲ��,��ֱ��ͼΪ��λ
			c_rot = (j*cos_t-i*sin_t)/hist_width;
			r_rot = (j*sin_t+i*cos_t)/hist_width;
			rbin  = r_rot + d/2 -0.5;
			cbin  = c_rot + d/2 -0.5;

			//�ݶȺͽǶ�
			mag = magimg[octave][s-1][y-j][x+i];
			rot_ori = ortimg[octave][s-1][y-j][x+i];

			//�ڷ�Χ��
			if (rbin>-1.0 && rbin<d && cbin>-1.0 && cbin<d)
			{
				//���㵱ǰ������ת��Ƕ�
				rot_ori = rot_ori-keyori;
				while(rot_ori<0.0)
					rot_ori = rot_ori+SIFT_PI2;
				while(rot_ori>=SIFT_PI2)
					rot_ori = rot_ori-SIFT_PI2;

				//���㵱ǰ���ش���ֱ��ͼ�е�λ��
				obin = rot_ori*bins_per_rad;

				//��˹Ȩ
				win = exp(-(c_rot*c_rot+r_rot*r_rot)/exp_denom);

				//����ǰ���ص��ݶ�ֵvote��ֱ���D��8��bin��
				r0 = floor(rbin);
				c0 = floor(cbin);
				o0 = floor(obin);
				d_r = rbin-r0;
				d_c = cbin-c0;
				d_o = obin-o0;

				//trilinear interpolation
				for (r=0; r<=1; r++)
				{
					rb = r0 + r;
					if (rb>=0 && rb<d)
					{
						v_r =  win*mag*((r==0) ? 1.0-d_r : d_r);
						for (c=0; c<=1; c++)
						{
							cb = c0+c;
							if (cb>=0 && cb<d)
							{
								v_c = v_r*((c==0) ? 1.0-d_c : d_c);
								for (o=0; o<=1; o++)
								{
									ob = (o0+o)%binnum;
									if (ob>=binnum)
									{
										printf("");
									}
									v_o = v_c*((o==0) ? 1.0-d_o : d_o);
									hist[rb][cb][ob] += v_o;
								}
							}
						}						
					}
				}//for
			}
		}
	}
}


/*
����ؼ���������

featVect:	�洢��ֵ��������Ϣ
magimg:     �ݶȷ���ֵ
ortimg:     �ݶȷ���ֵ
imginf:    ԭʼͼ����Ϣ����ӱ���ģ�
*/
void cal_feat_oris(FeatureVect *featVect, float ****magimg, float ****ortimg, ImgInfo *imginf)
{
	int i;
	int	x, y, w, h, times;
	int	octave, scale;
	int 		rad;		//����ؼ���������ʱ��region�뾶
	float		sig;		//�ڵ�ǰ��ľ�ȷ�߶�
	Feature		*feat;
	FeatureNode *node;
	double *hist = (double *)calloc(SIFT_ORI_HIST_BIN_NUM, sizeof(double));
	double omax;		//ֱ��ͼ����ֵ

	w = imginf->width;
	h = imginf->height;
	node = featVect->first;
	while(node!=NULL)
	{
		feat	= node->feat;
		sig		= SIFT_ORI_SIG*feat->sig_octave;
		rad		= (int)(SIFT_ORI_RADIUS*feat->sig_octave + 0.5);

		//���㵱ǰ�ؼ�����ݶ�ֱ��ͼ
		x = feat->x;
		y = feat->y;
		octave = feat->octave;
		scale  = feat->scale;
		times  = (int)fpow(2.0, octave);
		w = w/times;
		h = h/times;
		cal_hist(magimg[octave][scale-1], ortimg[octave][scale-1], y, x, rad, sig, hist, w, h);

		//ƽ������ֱ��ͼ
		for (i=0; i<SIFT_SMOOTH_HIST_LEVEL; i++)
			smooth_hist(hist, SIFT_ORI_HIST_BIN_NUM);

		//����ֱ��ͼ��������ֵ
		omax = domin_mag(hist, SIFT_ORI_HIST_BIN_NUM);

		//����ؼ��������򣬸������� ������һ������ָ��
		node = add_dominated_ori(hist, omax*SIFT_HIST_PEAK_RATIO, SIFT_ORI_HIST_BIN_NUM, node, featVect);
	}
	free(hist);
}

/*
����ؼ��������򣬲�����ֵ>=omax�ķ���Ҳ��Ϊ�ؼ��������򣨽�����Ϊһ���¹ؼ�������б��У�

hist:		ֱ��ͼ����
mag_thr:	������ֵ��omax*SIFT_HIST_PEAK_RATIO
bins:		ֱ��ͼ�е�bin��
featNode:	��ǰ�ؼ���
featVect:	�洢��ֵ��������Ϣ
����ֵ��	��һ�����
*/
FeatureNode* add_dominated_ori(double *hist, double mag_thr, int bins, FeatureNode *featNode, FeatureVect *featVect)
{
	int i, signal = 0;
	int l, r;
	double nb, ori;

	Feature		*feat;
	FeatureNode *newNode, *nextNode, *retNode = featNode->next;

	for (i=0; i<bins; i++)
	{
		l = (i==0)?(bins-1):i-1;
		r = (i+1==bins) ? 0 : i+1;
		if((hist[i]>hist[l])&&(hist[i]>hist[r])&&(hist[i]>=mag_thr))
		{
			//�����ֵ��ķ���
			nb = i + interp_hist_peak(hist[l], hist[i], hist[r]);
			nb = (nb<0) ? (bins+nb) : (nb>=bins ? (nb-bins) : nb);
			ori = ((SIFT_PI2*nb)/bins) - SIFT_PI;
			signal++;
			if (signal==1)
			{
				feat = featNode->feat;
				feat->orient = ori;
			}
			else
			{
				feat = new_feature();
				feat_copy(feat, featNode->feat);
				feat->orient = ori;
				newNode = (FeatureNode *)calloc(1, sizeof(FeatureNode));
				newNode->feat = feat;
				//���½�����������
				nextNode = featNode->next;
				if (nextNode!=NULL)
					nextNode->prev = newNode;
				newNode->next = nextNode;
				newNode->prev = featNode;
				featNode->next = newNode;	
				featVect->to_extr_num++;		//�ؼ����������
			}			
		}
	}
	return retNode;
}

/*
����������

dest: ָ��Ŀ��feature�ṹ
src:  ָ��Դfeature�ṹ
*/
void feat_copy(Feature *dest, Feature *src)
{
	dest->octave = src->octave;
	dest->ox = src->ox;
	dest->oy = src->oy;
	dest->scale = src->scale;
	dest->sig_octave = src->sig_octave;
	dest->sig_space = src->sig_space;
	dest->sub_sca = src->sub_sca;
	dest->x = src->x;
	dest->y = src->y;
}

/*
����dominated �ؼ������

hist: ֱ��ͼ
bins: ֱ��ͼ�е�bin��
����ֵ�� ������ֵ
*/
double domin_mag(double *hist, int bins)
{   
	int i;
	int binmax;
	double omax;

	binmax = 0; 
	omax = hist[0];
	for (i=1; i<bins; i++)
	{
		if (hist[i]>omax)
		{
			omax	= hist[i];
			binmax	= i;
		}
	}
	return omax;
}

/*
ƽ������ֱ��ͼ

hist: ��ƽ����ֱ��ͼ
bins: ֱ��ͼ�е�bin��
*/
void smooth_hist(double *hist, int bins)
{
	int i;
	double prev, next, tmp;

	prev = hist[bins-1];
	for (i=0; i<bins; i++)
	{
		tmp = hist[i];
		next = (i+1 == bins) ? hist[0] : hist[i+1];
		hist[i] = 0.25*prev + 0.5*tmp + 0.25*next;
		prev = tmp;
	}
}

/*
���㵱ǰ�ؼ�����ݶ�ֱ��ͼ

magimg: �ݶ�ֵ
ortimg: ����
y:     �ؼ���������
x:     �ؼ��������
rad:   region�뾶
sigma: regionƽ���߶�
hist:  ֱ��ͼ
w:     ��ǰ�ؼ�������octave��ͼ����
h:     ͼ��߶�
*/
void cal_hist(float **magimg, float **ortimg, int y, int x, int rad, float sigma, double *hist, int w, int h)
{
	int i, j;
	int bin;
	double mag, ori, exp_denom, wei;

	memset(hist, 0, sizeof(float)*SIFT_ORI_HIST_BIN_NUM);

	exp_denom = 2.0*sigma*sigma;
	for (j=-rad; j<=rad; j++)
	{
		if ((y-j<1)||(y-j>h-2))
			continue;
		for (i=-rad; i<=rad; i++)
		{
			if ((x+i<1)||(x+i>w-2))		//���ڷ�Χ��
				continue;
			mag = magimg[y-j][x+i];
			ori = ortimg[y-j][x+i];
			wei = exp(-(j*j+i*i)/exp_denom);
			bin = round(SIFT_ORI_HIST_BIN_NUM*(ori+SIFT_PI)/SIFT_PI2);
			bin = (bin<SIFT_ORI_HIST_BIN_NUM) ? bin : 0;		//[0,10).....[340, 350)[350, 360)
			hist[bin] += wei*mag;
		}
	}
}


/*
����һ���baseͼ��double�ˣ��޸�feature��Ϣ�е�ȫ�������ƽ������

featVect:	�洢��ֵ��������Ϣ
*/
void adjust_doubled_img(FeatureVect *featVect)
{
	Feature		*feat;
	FeatureNode *node;

	node = featVect->first;
	while(node!=NULL)
	{
		feat = node->feat;
		feat->ox /= 2.0;
		feat->oy /= 2.0;
		feat->sig_space /= 2.0;

		node = node->next;
	}
}

/*
�������м�ֵ��ĳ߶�

featVect:	�洢��ֵ��������Ϣ
sigma:      base��ĳ߶�
scales��    ����
*/
void cal_feat_sigma(FeatureVect *featVect, float sigma, int scales)
{
	int octave;
	float scale; //��ȷ����λ��
	FeatureNode *node = featVect->first;
	Feature *feat;

	while(node!=NULL)
	{
		feat	= node->feat;
		octave	= feat->octave;
		scale	= feat->scale+feat->sub_sca;

		feat->sig_octave = sigma*fpow(2.0, scale/scales);
		feat->sig_space = feat->sig_octave*fpow(2.0, octave);

		node = node->next;
	}
}

/*
�õ����м�ֵ�㣨��ȷ��λ��ȥ���ͶԱȶȼ���Ե���ģ�

bd_dist:  �߽�㣬�ڴ������ڲ����м�ֵ���
dogimg:   dog�ռ�
octaves:  �߶ȿռ�����
scales:   ����
basWid:   base��ͼ����
basHei:   base��ͼ��߶�
����ֵ:   �ؼ���������Ϣ
*/
FeatureVect* scale_space_extrema(int bd_dist, float ****dogimg, int octaves, int scales, int basWid, int basHei, float contr_thr)
{
	int i, j;
	int o, s;
	int w = basWid<<1;
	int h = basHei<<1;
	float ctr_thresh = contr_thr/scales;		//�ͶԱȶ���ֵ
	float ini_thresh = ctr_thresh*0.5f;			//Ԥɸѡֵ
	Feature		*nfeat;
	FeatureNode *featnode;
	FeatureVect *featVect = (FeatureVect *)calloc(1, sizeof(FeatureVect));
	featVect->to_extr_num = 0;
	featVect->first = NULL;
	featVect->cur_node = NULL;

	for (o=0; o<octaves; o++)
	{
		w = w>>1; h = h>>1;
		for (s=1; s<=scales; s++)
		{
			for (j=bd_dist; j<h-bd_dist; j++)
			{
				for (i=bd_dist; i<w-bd_dist; i++)
				{
					//Ԥɸѡ
					if (dogimg[o][s][j][i]<ini_thresh)
						continue;
					//�����ǰλ��Ϊ��ֵ��
					if (is_local_extrema(dogimg[o], s, j, i))
					{
						//��ȷ��λ����ȥ���ͶԱȶȼ��߽��
						nfeat = acc_localization(w, h, scales, bd_dist, dogimg[o], o, s, j, i, ctr_thresh, SIFT_EDGE_RATIO, SIFT_MAX_INTERP_STEPS);
						if (nfeat)
						{
							featnode = (FeatureNode *)calloc(1, sizeof(FeatureNode));
							featVect->to_extr_num++;			//��ֵ����++
							featnode->feat = nfeat;				//��ǰ����еļ�ֵ����Ϣ
							featnode->next = NULL;				//ָ�������е���һ�����
							featnode->prev = featVect->cur_node;//ָ�������е�ǰһ�����
							if (featVect->to_extr_num==1)		//����ǰ����ǵ�һ�����
								featVect->first = featnode;
							else
								featVect->cur_node->next = featnode;
							featVect->cur_node = featnode;		//�������ֵ����Ϊ�����еĵ�ǰ���
						}
					}
				}
			}
		}
	}

	return featVect;
}

/*
��þ�ȷ��λ��ȥ���ԵͶԱȶȺͱ߽���ĵ�

w:          ͼ����
h:          ͼ��߶�
scales:		����
bd_dist:    �߽�㣬�ڴ������ڲ����м�ֵ���
dog:		dogͼ��[scale][y][x]
octave:     ������
scale:		��ǰ�����ڲ�
y:			���Ƚ�ͼ��������
x:			���Ƚ�ͼ�������
ctr_thresh:	�Աȶ�����
ratio:     ���ʣ�ȥ���߽��ʱ�ã�Ĭ��Ϊ10.0
max_steps:	��ȷ��λ��󲽳�
*/
Feature* acc_localization(int w, int h, int scales, int bd_dist, float ***dog, int octave, int scale, int y, int x, float ctr_thresh, float ratio, int max_steps)
{
	int		cnt = 0, times;
	int		ny, nx, ns;		//��λ��
	double	yi, xi, si;		//ƫ��λ��
	double	dy, dx, ds;		//һ��ƫ��	
	double  dExtr;			//��ֵ
	Feature *feat;

	ns = scale;
	ny = y;
	nx = x;
	while(cnt<max_steps)
	{
		//����ƫ��ֵ
		cal_interp_offset(dog, ns, ny, nx, &yi, &xi, &si, &dy, &dx, &ds);
		//�ҵ���ȷ�㣬����ѭ��
		if ((abs_doub(yi)<0.5)&&(abs_doub(xi)<0.5)&&(abs_doub(si)<0.5))
			break;
		nx += round(xi);
		ny += round(yi);
		ns += round(si);

		//��λ���Ƿ񳬳��߽緶Χ
		if ((nx<bd_dist)||(ny<bd_dist)||(nx>=w-bd_dist)||(ny>=h-bd_dist)||(ns<1)||(ns>scales))
			return NULL;

		cnt++;
	}
	if (cnt==max_steps)
		return NULL;

	//ȥ���ͶԱȶ�
	dExtr = dx*xi + dy*yi + ds*si;
	dExtr = dog[ns][ny][nx]+0.5*(dExtr);
	if (dExtr<ctr_thresh)
		return NULL;


	//ȥ���߽��
	if(is_on_edge(dog[scale], ny, nx, ratio))
		return NULL;

	feat = new_feature();
	feat->octave = octave;
	feat->scale = ns;
	feat->y = ny;
	feat->x = nx;
	feat->sub_sca = si;
	times = (int)fpow(2.0, octave);
	feat->ox = (nx+xi)*times;
	feat->oy = (ny+yi)*times;

	return feat;
}

/*
�½�һ��feature�ռ�
*/
Feature* new_feature()
{
	Feature *feat = (Feature *)calloc(1, sizeof(Feature));
	return feat;
}


/*
�Ƿ�Ϊ�߽��

dog:   dog�ռ�[y][x]
y:     ������
x:     ������
ratio�������������С����ֵ�ı���
*/
int is_on_edge(float **dog, int y, int x, float ratio)
{
	double dxx, dxy, dyy;
	double tr, det;
	float  val= dog[y][x];

	dxx = dog[y][x+1]-val - (val-dog[y][x-1]);
	dyy = dog[y+1][x]-val - (val-dog[y-1][x]);
	dxy = ((dog[y+1][x+1]-dog[y+1][x-1])-(dog[y-1][x+1]-dog[y-1][x-1]))/4.0;

	tr	= dxx + dyy;
	det	= dxx*dyy-dxy*dxy;
	if (det<=0)
		return 1;  //�ڱ���
	ratio = ((ratio+1)*(ratio+1))/ratio;
	if(((tr*tr)/det)<ratio)
		return 0;  //���ڱ��ʣ����ڱ��ϵĿ����Դ�

	return 1;
}

/*
����ƫ��ֵ

dog:		dogͼ��[scale][y][x]
scale:		��ǰ�����ڲ�
y:			���Ƚ�ͼ��������
x:			���Ƚ�ͼ�������
yi:         yƫ��
xi:			xƫ��
si:			�߶�ƫ��
dy��dx��ds: ƫ��
*/
void cal_interp_offset(float ***dog, int scale, int y, int x, double *yi, double *xi, double *si, double *pdy, double *pdx, double *pds)
{
	//ʹ��double������߾���
	double dx, dy, ds;		//һ��ƫ��
	double dxx, dyy, dss, dxy, dxs, dys;//����ƫ��
	double **m;				//����Ԫ��
	float  val = dog[scale][y][x];

	dx = (dog[scale][y][x+1]-dog[scale][y][x-1])/2.0;
	dy = (dog[scale][y+1][x]-dog[scale][y-1][x])/2.0;
	ds = (dog[scale+1][y][x]-dog[scale-1][y][x])/2.0;

	dxx = dog[scale][y][x+1]-val - (val-dog[scale][y][x-1]);
	dyy = dog[scale][y+1][x]-val - (val-dog[scale][y-1][x]);
	dss = dog[scale+1][y][x]-val - (val-dog[scale-1][y][x]);
	dxy = ((dog[scale][y+1][x+1]-dog[scale][y+1][x-1])-(dog[scale][y-1][x+1]-dog[scale][y-1][x-1]))/4.0;
	dxs = ((dog[scale+1][y][x+1]-dog[scale+1][y][x-1])-(dog[scale-1][y][x+1]-dog[scale-1][y][x-1]))/4.0;
	dys = ((dog[scale+1][y+1][x]-dog[scale+1][y-1][x])-(dog[scale-1][y-1][x]-dog[scale-1][y-1][x]))/4.0;

	//��������
	mem2D_alloc_double(&m, 3, 3);
	m[0][0] = dxx;
	m[1][1] = dyy;
	m[2][2] = dss;
	m[0][1] = m[1][0] = dxy;
	m[0][2] = m[2][0] = dxs;
	m[1][2] = m[2][1] = dys;
	if (inverse(m, 3)==1)
	{
		//����˷�
		*xi = m[0][0]*dx + m[0][1]*dy + m[0][2]*ds;
		*yi = m[1][0]*dx + m[1][1]*dy + m[1][2]*ds;
		*si = m[2][0]*dx + m[2][1]*dy + m[2][2]*ds;
	}
	else
		*xi = *yi = *si = 0.0;

	*pdy = dy;
	*pdx = dx;
	*pds = ds;

	mem2D_free_double(m);
}

/*
�жϵ�ǰ���Ƿ�Ϊ��ֵ��

dog:  dogͼ��[scale][y][x]
scale:��ǰ�����ڲ�
y:	  ���Ƚ�ͼ��������
x:    ���Ƚ�ͼ�������
*/
int is_local_extrema(float ***dog, int scale, int y, int x)
{
	int s, signal=0;
	int i, j;
	float val = dog[scale][y][x];

	if (val>0)
	{
		for (s=-1; s<=1; s++)
			for (j=-1; j<=1; j++)
				for (i=-1; i<=1; i++)
					if (dog[s+scale][y+j][x+i]>val)
						return 0;
	}
	else
	{
		for (s=-1; s<=1; s++)
			for (j=-1; j<=1; j++)
				for (i=-1; i<=1; i++)
					if (dog[s+scale][y+j][x+i]<val)
						return 0;
	}
	return 1;
}

/*
����õ�dog�ռ�

octaves:  �߶ȿռ�����
scales:	  gauss�ռ�ÿ�����, +3֮ǰ
dogimg:   dogͼ��
gaussimg: gaussͼ��
baseWid:  base��Ŀ��
baseHei:  base��ĸ߶�
*/
void dog_space(int octaves, int scales, float ****dogimg, float ****gaussimg, int baseWid, int baseHei)
{
	int o, s;
	int i, j;
	int w = baseWid<<1;
	int h = baseHei<<1;

	for (o=0; o<octaves; o++)
	{
		w = w>>1;
		h = h>>1;
		for (s=0; s<scales+2; s++)		//dog�ռ��gauss�ռ���һ��
		{
			for (j=0; j<h; j++)
			{
				for (i=0; i<w; i++)
				{
					dogimg[o][s][j][i] = gaussimg[o][s+1][j][i]-gaussimg[o][s][j][i];
				}
			}
		}
	}
}

/*
����õ�gauss�߶ȿռ�

wid:      ����gaussƽ��ģ��Ĵ��ڴ�С��wid*sigma,widĬ��ֵΪ2
octaves:  �߶ȿռ�����
scales:	  gauss�ռ�ÿ�����, +3֮ǰ
sigma0:   ��ײ��gaussƽ���߶ȣ�Ĭ��Ϊ1.6
sigman:   �����ԭʼͼ���ѱ�ƽ���ĳ߶�, ��������Ϊ0.5��1.0��doubled��
gaussimg: ��˹ƽ��ͼ��
imginfo:  ԭʼͼ�����ݣ���ӱ���ģ�
*/
void gauss_space(float wid, int octaves, int scales, float sigma0, float sigman, float ****gaussimg, ImgInfo *imginf)
{
	int i,j,o,s,t;
	int gwid;		//gauss���ڰ뾶��С
	int w	= imginf->width;
	int h	= imginf->height;
	int nw, nh;			//�߽�����Ŀ��
	imgpix  **orgdata = imginf->imgdata;
	float *sigma = (float *)calloc(scales+3, sizeof(float));
	float sigma_prev;		//��һ��ĳ߶�
	float ki;
	int max_gwid = (int)(wid*SIFT_MAX_GAUSS_SMOOTH_SIG); 
	float **gsKernel;		//��Ÿ�˹��
	float **smoothdata;		//����˲�����
	max_gwid = max_gwid<<1;
	max_gwid++;  //��󴰿ڴ�С�����ǰ뾶
	mem2D_alloc_float(&gsKernel, max_gwid, max_gwid);
	mem2D_alloc_float(&smoothdata, max_gwid+h, max_gwid+w);

	//��һ��ͼ������ֵ
	for (j=0; j<h; j++)
		for (i=0; i<w; i++)
			gaussimg[0][0][j][i] = ((float)orgdata[j][i])/MAX_IMG_VAL;
			
	//��������ƽ���߶�
	sigma[0] = fsqrt(sigma0, sigman);
	ki = fpow(2.0, 1.0/scales);
	for (s=1; s<(scales+3); s++)
	{
		sigma_prev = fpow(ki, s-1)*sigma0;
		sigma[s] = sigma_prev*ki;
		sigma[s] = fsqrt(sigma[s], sigma_prev);
	}

	//����gauss�߶ȿռ�
	w = w<<1;
	h = h<<1;
	for (o=0; o<octaves; o++)
	{
		w = w>>1; h = h>>1;
		for (s=0; s<scales+3; s++)
		{
			gwid = (int)(sigma[s]*wid+0.5f);
			gauss_kernel(sigma[s], gwid, gsKernel);
			nw = w + gwid*2;
			nh = h + gwid*2;
			if ((s>=scales)||((s==0)&&(o==0))||(o==0)) 
			{
				if (s!=0)
					t = s-1;
				else
					t = s;
				//�����˲����ݿ�������ʱ�����������б߽����
				for (j=0; j<h; j++)
				{
					memcpy(smoothdata[j+gwid]+gwid, gaussimg[o][t][j], w*sizeof(float));
					for (i=0; i<gwid; i++)
					{
						smoothdata[j+gwid][i]	= gaussimg[o][t][j][0];
						smoothdata[j+gwid][nw-i-1]	= gaussimg[o][t][j][w-1];
					}
				}
				for (j=0; j<gwid; j++)
				{
					memcpy(smoothdata[j], smoothdata[gwid], nw*sizeof(float));
					memcpy(smoothdata[nh-j-1], smoothdata[nh-gwid-1], nw*sizeof(float));
				}

				//�˲�
				for (j=0; j<h; j++)
				{
					for (i=0; i<w; i++)
					{
						gaussimg[o][s][j][i] = gauss_filter(smoothdata, gsKernel, i+gwid, j+gwid, gwid);
					}
				}
			}
			else  //������ĵ�һ��scale��
			{
				down_sample(gaussimg[o][s], gaussimg[o-1][scales+s], w, h);	//������ĵ�һ�������ֱ������һ��ĳ߶�Ϊ2sigma��ƽ��ͼ���²����õ�
			}
		}
	}
	
	mem2D_free_float(gsKernel);
	mem2D_free_float(smoothdata);
}

/*
�²��������ڽ���ֵ����

dest:		�����������
src:		Դ����
destWidth:	Ŀ��ͼ���
destHeight��Ŀ��ͼ���
*/
void down_sample(float **dest, float **src, int destWidth, int destHeight)
{
	int i, j;
	int i1, j1;

	for(j=0; j<destHeight; j++)
	{
		for (i=0; i<destWidth; i++)
		{
			i1 = i<<1;
			j1 = j<<1;
			dest[j][i] = src[j1][i1];
		}
	}
}

/*
�����˹��

sig:  ƽ���߶�
gwid: ƽ���뾶
gw��  ��˹ģ��
*/
void gauss_kernel(float sig, int gwid, float **gw)
{
	int i, j;
	float sig2 = 2*sig*sig;

	for (j=-gwid; j<=gwid; j++)
	{
		for (i=-gwid; i<=gwid; i++)
			gw[j+gwid][i+gwid] = exp(-(i*i+j*j)/sig2);
	}
}


/*
gauss filter

data: ���˲�����
gw��  ��˹ģ��
c:    �����꣬i
r:    ������, j
w:    �˲��뾶
*/
float gauss_filter(float **data, float **gw, int c, int r, int w)
{
	int		i, j;
	float	res = 0.0, sum=0.0;
	float	tmp; 

	c = c-w;
	r = r-w;
	w = w+w;
	for (j=0; j<=w; j++)
	{
		for (i=0; i<=w; i++)
		{
			tmp = gw[j][i];
			res += tmp * (data[r+j][c+i]);
			sum += tmp;
		}
	}
	return res/sum;
}


/*
Ϊ�߶ȿռ�����ڴ�

octave: �߶ȿռ�����
scales: ÿ�����
imginf: ԭʼͼ�񣨻�ӱ����ԭʼͼ��
gaussimg: gaussƽ��ͼ��[octaves][scales][height][width]
dogimg:	  dogͼ��
*/
int gauss_mem_alloc(int octaves, int scales, ImgInfo *imginf, float *****gaussimg, float *****dogimg)
{
	int i, w, h;
	int wid = imginf->width;
	int hei = imginf->height;
	int mem=0;
	
	w = wid<<1;
	h = hei<<1;

	//Ϊgaussƽ����dogͼ�����ռ�
	*gaussimg = (float ****)calloc(octaves, sizeof(float ***));
	*dogimg	 = (float ****)calloc(octaves, sizeof(float ***));
	for (i=0; i<octaves; i++)
	{
		w = w>>1;
		h = h>>1;
		mem += mem3D_alloc_float(&(*gaussimg)[i], scales, h, w);
		mem += mem3D_alloc_float(&(*dogimg)[i], scales-1, h, w);		//dog�ռ��gauss�ռ���һ��
	}

	return mem;
}

/*
�ͷ��ѷ����ڴ�

octave: �߶ȿռ�����
scales: ÿ�����
gaussimg: gaussƽ��ͼ��[octaves][scales][height][width]
dogimg:	  dogͼ��
*/
void gauss_mem_free(int octaves, int scales, float ****gaussimg, float ****dogimg)
{
	mem4D_free_float(gaussimg, octaves, scales);
	mem4D_free_float(dogimg, octaves, scales-1);
}

/*
��ͼ��˫���Բ�ֵΪ2����С
˫���Բ�ֵ��ʽ:f(i+u,j+v) = (1-u)(1-v)f(i,j) + (1-u)vf(i,j+1) + u(1-v)f(i+1,j) + uvf(i+1,j+1)

imgData:ԭʼͼ������
imginf: �����
width:	ԭʼͼ���
height��ԭʼͼ���
*/
int imginfo_fill_dbl(imgpix *imgData, ImgInfo **imginf, int width, int height)
{
	int i, j;
	int i1, j1;
	int w = width*2;
	int h = height*2;
	int fij, fij1, fi1j, fi1j1;
	int nv;
	float u,v;

	//Ϊͼ�����ռ�
	*imginf	= (ImgInfo *)calloc(1, sizeof(ImgInfo));
	if (NULL==((*imginf)->imgdata = (imgpix **)calloc(h, sizeof(imgpix *))))
		return 0;
	if (NULL==((*imginf)->imgdata[0]	= (imgpix *)calloc(h*w, sizeof(imgpix))))
		return 0;
	for (i=1; i<h; i++)
		(*imginf)->imgdata[i] = (*imginf)->imgdata[i-1]+w;

	(*imginf)->width		= w;
	(*imginf)->height		= h;
	//��ֵԭʼ����
	for (j=0; j<h; j++)
	{
		for (i=0; i<w; i++)
		{
			u = i/2.0; v = j/2.0;
			i1 = i>>1; j1 = j>>1;
			fij		= j1*width + i1;
			fij1	= fij + width;
			fi1j	= fij + 1;
			fi1j1	= fij1 + 1;
			u = u-i1;
			v = v-j1;
			nv	= (int)((1-u)*(1-v)*imgData[fij] + (1-u)*v*imgData[fij1] + u*(1-v)*imgData[fi1j] + u*v*imgData[fi1j1]);
			nv	= (nv<0) ? 0: ((nv<MAX_IMG_VAL) ? nv : MAX_IMG_VAL);
			(*imginf)->imgdata[j][i] = nv;
		}
	}

	return w*h*sizeof(imgpix);
}

/*
���ͼ����Ϣ

imgData:ԭʼͼ������
imginf: �����
width:	ԭʼͼ���
height��ԭʼͼ���
*/
int imginfo_fill(imgpix *imgData, ImgInfo **imginf, int width, int height)
{
	int i,j,k;
	(*imginf)				= (ImgInfo *)calloc(1, sizeof(ImgInfo));
	if (((*imginf)->imgdata = (imgpix **)calloc(height, sizeof(imgpix *)))==NULL)
		return 0;
	if (((*imginf)->imgdata[0]	= (imgpix *)calloc(height*width, sizeof(imgpix)))==NULL)
		return 0;
	for (i=1; i<height; i++)
		(*imginf)->imgdata[i] = (*imginf)->imgdata[i-1] + width;

	(*imginf)->width		= width;
	(*imginf)->height		= height;

	for (j=0; j<height; j++)
	{
		for (i=0; i<width; i++)
		{
			k = j*width + i;
			(*imginf)->imgdata[j][i] = imgData[k];
		}
	}

	return height*width*sizeof(imgpix);
}



/*
�ͷ�ͼ��

imginfo��Ҫ�ͷŵ�ͼ��ṹ
*/
void imginfo_release(ImgInfo *imginf)
{
	if (imginf->imgdata)
	{
		if (imginf->imgdata[0])
			free(imginf->imgdata[0]);
		free(imginf->imgdata);
	}
	free(imginf);
}


#ifdef _CRT_SECURE_NO_DEPRECATE
#undef _CRT_SECURE_NO_DEPRECATE
#endif