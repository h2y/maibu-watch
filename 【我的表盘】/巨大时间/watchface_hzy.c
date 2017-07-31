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

/*小时分钟图层ID，通过该图层ID获取图层句柄*/
static int8_t g_app_hm_layer_id 	= -1,
			  g_app_wmd_layer_id 	= -1, //月周日图层ID
			  g_app_words_layer_id 	= -1, //顶部文字图层ID
			  g_app_bg_layer_id     = -1; //背景图图层ID
			  
//昼夜模式
//0为正常模式 1为反转模式
static bool color_mode = 0;

/*当前日记录*/
static uint8_t g_official_day = -1;


static get_true_BGcolor(bool white_as_0)
{
	if(color_mode)
		white_as_0 = !white_as_0;
	return white_as_0? GColorBlack : GColorWhite;
}


static void app_watch_update()
{
	/*根据窗口ID获取窗口句柄*/
	P_Window p_window = app_window_stack_get_window_by_id(g_app_window_id);	
	if (NULL == p_window)
		return;

	//获取当前时间
	struct date_time datetime;
	app_service_get_datetime(&datetime);
	//字符串
	char str[20] = "";
	//共用的图层指针
	P_Layer p_layer;
	//临时变量
	int8_t int8_temp;
	//是否需要更换昼夜配色 切换1 不切换0
	bool needChangeColor = 0;
	
	//判断是否需要更换配色 7.~19.
	int8_temp = (datetime.hour + 5) % 24;
	if(color_mode && int8_temp>=13)
		needChangeColor = 1;
	else if(!color_mode && int8_temp<13)
		needChangeColor = 1;
		
	if(needChangeColor)
		color_mode = !color_mode;
	
	
	//上面显示的话
	p_layer = app_window_get_layer_by_id(p_window, g_app_words_layer_id);
	if(needChangeColor)
		app_layer_set_bg_color(p_layer, get_true_BGcolor(0) );
	
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
	app_layer_set_text_text(p_layer, str);	
	
	
	//12小时制处理
	if(datetime.hour > 12)
		datetime.hour -= 12;
	else if(datetime.hour == 0)
		datetime.hour = 12;
		
	//中间时间更新
	p_layer = app_window_get_layer_by_id(p_window, g_app_hm_layer_id);
	if(needChangeColor)
		app_layer_set_bg_color(p_layer, get_true_BGcolor(0) );
	sprintf(str, "%02d:%02d", datetime.hour, datetime.min);
	app_layer_set_text_text(p_layer, str);	
	
	
	/*每天凌晨更新日期*/	
	if ( datetime.mday != g_official_day )
	{
		g_official_day = datetime.mday;
		char wday[7][4]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"}; 
		
		p_layer = app_window_get_layer_by_id(p_window, g_app_wmd_layer_id);
		if(needChangeColor)
			app_layer_set_bg_color(p_layer, get_true_BGcolor(1) );
		sprintf(str, "%02d - %s - %02d", datetime.mon, wday[datetime.wday], datetime.mday);
		app_layer_set_text_text(p_layer, str);	
	}
	
	
	//背景图
	if(needChangeColor) {
		p_layer = app_window_get_layer_by_id(p_window, g_app_bg_layer_id);
		GBitmap gbitmap;
		res_get_user_bitmap(color_mode? RES_BITMAP_BG1:RES_BITMAP_BG0, &gbitmap);
		app_layer_set_bitmap_bitmap(p_layer, &gbitmap);
	}
	
	
	app_window_update(p_window);
}


static P_Window init_window()
{
	P_Window p_window = app_window_create();
	if (NULL == p_window) return;

	//clear dirty data
	color_mode = 0;
	g_official_day = -1;
	
	//图层共用的矩形区域对象
	GRect grect = {{0,0},{0,128}};
	//共用的Layer*指针（创建图层的结果）
	Layer *p_layer;
	//用于创建文字图层
	LayerText layertext = {"", grect, GAlignCenter, U_ASCII_ARIALBD_42, 0};
	//用于创建bmp图层
	LayerBitmap layerbitmap = {0};
	GBitmap gbitmap;
	

	//背景图层
	grect.origin.x = 0;		grect.origin.y = 0;
	grect.size.h   = 128;	grect.size.w   = 128;
	res_get_user_bitmap(RES_BITMAP_BG0, &gbitmap);
	layerbitmap.bitmap = gbitmap;
	layerbitmap.frame = grect;
	layerbitmap.alignment = GAlignCenter;
	p_layer = app_layer_create_bitmap(&layerbitmap);
	g_app_bg_layer_id = app_window_add_layer(p_window, p_layer);

	/*添加小时分钟图层*/
	grect.origin.x = 0;		grect.origin.y = 37;
	grect.size.h   = 42;	grect.size.w   = 128;
	layertext.frame = grect;
	layertext.font_type = U_ASCII_ARIALBD_42;
	p_layer = app_layer_create_text(&layertext);
	app_layer_set_bg_color(p_layer, get_true_BGcolor(0) );
	g_app_hm_layer_id = app_window_add_layer(p_window, p_layer);

	/*添加星期月图层*/
	grect.origin.x = 0;		grect.origin.y = 107;
	grect.size.h   = 14;	grect.size.w   = 128;
	layertext.frame = grect;
	layertext.font_type = U_ASCII_ARIALBD_14;
	p_layer = app_layer_create_text(&layertext);
	app_layer_set_bg_color(p_layer, get_true_BGcolor(1) );
	g_app_wmd_layer_id = app_window_add_layer(p_window, p_layer);
	
	//顶部文字图层
	grect.origin.x = 0;		grect.origin.y = 11;
	grect.size.h   = 14;	grect.size.w   = 128;
	layertext.frame = grect;
	layertext.font_type = U_GBK_SIMSUN_14;
	p_layer = app_layer_create_text(&layertext);
	app_layer_set_bg_color(p_layer, get_true_BGcolor(0) );
	g_app_words_layer_id = app_window_add_layer(p_window, p_layer);

	return p_window;
}


void app_jdsj_watch_time_change(enum SysEventType type, void *context)
{
	if (type == SysEventTypeTimeChange)
		app_watch_update();
}


int main()
{
    //simulator_init();

	P_Window p_window = init_window(); 
	g_app_window_id = app_window_stack_push(p_window);
	
	//立即刷新一次内容
	app_watch_update();
	
	/*注册一个事件通知回调，当有时间改变，立即更新时间*/
	maibu_service_sys_event_subscribe(app_jdsj_watch_time_change);

    //simulator_wait();
	return 0;
}
