/*
 * =====================================================================================
 * 
 *     Modulename:
 *       Filename:  watchface_Calendar.c
 *
 *    Description:  挂历时间表盘
 *    Corporation:
 * 
 *         Author:  claire 
 *        Created:  2016年03月19日
 *
 * =====================================================================================
 *
 * =====================================================================================
 * 
 *   MODIFICATION HISTORY :
 *    
 *		     DATE :
 *		     DESC :
 * =====================================================================================
 */	
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "maibu_sdk.h"
#include "maibu_res.h"

#define BACKGROUND_NUM 1 //背景图片个数
#define LAYER_NUM 11 //需定时更新的图层个数

/*图片坐标：第一项为背景图片，后面依次为步数，卡路里，公里数图层的最后一位*/
static uint8_t ORIGIN_X[LAYER_NUM+BACKGROUND_NUM] = {0, 24, 33, 86, 95, 15, 37, 69, 91, 18, 81, 99};
static uint8_t ORIGIN_Y[LAYER_NUM+BACKGROUND_NUM] = {0, 26, 26, 49, 49, 80, 80, 80, 80, 49, 49, 41};
static uint8_t ORIGIN_W[LAYER_NUM+BACKGROUND_NUM] = {128, 9, 9, 9, 9, 22, 22, 22, 22, 28, 27, 11};
static uint8_t ORIGIN_H[LAYER_NUM+BACKGROUND_NUM] = {128, 12, 12, 12, 12, 30, 30, 30, 30, 12, 12, 20};


/*图片名数组：分别为大数字0~9图片和小数字0~9图片*/
uint16_t bmp_array[29] = {PIC_0_1, PIC_1_1, PIC_2_1, PIC_3_1, PIC_4_1, PIC_5_1, PIC_6_1, PIC_7_1, PIC_8_1, PIC_9_1,
PIC_0_2, PIC_1_2, PIC_2_2, PIC_3_2, PIC_4_2, PIC_5_2, PIC_6_2, PIC_7_2, PIC_8_2, PIC_9_2, 
PIC_0_3, PIC_1_3, PIC_2_3, PIC_3_3, PIC_4_3, PIC_5_3, PIC_6_3, PIC_100, PIC_BLANK};


