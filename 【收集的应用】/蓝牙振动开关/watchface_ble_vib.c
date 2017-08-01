
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "maibu_res.h"
#include "maibu_sdk.h"




static uint8_t g_triangle_layer_h = 0, g_triangle_layer_m = 0;
static uint32_t g_triangle_window = 0;

static uint32_t g_switch_BLE = 0;

/*窗口图层位置*/
/*背景图层*/
#define TRIANGLE_BG_ORIGIN_X			0
#define TRIANGLE_BG_ORIGIN_Y			0
#define TRIANGLE_BG_SIZE_H			128
#define TRIANGLE_BG_SIZE_W			128

/*蓝牙文本图层位置*/
#define SHOW_MORE_D_ORIGIN_X		9	
#define SHOW_MORE_D_ORIGIN_Y		54
#define SHOW_MORE_D_SIZE_H			16	
#define SHOW_MORE_D_SIZE_W			114

	

//蓝牙图层id
char g_show_more_str_week[21] = {"0"};
/*定时器状态*/
static uint8_t g_request_ble_timer		= 0;
//蓝牙信息
static int32_t g_official_bledis = 0;

void get_ble_status(date_time_t tick_time, uint32_t millis, void *context);

P_Window init_triangle_watch(void);






/*
 *--------------------------------------------------------------------------------------
 *     function:  get_bmp_layer
 *    parameter: 
 *       return:
 *  description:  
 * 	  other:
 *--------------------------------------------------------------------------------------
 */
P_Layer get_bmp_layer(GRect *frame_p ,int32_t bmp_key)
{

	GBitmap bitmap;
	
	/*获取系统图片*/
	res_get_user_bitmap(bmp_key, &bitmap);

	//printf("XXXXXXXXXXXXXXXXXXXXXX:%d, %d, %d, %d\n", bitmap.app_id, bitmap.key, bitmap.height, bitmap.width);
	LayerBitmap lb1 = { bitmap,*frame_p, GAlignLeft};	

	/*图层1*/
	P_Layer	 layer1 = NULL;
	layer1 = app_layer_create_bitmap(&lb1);
	//app_layer_set_bg_color(layer1, GColorBlack);

	return (layer1);
}


//重新载入并刷新窗口所有图层
void window_reloading(void)
{
	/*根据窗口ID获取窗口句柄*/
	P_Window p_old_window = app_window_stack_get_window_by_id(g_triangle_window); 
	if (NULL != p_old_window)
	{
		P_Window p_window = init_triangle_watch();
		if (NULL != p_window)
		{
			g_triangle_window = app_window_stack_replace_window(p_old_window, p_window);
		}	
	}
	
}

/*定义enter按键事件*/
void scroll_select_enter(void *context)
{
	if (g_switch_BLE == 0)
	{
		g_switch_BLE = 1;
	}else{
		g_switch_BLE = 0;
	}
	window_reloading();
}

/*定义后退按键事件*/
static void scroll_select_back(void *context)
{
	P_Window p_window = (P_Window)context;
	if (NULL != p_window)
	{
		app_window_stack_pop(p_window);
	}
}

//获取蓝牙状态，是否断开。
 void get_ble_status(date_time_t tick_time, uint32_t millis, void *context)
{
        uint8_t ble_sta = maibu_get_ble_status();  //获取蓝牙状态
        if (ble_sta != 1 && ble_sta != 2 ) {
                if (g_official_bledis == 0 && g_switch_BLE == 1)
                {
					g_official_bledis = 1;
					maibu_service_vibes_pulse(VibesPulseTypeMiddle,1); 
                }
        }
        else if (ble_sta = 1) {
                if (g_official_bledis == 1 && g_switch_BLE == 1)
                {
					g_official_bledis = 0;
					maibu_service_vibes_pulse(VibesPulseTypeMiddle,1); 
                }
        }
		
}

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


P_Window init_triangle_watch(void)
{
	P_Window p_window = NULL;
	p_window = app_window_create();
	if (NULL == p_window)
	{
		return NULL;
	}

	P_Layer bgl = NULL, hl = NULL, ml = NULL, cl1 = NULL, cl2 = NULL;

	struct date_time datetime;
	app_service_get_datetime(&datetime);

	GRect frame = {{TRIANGLE_BG_ORIGIN_X, TRIANGLE_BG_ORIGIN_Y}, {TRIANGLE_BG_SIZE_H, TRIANGLE_BG_SIZE_W}};
	bgl = get_bmp_layer(&frame,RES_BITMAP_WATCHFACE_OFFICIAL_BG01);
	

	/*添加图层到窗口*/
	app_window_add_layer(p_window, bgl);
	

	//添加蓝牙文本图层
	frame.origin.x = SHOW_MORE_D_ORIGIN_X;
	frame.origin.y = SHOW_MORE_D_ORIGIN_Y;
	frame.size.h = SHOW_MORE_D_SIZE_H;
	frame.size.w = SHOW_MORE_D_SIZE_W;

	//获取蓝牙文本
	if (g_switch_BLE == 0)
	{
		sprintf(g_show_more_str_week, "%s", "蓝牙振动已关闭");	
	}else{
		sprintf(g_show_more_str_week, "%s", "蓝牙振动已开启");	
		maibu_service_vibes_pulse(VibesPulseTypeShort,1); 
	}
	
	display_target_layerText(p_window,&frame,GAlignCenter,GColorBlack,g_show_more_str_week,U_ASCII_ARIALBD_24);
	
	app_window_update(p_window);

	/*添加按键事件*/
	app_window_click_subscribe(p_window, ButtonIdSelect, scroll_select_enter);
	app_window_click_subscribe(p_window, ButtonIdBack, scroll_select_back);

	
	return p_window;
}


int main()
{
              /*模拟器模拟显示时打开，打包时屏蔽*/
	//simulator_init();

	if(g_request_ble_timer == 0)
	{
		g_request_ble_timer = app_service_timer_subscribe(1000, get_ble_status, NULL);	
	}

	P_Window p_window = init_triangle_watch();
	

	g_triangle_window = app_window_stack_push(p_window);


	/*模拟器模拟显示时打开，打包时屏蔽*/
	//simulator_wait();

	return 0;
}
