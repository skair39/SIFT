#ifndef _SIFT_H_
#define _SIFT_H_

#include <math.h>
#include "global.h"

#ifdef	SIFT_API   
#else
#define SIFT_API _declspec(dllimport)
#endif

//һ����ֵ����Ϣ
typedef struct
{
	int		y;			//�ؼ���ռ�����
	int		x;
	int 	octave;		//��������
	int 	scale;		//���ڲ���
	float	sub_sca;	//�ǲ���
	//ƽ������
	float   sig_space;	//��ǰ�ؼ����������߶ȿռ��ڵ�ƽ������
	float   sig_octave;	//��ǰ�ؼ�����һ�����ڵ�ƽ������
	//��ԭͼ��ֱ����µ�����
	float   ox;
	float   oy;
	float   orient;		//�ؼ���������
	float   descriptor[SIFT_DESC_LEN]; //�ؼ���������
}Feature;

//�洢һ��ֵ�������ڵ�
typedef struct feature
{
	Feature *feat;				//�洢һ���ؼ�����Ϣ
	struct feature *next;	//ָ����һ���ڵ�
	struct feature *prev;	//ָ��ǰһ���ڵ�
}FeatureNode;

//�洢���м�ֵ�������ͷ
typedef struct featureVect
{
	int			to_extr_num;		//���м�ֵ�����
	FeatureNode *first;				//ָ���һ�����
	FeatureNode	*cur_node;			//ָ������������һ�����
}FeatureVect;

/*
export�ĺ���������
*/
SIFT_API FeatureVect* _sift_main(const char* infile, imgpix	*imgData, int width, int height, float wid, int db_img, float sigma0,\
								 float sigman, float contr_thr, int sift_border_dist, float ill_norm_thresh, int top_oct_reso);
SIFT_API void free_feature_nodes(FeatureVect *featVect);


/*
������
*/
void cal_descriptor(FeatureVect *featVect, float ****magimg, float ****ortimg, ImgInfo *imginf, int d, int binnum, float thresh);
void desc_norm(float *descriptor, int len);
void desc_1Dto3D(FeatureNode *node, float ***hist, int d, int binnum);
void cal_desc_hist(float ***hist, float ****magimg, float ****ortimg, ImgInfo *imginf, FeatureNode *Node, int d, int binnum);
void cal_feat_oris(FeatureVect *featVect, float ****magimg, float ****ortimg, ImgInfo *imginf);
FeatureNode* add_dominated_ori(double *hist, double mag_thr, int bins, FeatureNode *featNode, FeatureVect *featVect);
void feat_copy(Feature *dest, Feature *src);
double domin_mag(double *hist, int bins);
void smooth_hist(double *hist, int bins);
void cal_hist(float **magimg, float **ortimg, int y, int x, int rad, float sigma, double *hist, int w, int h);
int cal_grad_mag_ori(float ****magimg, float ****ortimg, int octaves, int scales, int w, int h, float ****gaussimg);
void adjust_doubled_img(FeatureVect *featVect);
void cal_feat_sigma(FeatureVect *featVect, float sigma, int scales);
FeatureVect* scale_space_extrema(int bd_dist, float ****dogimg, int octaves, int scales, int basWid, int basHei, float contr_thr);
Feature* acc_localization(int w, int h, int scales, int bd_dist, float ***dog, int octave, int scale, int y, int x, float ctr_thresh, float ratio, int max_steps);
Feature* new_feature();
int is_on_edge(float **dog, int y, int x, float ratio);
void cal_interp_offset(float ***dog, int scale, int y, int x, double *yi, double *xi, double *si, double *pdy, double *pdx, double *pds);
int is_local_extrema(float ***dog, int scale, int y, int x);
void dog_space(int octaves, int scales, float ****dogimg, float ****gaussimg, int baseWid, int baseHei);
void gauss_space(float wid, int octaves, int scales, float sigma0, float sigman, float ****gaussimg, ImgInfo *imginf);
void down_sample(float **dest, float **src, int destWidth, int destHeight);
void gauss_kernel(float sig, int gwid, float **gw);
float gauss_filter(float **data, float **gw, int c, int r, int w);
int gauss_mem_alloc(int octaves, int scales, ImgInfo *imginf, float *****gaussimg, float *****dogimg);
void gauss_mem_free(int octaves, int scales, float ****gaussimg, float ****dogimg);
int imginfo_fill_dbl(imgpix *imgData, ImgInfo **imginf, int width, int height);
int imginfo_fill(imgpix *imgData, ImgInfo **imginf, int width, int height);
void imginfo_release(ImgInfo *imginf);
int mag_ort_mem_alloc(float *****mag, float *****ort, int octaves, int scales, int w, int h);
void mag_ort_mem_free(float ****mag, float ****ort, int octaves, int scales);


#endif