/*小时分钟月日图层ID，通过该图层ID获取图层句柄*/
static int8_t g_layer_time[LAYER_NUM] ={ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

/*窗口ID, 通过该窗口ID获取窗口句柄*/
static int32_t g_window = -1;

/*表盘应显示数据*/
static uint8_t watch_data[LAYER_NUM] = {0,0,0,0,0,0,0,0,0,0,0};


/*
 *--------------------------------------------------------------------------------------
 *     function:  app_watch_data
 *    parameter:  0为watch_data所有元素值，11仅为watch_data中的秒
 *       return:
 *  description:  将数组参数赋值为当前表盘应显示值
 * 	      other:
 *--------------------------------------------------------------------------------------
 */
static void app_watch_data()
{
	int8_t percent;
	maibu_get_battery_percent(&percent);

	struct date_time datetime;
	app_service_get_datetime(&datetime);

	watch_data[0] = datetime.mday/10;
	watch_data[1] = datetime.mday%10;
	if (percent >= 100)
	{
		watch_data[2] = 28;
		watch_data[3] = 28;
		watch_data[9] = 27;
	}else
	{
		watch_data[2] = percent/10;
		watch_data[3] = percent%10;
		watch_data[9] = 28;
	}
	watch_data[4] = datetime.hour/10+10;
	watch_data[5] = datetime.hour%10+10;
	watch_data[6] = datetime.min/10+10;
	watch_data[7] = datetime.min%10+10;
	watch_data[8] = datetime.wday+20;
	watch_data[10] = percent/5;
}




/*
 *--------------------------------------------------------------------------------------
 *     function:  get_layer
 *    parameter: 
 *       return:
 *  description:  生成表盘窗口的各图层
 * 	      other:
 *--------------------------------------------------------------------------------------
 */
static P_Layer get_layer(uint8_t pos)
{
	GRect frame = {{ORIGIN_X[pos], ORIGIN_Y[pos]}, {ORIGIN_H[pos], ORIGIN_W[pos]}};
	GBitmap bitmap;
	P_Layer	 layer_back = NULL;
		
	/*获取用户图片*/
	if (pos < BACKGROUND_NUM)
	{
		res_get_user_bitmap(PIC_BG, &bitmap);
	}else
	if (pos == LAYER_NUM)
	{
		GPoint p1 = {ORIGIN_X[pos]-ORIGIN_W[pos], ORIGIN_Y[pos]-watch_data[10]}, p2 = {ORIGIN_X[pos]-ORIGIN_W[pos], ORIGIN_Y[pos]},
			p3 = {ORIGIN_X[pos], ORIGIN_Y[pos]}, p4 = {ORIGIN_X[pos], ORIGIN_Y[pos]-watch_data[10]};

		/*多边形*/
		GPoint points[4] = {p1, p2, p3, p4};
		Polygon po = {4, points};
		Geometry pg = {GeometryTypePolygon, FillArea, GColorWhite, (void*)&po}; 
		P_Geometry p_pg[1];
		LayerGeometry lg;
		memset(&lg, 0, sizeof(LayerGeometry));
		p_pg[lg.num++] = &pg;
		lg.p_g = p_pg;

		/*生成几何图形图层*/
		layer_back = app_layer_create_geometry(&lg);
		return (layer_back);
	}else
	{
		res_get_user_bitmap(bmp_array[watch_data[pos-BACKGROUND_NUM]], &bitmap);
	}
	LayerBitmap layerbmp = { bitmap, frame, GAlignLeft};	

	/*生成图层*/
	layer_back = app_layer_create_bitmap(&layerbmp);

	return (layer_back);
}


/*
 *--------------------------------------------------------------------------------------
 *     function:  app_watch_update
 *    parameter: 
 *       return:
 *  description:  更新图层数据
 * 	      other:
 *--------------------------------------------------------------------------------------
 */
static void app_watch_update()
{
	P_Window p_window = NULL;
	P_Layer p_old_layer = NULL, p_new_layer = NULL;
	uint8_t pos;

	/*根据窗口ID获取窗口句柄*/
	p_window = app_window_stack_get_window_by_id(g_window);
	if (p_window == NULL)
	{
		return;
	}

	app_watch_data();

	for (pos=0; pos<LAYER_NUM; pos++)
	{
		/*获取数据图层句柄*/
		p_old_layer = app_window_get_layer_by_id(p_window, g_layer_time[pos]);	
		if (p_old_layer != NULL)
		{
			/*更新数据图层*/
			p_new_layer = get_layer(pos+1);
			app_window_replace_layer(p_window, p_old_layer, p_new_layer);
		}
	}

	/*窗口显示*/	
	app_window_update(p_window);
}


/*
 *--------------------------------------------------------------------------------------
 *     function:  app_watch_time_change 
 *    parameter: 
 *       return:
 *  description:  系统时间有变化时，更新时间图层
 * 	      other:
 *--------------------------------------------------------------------------------------
 */
static void app_watch_time_change(enum SysEventType type, void *context)
{

	/*时间更改*/
	if (type == SysEventTypeTimeChange)
	{
		app_watch_update();	
	}
}



/*
 *--------------------------------------------------------------------------------------
 *     function:  init_watch
 *    parameter: 
 *       return:
 *  description:  生成表盘窗口
 * 	      other:
 *--------------------------------------------------------------------------------------
 */
static P_Window init_watch(void)
{
	P_Window p_window = NULL;
	P_Layer layer = NULL;
	uint8_t pos;

	/*创建一个窗口*/
	p_window = app_window_create();
	if (NULL == p_window)
	{
		return NULL;
	}

	/*创建背景图层*/
	layer = get_layer(0);

	/*添加背景图层到窗口*/
	app_window_add_layer(p_window, layer);

	app_watch_data();

	/*创建所有图层并添加到窗口*/
	for (pos=BACKGROUND_NUM; pos<BACKGROUND_NUM+LAYER_NUM; pos++)
	{
		layer = get_layer(pos);
		g_layer_time[pos-BACKGROUND_NUM] = app_window_add_layer(p_window, layer);
	}

	/*注册一个事件通知回调，当有时间改变时，立即更新时间*/
	maibu_service_sys_event_subscribe(app_watch_time_change);

	return p_window;
}



/*
 *--------------------------------------------------------------------------------------
 *     function:  main
 *    parameter: 
 *       return:
 *  description:  主程序
 * 	      other:
 *--------------------------------------------------------------------------------------
 */
int main()
{
	/*模拟器模拟显示时打开，打包时屏蔽*/
	//simulator_init();

	/*创建显示表盘窗口*/
	P_Window p_window = init_watch();
	if (p_window != NULL)
	{
		/*放入窗口栈显示*/
		g_window = app_window_stack_push(p_window);
	}
	
	/*模拟器模拟显示时打开，打包时屏蔽*/
	//simulator_wait();

	return 0;
}
