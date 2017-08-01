//TODO：my own uuid
//TODO: push window to notify

#include "maibu_res.h"
#include "maibu_sdk.h"


//蓝牙状态扫描周期（ms）
#define BT_SCAN_TIME_HZY 5000

//断开时震动次数
#define VIBES_TIMES_DISCON 1

//窗口ID
static int32_t g_triangle_window = 0;

//是否打开蓝牙提醒功能
static bool g_switch_BLE = 1;


/*定时器ID */
static uint8_t g_request_ble_timer = 0;
//上一次的蓝牙信息 1连接 2断开 0未知
static uint8_t bt_last_state = 0;


//函数声明
static void get_ble_status(date_time_t tick_time, uint32_t millis, void *context);
static P_Window init_triangle_watch(void);


/*定义enter按键事件*/
static void scroll_select_enter(void *context)
{
	maibu_service_vibes_pulse(VibesPulseTypeShort, 1); 
	g_switch_BLE = !g_switch_BLE;
	
	//重新载入窗口
	P_Window p_old_window = app_window_stack_get_window_by_id(g_triangle_window); 
	if (NULL != p_old_window)
	{
		P_Window p_window = init_triangle_watch();
		g_triangle_window = app_window_stack_replace_window(p_old_window, p_window);
	}
}


/*定义后退按键事件*/
static void scroll_select_back(void *context)
{
	P_Window p_window = (P_Window)context;
	if (NULL != p_window)
		app_window_stack_pop(p_window);
}


//定时回调：获取蓝牙状态
static void get_ble_status(date_time_t tick_time, uint32_t millis, void *context)
{
	//获取蓝牙状态
	uint8_t bt_state = maibu_get_ble_status();  
	if(bt_state==BLEStatusConnected || bt_state==BLEStatusUsing)
		bt_state = 1;
	else
		bt_state = 2;
	
	
	if(bt_state != bt_last_state)
	{
		bt_last_state = bt_state;
		
		//仅断开震动
		if(bt_state == 2)
			maibu_service_vibes_pulse(VibesPulseTypeMiddle, VIBES_TIMES_DISCON); 
	}
}


//生成窗口
static P_Window init_triangle_watch(void)
{
	P_Window p_window = NULL;
	p_window = app_window_create();
	if (NULL == p_window)
		return NULL;


	GRect grect = {{8,74},{16,112}};
	Layer *p_layer;
	
	//创建文本图层
	char str[22] = "蓝牙振动已开启";
	if(!g_switch_BLE)
		sprintf(str, "%s", "蓝牙振动已关闭");
	
	LayerText layertext = {str, grect, GAlignCenter, U_ASCII_ARIALBD_24, 0};
	p_layer = app_layer_create_text(&layertext);
	app_window_add_layer(p_window, p_layer);
	
	//创建图标图层
	grect.origin.x = 52;		grect.origin.y = 40;
	grect.size.h   = 24;	    grect.size.w   = 24;
	GBitmap gbitmap;
	res_get_user_bitmap(IMAGE_ICON, &gbitmap);
	LayerBitmap layerbitmap = {gbitmap, grect, GAlignCenter};
	p_layer = app_layer_create_bitmap(&layerbitmap);
	app_window_add_layer(p_window, p_layer);
	
	
	/*添加按键事件*/
	app_window_click_subscribe(p_window, ButtonIdSelect, scroll_select_enter);
	app_window_click_subscribe(p_window, ButtonIdBack, scroll_select_back);

	//定时器处理
	if(g_switch_BLE && !g_request_ble_timer)
		g_request_ble_timer = app_service_timer_subscribe(BT_SCAN_TIME_HZY, get_ble_status, NULL);
	else if(!g_switch_BLE && g_request_ble_timer)
	{
		app_service_timer_unsubscribe(g_request_ble_timer);
		g_request_ble_timer = 0;
	}
	
	
	return p_window;
}


int main()
{
	//simulator_init();


	P_Window p_window = init_triangle_watch();
	
	g_triangle_window = app_window_stack_push(p_window);


	//simulator_wait();
	return 0;
}
