#include "pedometer.h"
#include "math.h"
#include "stdio.h"
//#include "ble_nus.h"


#define TIMEWINDOW_MIN	2   	//0.2s:	2*Interval
#define TIMEWINDOW_MAX	20		//2s:	20*Interval
#define REGULATION		4		//
#define INVALID			2

#define VPPRANGE_A		2400	//
#define VPPRANGE_B		1000	//
#define VPPRANGE_C		300

#define PRECISION_A		(tmp >> 4)
#define PRECISION_B		100
#define PRECISION_C		50

uint8_t ReRecognize;
uint8_t TempSteps;
uint8_t Interval;
uint8_t InvalidSteps;
uint8_t SampleCounter;

uint32_t STEPS;

#if 0
xyzdata_t getMax(xyzdata_t a, xyzdata_t b)
{
	xyzdata_t max;
	max.x = abs(a.x) > abs(b.x) ? a.x : b.x;
	max.y = abs(a.y) > abs(b.y) ? a.y : b.y;
	max.z = abs(a.z) > abs(b.z) ? a.z : b.z;
	
	return max;
}

xyzdata_t getMin(xyzdata_t a, xyzdata_t b)
{
	xyzdata_t min;
	min.x = abs(a.x) < abs(b.x) ? a.x : b.x;
	min.y = abs(a.y) < abs(b.y) ? a.y : b.y;
	min.z = abs(a.z) < abs(b.z) ? a.z : b.z;
	
	return min;
}


xyzdata_t getThreshold(xyzdata_t max, xyzdata_t min)
{
	xyzdata_t res;
	res.x = (max.x + min.x) >> 1;
	res.y = (max.y + min.y) >> 1;
	res.z = (max.z + min.z) >> 1;
	
	return res;
}

xyzdata_t getDelta(xyzdata_t a, xyzdata_t b)
{
	xyzdata_t res;
	res.x = b.x - a.x;
	res.y = b.y - a.y;
	res.z = b.z - a.z;
	
	return res;
}

#endif


/*------------------------------------------------------------------------------------------------------------------------
*Name: 		TimeWindow()
*Function:	ʵ��"ʱ�䴰"�㷨,��Ϊֻ������Ч"ʱ�䴰"�ڵļǲ�����Ч,������ʼʱ��Ҫ����������Ч������Ϊ��ʼ��20msִ��һ��
*Input:		void
*Output: 	void
*------------------------------------------------------------------------------------------------------------------------*/
void TimeWindow(void)
{
//	uint32_t err_code;
	//printf("TimeWindow\r\n");
	if(ReRecognize==2)		//������¿�ʼ�ĵ�һ����ֱ���ڼǲ������м�1
	{
		TempSteps++;
		Interval=0;		//100ms
		ReRecognize=1;
		InvalidSteps=0;	
	}
	else				//��������¿�ʼ�ĵ�һ��
	{
		if( (Interval>=TIMEWINDOW_MIN) && (Interval<=TIMEWINDOW_MAX) )	//���ʱ��������Ч��ʱ�䴰��
		{
			InvalidSteps=0;	
			if(ReRecognize==1)					//�����û���ҵ�����
			{
				TempSteps++;				//�ǲ������1
				if(TempSteps>=REGULATION)	//����ǲ�����ﵽ��Ҫ��Ĺ�����
				{
					ReRecognize=0;				//�Ѿ��ҵ�����
					STEPS=STEPS+TempSteps;	//������ʾ
					TempSteps=0;
				}
				Interval=0;
			}
			else if(ReRecognize==0)				//����Ѿ��ҵ����ɣ�ֱ�Ӹ�����ʾ
			{
				STEPS++;
				TempSteps=0;
				Interval=0;
				//printf("%d Steps\r\n", STEPS);
			}
		}
		else if( Interval<TIMEWINDOW_MIN )	//���ʱ����С��ʱ�䴰����
		{	
			if(ReRecognize==0)					//����Ѿ��ҵ�����
			{
				if(InvalidSteps<255) 	InvalidSteps++;	//��Ч�������1
				if(InvalidSteps>=INVALID)				//�����Ч���ﵽ��Ҫ�����ֵ��������Ѱ�ҹ���
				{	
					InvalidSteps=0;
					ReRecognize=1;
					TempSteps=1;
					Interval=0;
				}
				else									//����ֻ������һ�εļǲ������Ǽ����ǲ�������Ҫ����Ѱ�ҹ���
				{
					Interval=0;
				}
			}
			else if(ReRecognize==1)				//�����û���ҵ����ɣ���֮ǰ��Ѱ�ҹ��ɹ�����Ч������Ѱ�ҹ���
			{
				InvalidSteps=0;	
				ReRecognize=1;
				TempSteps=1;
				Interval=0;
			}
		}
		else if(Interval>TIMEWINDOW_MAX)	//���ʱ��������ʱ�䴰���ޣ��ǲ��Ѿ���ϣ�����Ѱ�ҹ���
		{
			InvalidSteps=0;	
			ReRecognize=1;						
			TempSteps=1;
			Interval=0;
		}
	}		
}


