// TODO: a new UUID
//TODO: 返回键切歌

#include "maibu_res.h"
#include "maibu_sdk.h"


//默认显示的文字
#define DEFAULT_WORDS_HZY "~ hello, hzy ~"
//蓝牙断开文字
#define NO_BT_WORDS_HZY "! disconnected !"
//低电量百分值和文字
#define LOW_BATTERY_LIMIT_HZY 10
#define LOW_BATTERY_WORDS_HZY "> no energy <"

/*窗口ID, 通过该窗口ID获取窗口句柄*/
static int32_t g_app_window_id = -1;
			  

static P_Window init_window()
{
	P_Window p_window = app_window_create();
	
	//共用变量
	int8_t int8_temp;
	char str[20] = "Ti:ME";
	//图层共用的矩形区域对象
	GRect grect = {{0,0},{128,128}};
	//共用的Layer*指针（创建图层的结果）
	Layer *p_layer;
	//用于创建文字图层
	LayerText layertext = {str, grect, GAlignCenter, U_ASCII_ARIALBD_42, 0};
	//用于创建bmp图层
	LayerBitmap layerbitmap = {0};
	GBitmap gbitmap;
	//获取当前时间
	struct date_time datetime;
	app_service_get_datetime(&datetime);
	//昼夜模式
	bool color_mode = (datetime.hour<8 || datetime.hour>19);
	
	
	//12小时制处理
	if(datetime.hour > 12)
		datetime.hour -= 12;
	else if(datetime.hour == 0)
		datetime.hour = 12;
		

	//背景图层
	res_get_user_bitmap(color_mode? RES_BITMAP_BG1:RES_BITMAP_BG0, &gbitmap);
	
	//grect.origin.x = 0;		grect.origin.y = 0;
	//grect.size.h   = 128;	    grect.size.w   = 128;
	layerbitmap.bitmap = gbitmap;
	layerbitmap.frame = grect;
	layerbitmap.alignment = GAlignCenter;
	p_layer = app_layer_create_bitmap(&layerbitmap);
	app_window_add_layer(p_window, p_layer);


	/*添加小时分钟图层*/
	sprintf(str, "%02d:%02d", datetime.hour, datetime.min);
	
	grect.origin.y = 37;
	grect.size.h   = 42;
	layertext.frame = grect;
	layertext.font_type = U_ASCII_ARIALBD_42;
	p_layer = app_layer_create_text(&layertext);
	app_layer_set_bg_color(p_layer, color_mode );
	app_window_add_layer(p_window, p_layer);


	/*添加星期月图层*/
	char wday[7][4]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"}; 
	sprintf(str, "%02d - %s - %02d", datetime.mon, wday[datetime.wday], datetime.mday);
	
	grect.origin.y = 107;
	grect.size.h   = 14;	
	layertext.frame = grect;
	layertext.font_type = U_ASCII_ARIALBD_14;
	p_layer = app_layer_create_text(&layertext);
	app_layer_set_bg_color(p_layer, !color_mode );
	app_window_add_layer(p_window, p_layer);
	
	
	//顶部文字图层
	int8_temp = maibu_get_ble_status();
	if( int8_temp == BLEStatusAdvertising || int8_temp == BLEStatusClose )
		sprintf(str, "%s", NO_BT_WORDS_HZY);
	else
	{
		maibu_get_battery_percent(&int8_temp);
		if(int8_temp < LOW_BATTERY_LIMIT_HZY)
			sprintf(str, "%s", LOW_BATTERY_WORDS_HZY);
		else
			sprintf(str, "%s", DEFAULT_WORDS_HZY);
	}
	
	grect.origin.y = 11;
	grect.size.h   = 14;
	layertext.frame = grect;
	layertext.font_type = U_GBK_SIMSUN_14;
	p_layer = app_layer_create_text(&layertext);
	app_layer_set_bg_color(p_layer, color_mode);
	app_window_add_layer(p_window, p_layer);


	return p_window;
}


void app_watch_time_change(enum SysEventType type, void *context)
{
	if (type != SysEventTypeTimeChange)
		return;

	P_Window p_old_window = app_window_stack_get_window_by_id(g_app_window_id); 
	P_Window p_window = init_window();
	g_app_window_id = app_window_stack_replace_window(p_old_window, p_window);
}


int main()
{
    simulator_init();

	P_Window p_window = init_window(); 
	g_app_window_id = app_window_stack_push(p_window);
	
	/*注册一个事件通知回调，当有时间改变，立即更新时间*/
	maibu_service_sys_event_subscribe(app_watch_time_change);

    simulator_wait();
	return 0;
}
