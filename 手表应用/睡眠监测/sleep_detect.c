#include "Maibu_sdk.h"
#include "maibu_res.h"


typedef enum ProgressType
{
	TYPE_CUR,	//当前类型
	TYPE_OLD_IN,//上周类型,内框
	TYPE_OLD_EX,//上周类型,外框
}ProgressType;

typedef struct SleepInfo
{
	uint16_t deepSleep;		//深睡眠时长,单位分钟
	uint16_t shallowSleep;	//浅睡眠时长,单位分钟

	uint8_t wakeCount;		//醒来的次数
	uint8_t sleepQuality;	//睡眠质量
	uint8_t flag;			//bit 1表示是否需要更新数据
	uint8_t reserved;		//保留位
	
	struct date_time sleepTime;	//入睡时间
	struct date_time wakeTime;	//起床时间
}SleepInfo;

//七天睡眠信息
static SleepInfo week_info[7] = {0};

//七天中有数据的天数
static uint8_t week_info_num = 0;
//七天睡眠平均分钟数
static uint16_t week_time_avg = 0;


//---------------------界面使用的参数---------------------
static int32_t g_window_id = -1;

static uint8_t BGM_flag = 0;


void update_display(void);

uint16_t getSleepHours(uint8_t wday)
{
	return (week_info[wday].shallowSleep + week_info[wday].deepSleep)/60;
}
uint16_t getSleepMins(uint8_t wday)
{
	return (week_info[wday].shallowSleep + week_info[wday].deepSleep)%60;
}

bool isSleepTimeEmpty(uint8_t wday)
{
	return (week_info[wday].shallowSleep + week_info[wday].deepSleep > 0) 
		? false : true;
}

uint32_t get_sleep_level(uint8_t wday)
{
	uint32_t array_level[] = {RES_BITMAP_LV_3,RES_BITMAP_LV_2,RES_BITMAP_LV_1,RES_BITMAP_LV_0};

	return array_level[week_info[wday].sleepQuality];
}



void button_select_back(void *context)
{
	P_Window p_window = (P_Window)context;
	if (NULL != p_window)
		app_window_stack_pop(p_window);
}
void button_select_up_down(void *context)
{
	P_Window p_window = (P_Window)context;		
	if (p_window == NULL)
		return;

	BGM_flag = (BGM_flag + 1)%2;
	update_display();
}

void create_layer_bmp(P_Window p_window,GBitmap* bmp,GRect frame,enum GAlign align,enum GColor color)
{
	LayerBitmap lb = {*bmp,frame,align};
	P_Layer layer_bmp = app_layer_create_bitmap(&lb);
	app_layer_set_bg_color(layer_bmp,color);
	app_window_add_layer(p_window,layer_bmp);
}

int8_t create_layer_text(P_Window p_window,char* str,GRect frame,enum GAlign align,int8_t font,enum GColor color)
{
	LayerText lt = {str,frame,align,font,0};
	P_Layer Layer_text = app_layer_create_text(&lt);
	app_layer_set_bg_color(Layer_text, color);
	return app_window_add_layer(p_window,Layer_text);
}

void create_progress_bar(P_Window p_window,uint8_t date_flag,uint8_t percent,ProgressType type )
{
	Geometry *p_geometry_array[1];

	uint8_t origin_x = 0,width = 0;
	enum GColor color;

	if(type == TYPE_CUR)
	{
		origin_x = 10;
		width = 14;
		color = GColorBlack;
	}
	else if(type == TYPE_OLD_EX)
	{
		origin_x = 14;
		width = 6;
		color = GColorBlack;
	}
	else if(type == TYPE_OLD_IN)
	{
		origin_x = 16;
		width = 2;
		color = GColorWhite;

		if(percent <= 6)
			return;
	}
	
	//绘制进度条
	GPoint prog_bar_points[4] = { {origin_x + date_flag *16, 35}, 
								  {origin_x+width+ date_flag *16, 35}, 
								  {origin_x+width+ date_flag *16, 95}, 
								  {origin_x+ date_flag *16, 95} };

	if((percent>0)&&(100 >= percent ))
	{
		prog_bar_points[0].y += 60*(100-percent)/100;
		prog_bar_points[1].y += 60*(100-percent)/100;

		if((type == TYPE_OLD_IN))
		{
			prog_bar_points[0].y += 2;
			prog_bar_points[1].y += 2;
		}
		
	}
	else
	{
		prog_bar_points[0].y += 59;
		prog_bar_points[1].y += 59;
	}
	
	Polygon prog_bar   = {4, prog_bar_points};
	Geometry geometry = {GeometryTypePolygon, FillArea, color,(void*)&prog_bar}; 
	
	p_geometry_array[0] = &geometry;
	LayerGeometry prog_bar_struct = {1, p_geometry_array};
	
	P_Layer	 layer = app_layer_create_geometry(&prog_bar_struct);
	app_window_add_layer(p_window, layer);
}