/*------------------------------------------------------------------------------------------------------------------------
*Name: 		step_counter()
*Function:	ʵ��Pedometer�Ļ����㷨��20msִ��һ�Ρ�
*Input:		void
*Output: 	void
*------------------------------------------------------------------------------------------------------------------------*/
void stepCounter(void)
{
	uint16_t tmp;
	xyzdata_t accelValue;
	xyzdata_t accelResult;
	static xyzdata_t softFilter[4];
	static xyzdata_t accelMax, accelMin;
	static xyzdata_t accelVpp, accelDC;
	static xyzdata_t badFlag, precision;
	static xyzdata_t oldFixed, newFixed;
	
	//--------------------------Data Sampling-----------------------------------
	accelValue = mma8451_get_data();
	softFilter[3] = softFilter[2];
	softFilter[2] = softFilter[1];
	softFilter[1] = softFilter[0];
	softFilter[0] = accelValue;
	
	accelResult.x = (softFilter[0].x + softFilter[1].x + softFilter[2].x + softFilter[3].x) >> 2;
	accelResult.y = (softFilter[0].y + softFilter[1].y + softFilter[2].y + softFilter[3].y) >> 2;
	accelResult.z = (softFilter[0].z + softFilter[1].z + softFilter[2].z + softFilter[3].z) >> 2;
	
//	if( (accelResult&0x7FFF) > (accelMax.x&0x7FFF) )
//		accelMax.x = accelResult.x;
	accelMax.x = abs(accelResult.x) > abs(accelMax.x) ? accelResult.x : accelMax.x;
	accelMax.y = abs(accelResult.y) > abs(accelMax.y) ? accelResult.y : accelMax.y;
	accelMax.z = abs(accelResult.z) > abs(accelMax.z) ? accelResult.z : accelMax.z;
	
	accelMin.x = abs(accelResult.x) < abs(accelMin.x) ? accelResult.x : accelMin.x;
	accelMin.y = abs(accelResult.y) < abs(accelMin.y) ? accelResult.y : accelMin.y;
	accelMin.z = abs(accelResult.z) < abs(accelMin.z) ? accelResult.z : accelMin.z;	
		
	SampleCounter++;

	//----------------------------------���㶯̬���޺Ͷ�̬����-----------------------//
	if( SampleCounter >= 60 )
	{
		SampleCounter = 0;
		//printf("%d\t%d\t%d\r\n", accelResult.x, accelResult.y, accelResult.z);
		
		accelVpp.x = accelMax.x - accelMin.x;
		accelVpp.y = accelMax.y - accelMin.y;
		accelVpp.z = accelMax.z - accelMin.z;
		
		accelDC.x = accelMin.x + (accelVpp.x >> 1);
		accelDC.y = accelMin.y + (accelVpp.y >> 1);
		accelDC.z = accelMin.z + (accelVpp.z >> 1);
		
		accelMax.x = accelMax.y = accelMax.z = 0;
		accelMin.x = accelMin.y = accelMin.z = 32767;
		badFlag.x = badFlag.y = badFlag.z = 0;
		
		tmp = abs(accelVpp.x);
		//printf("%d\t", tmp);
		if(tmp >= VPPRANGE_A)			precision.x = PRECISION_A;
		else if(tmp >= VPPRANGE_B) 		precision.x = PRECISION_B;
		else if(tmp >= VPPRANGE_C)		precision.x = PRECISION_C;
		else
		{
			precision.x = 2;
			badFlag.x = 1;
		}
		
		tmp = abs(accelVpp.y);
		//printf("%d\t", tmp);
		if(tmp >= VPPRANGE_A)			precision.y = PRECISION_A;//tmp >> 5;
		else if(tmp >= VPPRANGE_B) 		precision.y = PRECISION_B;
		else if(tmp >= VPPRANGE_C)		precision.y = PRECISION_C;
		else
		{
			precision.y = 2;
			badFlag.y = 1;
		}
		
		tmp = abs(accelVpp.z);
		//printf("%d\r\n", tmp);
		if(tmp >= VPPRANGE_A)			precision.z = PRECISION_A;
		else if(tmp >= VPPRANGE_B) 		precision.z = PRECISION_B;
		else if(tmp >= VPPRANGE_C)		precision.z = PRECISION_C;
		else
		{
			precision.z = 2;
			badFlag.z = 1;
		}
		
	}
	//--------------------------������λ�Ĵ���--------------------------------------
	oldFixed.x = newFixed.x;
	oldFixed.y = newFixed.y;
	oldFixed.z = newFixed.z;
	//-----------------x
	if( accelResult.x >= newFixed.x ) 
	{
		if( (accelResult.x - newFixed.x) >= precision.x )	{newFixed.x = accelResult.x;}
	}
	else
	{
		if( (newFixed.x - accelResult.x) >= precision.x )	{newFixed.x = accelResult.x;}		
	}
	
	//-----------------y
	if( accelResult.y >= newFixed.y ) 
	{
		if( (accelResult.y - newFixed.y) >= precision.y )	{newFixed.y = accelResult.y;}
	}
	else
	{
		if( (newFixed.y - accelResult.y) >= precision.y )	{newFixed.y = accelResult.y;}		
	}

	//-----------------z
	if( accelResult.z >= newFixed.z ) 
	{
		if( (accelResult.z - newFixed.z) >= precision.z )	{newFixed.z = accelResult.z;}
	}
	else
	{
		if( (newFixed.z - accelResult.z) >= precision.z )	{newFixed.z = accelResult.z;}		
	}	

	//------------------------- ��̬�����о� ----------------------------------
	if( (accelVpp.x >= accelVpp.y) && (accelVpp.x >= accelVpp.z) )
	{
		if( (oldFixed.x >= accelDC.x) && (newFixed.x < accelDC.x) && (badFlag.x == 0) )	{TimeWindow();}
	}
	else if( (accelVpp.y >= accelVpp.z) && (accelVpp.y >= accelVpp.z) )
	{
		if( (oldFixed.y >= accelDC.y) && (newFixed.y < accelDC.y) && (badFlag.y == 0) )	{TimeWindow();}
	}
	else if( (accelVpp.z >= accelVpp.x) && (accelVpp.z >= accelVpp.y) )
	{
		if( (oldFixed.z >= accelDC.z) && (newFixed.z < accelDC.z) && (badFlag.z == 0) )	{TimeWindow();}
	}
}


