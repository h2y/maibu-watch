/*
	首先：程序代码复用率低，需要大面积重构
	
	修改：
		降低表盘刷新频率
		十二小时显示
		月日使用文本输出，而不是图片
		顶部的maibu显示为for hzy，或者disconnected
		
	第二阶段：
		返回键用作音乐控制
		黑底、白底自动切换
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "maibu_sdk.h"
#include "maibu_res.h"

#define BACKGROUND_NUM 1 //背景图片个数
#define TIME_NUM 8 		 //时间位数

/*图片坐标：第一项为背景图片，后面依次为时间的1~4位*/
static uint8_t ORIGIN_X[9] = {0, 11, 36, 68, 95, 44, 54, 80, 90};
static uint8_t ORIGIN_Y[9] = {0, 29, 29, 29, 29, 96, 96, 96, 96};
static uint8_t ORIGIN_H[9] = {128, 59, 59, 59, 59, 15, 15, 15, 15};
static uint8_t ORIGIN_W[9] = {128, 22, 22, 22, 22, 9, 9, 9, 9};


/*图片名数组：第一项为背景图片，后面依次为数字0~9图片*/
uint16_t bmp_array[20] = {PIC_0_1, PIC_1_1, PIC_2_1, PIC_3_1, PIC_4_1, PIC_5_1, PIC_6_1, PIC_7_1, PIC_8_1, PIC_9_1, PIC_0_2, PIC_1_2, PIC_2_2, PIC_3_2, PIC_4_2, PIC_5_2, PIC_6_2, PIC_7_2, PIC_8_2, PIC_9_2};


/*小时分钟月日图层ID，通过该图层ID获取图层句柄*/
static int8_t g_layer_time[TIME_NUM] ={ -1, -1, -1, -1, -1, -1, -1, -1};

/*窗口ID, 通过该窗口ID获取窗口句柄*/
static int32_t g_window = -1;
/*星期定义*/
static char WeekDay[7][10] = {"周日","周一","周二","周三","周四","周五","周六"};
/*显示星期文本图层*/
#define DATE_ORIGIN_X	12
#define DATE_ORIGIN_Y	94
#define DATE_SIZE_H	20
#define DATE_SIZE_W	55

/*创建并显示文本图层*/
int32_t display_target_layerText(P_Window p_window,GRect  *temp_p_frame,enum GAlign how_to_align,enum GColor color,char * str,uint8_t font_type)
{
	LayerText temp_LayerText = {0};
	temp_LayerText.text = str;
	temp_LayerText.frame = *temp_p_frame;
	temp_LayerText.alignment = how_to_align;
	temp_LayerText.font_type = font_type;
	
	P_Layer p_layer = app_layer_create_text(&temp_LayerText);
	
	if(p_layer != NULL)
	{
		app_layer_set_bg_color(p_layer, color);
		return app_window_add_layer(p_window, p_layer);
	}
	return 0;
}

/*
 *  description:  更新时间图层
 */
static void app_watch_update()
{
	P_Window p_window = NULL;
	P_Layer p_layer = NULL;
	GBitmap bitmap;
	struct date_time datetime;
	app_service_get_datetime(&datetime);
	uint8_t new_time[TIME_NUM] = {datetime.hour/10, datetime.hour%10, datetime.min/10, datetime.min%10, datetime.mon/10, datetime.mon%10, datetime.mday/10, datetime.mday%10};
	uint8_t pos;
	
	//12小时制
	pos = 0;
	if(datetime.hour > 12)
		pos = datetime.hour - 12;
	else if(datetime.hour==0)
		pos = 12;
	
	if(pos != 0) 
	{
		new_time[0] = pos/10;
		new_time[1] = pos%10;
	}
	
	
	/*根据窗口ID获取窗口句柄*/
	p_window = app_window_stack_get_window_by_id(g_window);
	if (p_window == NULL)
		return;

	for (pos=0; pos<TIME_NUM; pos++)
	{
		/*获取时间图层句柄*/
		p_layer = app_window_get_layer_by_id(p_window, g_layer_time[pos]);	
		if(p_layer != NULL)
		{
			/*更新时间图层图片*/
			if (pos < 4)
			{
				res_get_user_bitmap(bmp_array[new_time[pos]], &bitmap);
			}
			else
			{
				res_get_user_bitmap(bmp_array[new_time[pos]+10], &bitmap);
			}
			app_layer_set_bitmap_bitmap(p_layer, &bitmap);
		}
	}

	/*窗口显示*/	
	app_window_update(p_window);
}