//改变一个已有的矩形区域对象
void change_GRect(GRect *grect, int16_t x, int16_t y, int16_t h, int16_t w)
{
	grect->origin.x = x;
	grect->origin.y = y;
	grect->size.h = h;
	grect->size.w = w;
}


void update_display(void)
{
	P_Window p_window = app_window_stack_get_window_by_id(g_window_id);	
	if (NULL == p_window)
		return;

	P_Window p_new_window = app_window_create();
	if (NULL == p_new_window)
		return;
		
	//配置参数
    uint16_t array[] = {RES_BITMAP_BMG_1, RES_BITMAP_BMG_2};
	
	//添加背景图层
	GBitmap bmp_bg;
	GRect frame_bg = {{0, 0}, {128, 128}};						
	
	res_get_user_bitmap(array[BGM_flag], &bmp_bg);
	LayerBitmap layer_bmp_bg_struct = {bmp_bg, frame_bg, GAlignCenter};
	P_Layer layer_bmp_bg = app_layer_create_bitmap(&layer_bmp_bg_struct);
	app_layer_set_bg_color(layer_bmp_bg, GColorWhite);
	app_window_add_layer(p_new_window, layer_bmp_bg);

	GRect frame_bmp = {0};
	uint32_t* p_frame_bmp = (uint32_t*)&frame_bmp;
	char str_buf[60] = {0};

	
	struct date_time datetime;
	app_service_get_datetime(&datetime);
	
	if(BGM_flag == 0)
	{
		//显示睡眠时长
		uint16_t array_num[] = {RES_BITMAP_NO_0,RES_BITMAP_NO_1,RES_BITMAP_NO_2,RES_BITMAP_NO_3,RES_BITMAP_NO_4,RES_BITMAP_NO_5,RES_BITMAP_NO_6,RES_BITMAP_NO_7,RES_BITMAP_NO_8,RES_BITMAP_NO_9};

		GBitmap bmp_temp;
		
		// frame_bmp: 28,31,26,12
		*p_frame_bmp = 0x0c1a1f1c;
		res_get_user_bitmap(array_num[getSleepHours((datetime.wday))/10],&bmp_temp);
		create_layer_bmp(p_new_window,&bmp_temp,frame_bmp,GAlignCenter,GColorWhite);

		frame_bmp.origin.x += 12;
		res_get_user_bitmap(array_num[getSleepHours((datetime.wday))%10],&bmp_temp);
		create_layer_bmp(p_new_window,&bmp_temp,frame_bmp,GAlignCenter,GColorWhite);

		frame_bmp.origin.x += 24;
		res_get_user_bitmap(array_num[getSleepMins((datetime.wday))/10],&bmp_temp);
		create_layer_bmp(p_new_window,&bmp_temp,frame_bmp,GAlignCenter,GColorWhite);
		
		frame_bmp.origin.x += 12;
		res_get_user_bitmap(array_num[getSleepMins((datetime.wday))%10],&bmp_temp);
		create_layer_bmp(p_new_window,&bmp_temp,frame_bmp,GAlignCenter,GColorWhite);


		//如果有睡眠数据
		if( !isSleepTimeEmpty(datetime.wday) )
		{
			//显示睡眠质量
			change_GRect(&frame_bmp, 57, 62, 26, 14);
			res_get_user_bitmap(get_sleep_level((datetime.wday)),&bmp_temp);
			create_layer_bmp(p_new_window,&bmp_temp,frame_bmp,GAlignCenter,GColorWhite);
			
			//显示睡眠时间
			change_GRect(&frame_bmp, 0, 93, 14, 128);
			sprintf(str_buf,"%d:%02d - %d:%02d",\
				week_info[datetime.wday].sleepTime.hour,\
				week_info[datetime.wday].sleepTime.min,\
				week_info[datetime.wday].wakeTime.hour,\
				week_info[datetime.wday].wakeTime.min);
			create_layer_text(p_new_window,str_buf,frame_bmp,GAlignCenter,U_ASCII_ARIALBD_14,GColorWhite);
			
			//显示睡眠数据
			change_GRect(&frame_bmp, 0, 110, 14, 128);
			sprintf(str_buf,"深睡%d%% 清醒%d次", \
				(100 * week_info[datetime.wday].deepSleep + 50) / (week_info[datetime.wday].deepSleep + week_info[datetime.wday].shallowSleep), \
			    week_info[((datetime.wday))].wakeCount);
			create_layer_text(p_new_window,str_buf,frame_bmp,GAlignCenter,U_ASCII_ARIAL_14,GColorWhite);
		}
		
	}
	else if(BGM_flag == 1)
	{
		//SleepInfo week_info[7] = {0};
		uint8_t i = 0;
		uint16_t max_sleep_time = 1;
		ProgressType type = TYPE_CUR;
		
		//getWeekSleepInfo(week_info);

		for(i = 0; i < 7; i++)
		{
			if((week_info[i].deepSleep) != 0xffff)
			{
				max_sleep_time = (max_sleep_time>(week_info[i].deepSleep + week_info[i].shallowSleep))?(max_sleep_time):(week_info[i].deepSleep + week_info[i].shallowSleep);
			}
		}

		for(i = 1;i <= 7;i++)
		{
			uint8_t percent = 0;
			if((week_info[i%7].deepSleep) != 0xffff)
			{
				percent = (week_info[i%7].deepSleep + week_info[i%7].shallowSleep)*100/max_sleep_time;
			
			}
			if(i <= datetime.wday)
			{
				create_progress_bar(p_new_window,i-1,percent,TYPE_CUR);
			}
			else
			{
				create_progress_bar(p_new_window,i-1,percent,TYPE_OLD_EX);
				create_progress_bar(p_new_window,i-1,percent,TYPE_OLD_IN);
			}

		}

		//GRect frame_bmp = {{0,114},{14,128}};
		*p_frame_bmp = 0x800e7100;
		str_buf[0] = '\0';
		if(week_info_num > 0)
			sprintf(str_buf,"%d日平均时长%d时%d分", week_info_num, week_time_avg/60, week_time_avg%60);
		else
			sprintf(str_buf,"暂无统计");

		create_layer_text(p_new_window,str_buf,frame_bmp,GAlignCenter,U_GBK_SIMSUN_12,GColorWhite);
	}


	/*添加窗口按键事件*/
	app_window_click_subscribe(p_new_window, ButtonIdDown, button_select_up_down);
	app_window_click_subscribe(p_new_window, ButtonIdUp, button_select_up_down);
	app_window_click_subscribe(p_new_window, ButtonIdBack, button_select_back);

	g_window_id = app_window_stack_replace_window(p_window, p_new_window);	
}

