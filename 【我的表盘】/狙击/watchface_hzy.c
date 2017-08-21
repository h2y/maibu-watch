#include "maibu_res.h"
#include "maibu_sdk.h"


/*窗口ID, 通过该窗口ID获取窗口句柄*/
static int32_t g_app_window_id = -1;
//图层ID
static int8_t g_app_hour_id = -1,
			  g_app_min_id  = -1,
			  g_app_wday_id = -1,
			  g_app_date_id = -1;

//是否需要刷新所有的时间日期图层
static bool is_need_refresh_datetime = true;

//上一次的蓝牙状态
static bool last_bt_status = true;


//函数声明
static void update_contents();


////////////////////////////////////////////////

//获取蓝牙是否开启
static bool get_bt_status()
{
	uint8_t bt_state = maibu_get_ble_status();  
	if(bt_state==BLEStatusConnected || bt_state==BLEStatusUsing)
		return true;
	return false;
}


static P_Window init_window()
{
	P_Window p_window = app_window_create();
	
	
	//创建背景图层
	LayerBitmap layerbitmap = {0};
	GBitmap gbitmap;
	GRect grect = {{0,0},{128,128}};
	Layer *p_layer;
	
	res_get_user_bitmap(RES_BITMAP_BG, &gbitmap);
	layerbitmap.bitmap = gbitmap;
	layerbitmap.frame = grect;
	p_layer = app_layer_create_bitmap(&layerbitmap);
	app_window_add_layer(p_window, p_layer);
	
	
	//文字图层：小时
	char str[1] = "";
	LayerText layertext = {str, grect, GAlignCenter, U_ASCII_ARIALBD_42, 0};
	
	grect.origin.x = 11;		grect.origin.y = 18;
	grect.size.h   = 42;	    grect.size.w   = 46;
	layertext.frame = grect;
	p_layer = app_layer_create_text(&layertext);
	g_app_hour_id = app_window_add_layer(p_window, p_layer);
	
	
	//文字图层：分钟
	grect.origin.x = 69;		grect.origin.y = 65;
	grect.size.h   = 42;	    grect.size.w   = 46;
	layertext.frame = grect;
	p_layer = app_layer_create_text(&layertext);
	g_app_min_id = app_window_add_layer(p_window, p_layer);
	
	
	//图层：星期
	layertext.font_type = U_ASCII_ARIAL_20;
	layertext.alignment = GAlignTopRight;
	
	grect.origin.x = 12;		grect.origin.y = 70;
	grect.size.h   = 20;	    grect.size.w   = 45;
	layertext.frame = grect;
	p_layer = app_layer_create_text(&layertext);
	g_app_wday_id = app_window_add_layer(p_window, p_layer);
	
	
	//图层：日期
	layertext.alignment = GAlignTopLeft;
	
	grect.origin.x = 71;		grect.origin.y = 40;
	grect.size.h   = 20;	    grect.size.w   = 33;
	layertext.frame = grect;
	p_layer = app_layer_create_text(&layertext);
	g_app_date_id = app_window_add_layer(p_window, p_layer);
	

	return p_window;
}


//事件回调函数
void event_callback(enum SysEventType type, void *context)
{
	if (type == SysEventTypeTimeChange)
		update_contents();
}


//刷新表盘
static void update_contents()
{
	P_Window p_window = app_window_stack_get_window_by_id(g_app_window_id);
	
	//获取当前时间
	struct date_time datetime;
	app_service_get_datetime(&datetime);
		
	
	//修改分钟
	Layer *p_layer;
	char str[10];
	
	p_layer = app_window_get_layer_by_id(p_window, g_app_min_id);
	sprintf(str, "%02d", datetime.min);
	app_layer_set_text_text(p_layer, str);
	
	
	//修改小时	
	if(is_need_refresh_datetime || datetime.min==0) 
	{
		//12小时制处理
		uint8_t hour12 = datetime.hour;
		if(hour12 > 12)
			hour12 -= 12;
		else if(hour12 == 0)
			hour12 = 12;
		
		p_layer = app_window_get_layer_by_id(p_window, g_app_hour_id);
		sprintf(str, "%02d", hour12);
		app_layer_set_text_text(p_layer, str);
	}
	
	
	bool bt_status = get_bt_status();
	
	//修改星期
	if(is_need_refresh_datetime || datetime.hour+datetime.min == 0 || bt_status!=last_bt_status)
	{
		const char wday[7][3]={"Su","Mo","Tu","We","Th","Fr","Sa"}; 
		sprintf(str, " %s", wday[datetime.wday]);
		if(!bt_status)
			str[0] = '.'; //蓝牙特殊标志
		
		p_layer = app_window_get_layer_by_id(p_window, g_app_wday_id);
		app_layer_set_text_text(p_layer, str);
		
		last_bt_status = bt_status;
	}
	
	
	//修改日期
	if(is_need_refresh_datetime || datetime.hour+datetime.min == 0)
	{
		p_layer = app_window_get_layer_by_id(p_window, g_app_date_id);
		sprintf(str, "%d%02d", datetime.mon%10, datetime.mday);
		app_layer_set_text_text(p_layer, str);
	}
	
	//完成修改
	app_window_update(p_window);
	is_need_refresh_datetime = false;
}


int main()
{
    simulator_init();

	g_app_window_id = app_window_stack_push( init_window() );
	
	//首次刷新内容
	is_need_refresh_datetime = true;
	update_contents();
	
	/*注册一个事件通知回调（时间改变）*/
	maibu_service_sys_event_subscribe(event_callback);

    simulator_wait();
	return 0;
}