/*
 *  description:  系统时间有变化时，更新时间图层
 */
static void app_watch_time_change(enum SysEventType type, void *context)
{
	/*时间更改*/
	if (type == SysEventTypeTimeChange)
		app_watch_update();	
}



/*
 *  description:  生成表盘窗口的各图层
 */
static P_Layer get_layer(uint8_t pos, uint8_t time[])
{
	GRect frame = {{ORIGIN_X[pos], ORIGIN_Y[pos]}, {ORIGIN_H[pos], ORIGIN_W[pos]}};
	GBitmap bitmap;
		
	/*获取用户图片*/
	if (pos < BACKGROUND_NUM)
	{
		res_get_user_bitmap(PIC_BG, &bitmap);
	}
	else
	{
		if (pos < BACKGROUND_NUM + 4)
		{
			res_get_user_bitmap(bmp_array[time[pos-BACKGROUND_NUM]], &bitmap);
		}
		else
		{
			res_get_user_bitmap(bmp_array[time[pos-BACKGROUND_NUM]+10], &bitmap);
		}
	}
	LayerBitmap layerbmp = { bitmap, frame, GAlignLeft};	

	/*生成图层*/
	P_Layer	 layer_back = NULL;
	layer_back = app_layer_create_bitmap(&layerbmp);

	return (layer_back);
}


/*
 *  description:  生成表盘窗口
 */
static P_Window init_watch(void)
{
	P_Window p_window = NULL;
	P_Layer layer_background = NULL, layer_time = NULL;
	uint8_t pos;
	struct date_time datetime;
	app_service_get_datetime(&datetime);
	uint8_t current_time[TIME_NUM] = {datetime.hour/10, datetime.hour%10, datetime.min/10, datetime.min%10, datetime.mon/10, datetime.mon%10, datetime.mday/10, datetime.mday%10};

	/*创建一个窗口*/
	p_window = app_window_create();
	if (NULL == p_window)
		return NULL;

	/*创建背景图层*/
	layer_background = get_layer(0, current_time);

	/*添加背景图层到窗口*/
	app_window_add_layer(p_window, layer_background);

	/*创建时间图层并添加到窗口*/
	for(pos=BACKGROUND_NUM; pos<BACKGROUND_NUM+TIME_NUM; pos++)
	{
		layer_time = get_layer(pos, current_time);
		g_layer_time[pos-BACKGROUND_NUM] = app_window_add_layer(p_window, layer_time);
	}
	
	
	GRect temp_frame;
	char buff_str[14] = "";
	/* SJY----------------- 添加日期图层*/
	temp_frame.origin.x = DATE_ORIGIN_X;
	temp_frame.origin.y = DATE_ORIGIN_Y;
	temp_frame.size.h = DATE_SIZE_H;
	temp_frame.size.w = DATE_SIZE_W;
	sprintf(buff_str, "%s", WeekDay[datetime.wday]);
	display_target_layerText(p_window,&temp_frame,GAlignLeft,GColorBlack,buff_str,U_ASCII_ARIALBD_14);
	

	/*注册一个事件通知回调，当有时间改变时，立即更新时间*/
	maibu_service_sys_event_subscribe(app_watch_time_change);

	return p_window;
}



int main()
{
	/*模拟器模拟显示时打开，打包时屏蔽*/
	simulator_init();

	/*创建显示表盘窗口*/
	P_Window p_window = init_watch();
	if (p_window != NULL)
	{
		/*放入窗口栈显示*/
		g_window = app_window_stack_push(p_window);
	}
	
	/*模拟器模拟显示时打开，打包时屏蔽*/
	simulator_wait();
	return 0;
}