P_Window init_window()
{
	P_Window p_window = app_window_create();
	if (NULL == p_window)
		return NULL;

	app_window_click_subscribe(p_window, ButtonIdBack, button_select_back);
	app_window_click_subscribe(p_window, ButtonIdDown, button_select_up_down);
	app_window_click_subscribe(p_window, ButtonIdUp, button_select_up_down);

	return p_window;
}


void getWeekSleepInfo(void)
{
	struct date_time datetime;
	app_service_get_datetime(&datetime);

	//统计清零
	week_info_num = 0;


	uint32_t oneDaySum, 
			 sum = 0;
	uint8_t i = 0,
		    wday;
	for(; i<7; i++)
	{
		wday = (datetime.wday + 7 - i) % 7;
		maibu_get_sleep_info(i, &week_info[wday] );
		
		if( week_info[wday].deepSleep != 0xffff)
		{
			oneDaySum = week_info[wday].deepSleep + week_info[wday].shallowSleep;
			if(oneDaySum>0) 
			{
				week_info_num++;
				sum += oneDaySum;
			}
		}
	}
	
	week_time_avg = week_info_num? sum/week_info_num : 0;
}


int main(void)
{	
	getWeekSleepInfo();
	/*创建消息列表窗口*/
	P_Window p_window = init_window(); 
	
	/*放入窗口栈显示*/
	g_window_id = app_window_stack_push(p_window);
	
	update_display();

	return 0;
}
