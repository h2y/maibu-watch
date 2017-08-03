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


//窗口ID
static int32_t g_window_id = -1;

//当前浏览页面
static uint8_t BGM_flag = 0;


static void update_display(void);



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

	BGM_flag = (BGM_flag + 1) % 2;
	update_display();
}

static void create_layer_bmp(P_Window p_window, GBitmap* bmp, GRect* frame)
{
	LayerBitmap lb = { *bmp,*frame,GAlignCenter };
	P_Layer layer_bmp = app_layer_create_bitmap(&lb);
	app_window_add_layer(p_window, layer_bmp);
}

static void create_layer_text(P_Window p_window, char* str, GRect* frame, int8_t font)
{
	LayerText lt = { str,*frame,GAlignCenter,font,0 };
	P_Layer Layer_text = app_layer_create_text(&lt);
	app_window_add_layer(p_window, Layer_text);
}


static void create_progress_bar(P_Window p_window, uint8_t date_flag, uint8_t percent, ProgressType type)
{
	uint8_t origin_x = 14, width = 6; //TYPE_OLD_EX
	enum GColor color = GColorBlack;

	if (type == TYPE_CUR)
	{
		origin_x = 10;
		width = 14;
	}
	else if (type == TYPE_OLD_IN)
	{
		origin_x = 16;
		width = 2;
		color = GColorWhite;

		if (percent <= 6)
			return;
	}

	//绘制进度条
	GPoint prog_bar_points[4] = { {origin_x + date_flag * 16, 35},
								  {origin_x + width + date_flag * 16, 35},
								  {origin_x + width + date_flag * 16, 95},
								  {origin_x + date_flag * 16, 95} };

	if (percent)
	{
		prog_bar_points[0].y += 60 * (100 - percent) / 100;
		prog_bar_points[1].y += 60 * (100 - percent) / 100;

		if ((type == TYPE_OLD_IN))
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

	Polygon prog_bar = { 4, prog_bar_points };
	Geometry geometry = { GeometryTypePolygon, FillArea, color, (void*)&prog_bar };

	Geometry *p_geometry_array = &geometry;
	LayerGeometry prog_bar_struct = { 1, &p_geometry_array };

	P_Layer layer = app_layer_create_geometry(&prog_bar_struct);
	app_window_add_layer(p_window, layer);
}


//改变一个已有的矩形区域对象
static void change_GRect(GRect *grect, int16_t x, int16_t y, int16_t h, int16_t w)
{
	grect->origin.x = x;
	grect->origin.y = y;
	grect->size.h = h;
	grect->size.w = w;
}


static void update_display(void)
{
	P_Window p_window = app_window_stack_get_window_by_id(g_window_id);
	if (NULL == p_window) return;

	P_Window p_new_window = app_window_create();
	if (NULL == p_new_window) return;


	struct date_time datetime;
	app_service_get_datetime(&datetime);


	/*
		获取七日睡眠数据
	*/

	//七天各自的总睡眠时长
	static uint16_t weekday_sum[7] = { 0 };
	//七天睡眠信息
	static SleepInfo week_info[7] = { 0 };

	uint8_t i = 0, wday;
	for (; i < 7; i++)
	{
		wday = (datetime.wday - 1 + 7 - i) % 7;
		maibu_get_sleep_info(i, &week_info[wday]);

		if (week_info[wday].deepSleep != 0xffff)
		{
			weekday_sum[wday] = week_info[wday].deepSleep + week_info[wday].shallowSleep;
		}
		else
			weekday_sum[wday] = 0;
	}


	//公共变量创建
	GRect frame_bmp = { { 45, 9 },{ 47, 52 } };
	uint32_t* p_frame_bmp = (uint32_t*)&frame_bmp;

	char str_buf[30] = { 0 };
	GBitmap bmp_temp;


	//添加背景图层
	if (BGM_flag == 0)
		res_get_user_bitmap(RES_BITMAP_BMG_1, &bmp_temp);
	else
	{
		change_GRect(&frame_bmp, 0, 8, 104, 128);
		res_get_user_bitmap(RES_BITMAP_BMG_2, &bmp_temp);
	}

	create_layer_bmp(p_new_window, &bmp_temp, &frame_bmp);


	if (BGM_flag == 0)
	{
		//显示睡眠时长
		uint16_t array_num[] = { RES_BITMAP_NO_0,RES_BITMAP_NO_1,RES_BITMAP_NO_2,RES_BITMAP_NO_3,RES_BITMAP_NO_4,RES_BITMAP_NO_5,RES_BITMAP_NO_6,RES_BITMAP_NO_7,RES_BITMAP_NO_8,RES_BITMAP_NO_9 };

        uint8_t yesterday_index = datetime.wday - 1 - 1;

        
		// frame_bmp: 28,31,26,12
		*p_frame_bmp = 0x0c1a1f1c;
		res_get_user_bitmap(array_num[weekday_sum[yesterday_index] / 60 / 10], &bmp_temp);
		create_layer_bmp(p_new_window, &bmp_temp, &frame_bmp);

		frame_bmp.origin.x += 12;
		res_get_user_bitmap(array_num[(weekday_sum[yesterday_index] / 60) % 10], &bmp_temp);
		create_layer_bmp(p_new_window, &bmp_temp, &frame_bmp);

		frame_bmp.origin.x += 24;
		res_get_user_bitmap(array_num[weekday_sum[yesterday_index] % 60 / 10], &bmp_temp);
		create_layer_bmp(p_new_window, &bmp_temp, &frame_bmp);

		frame_bmp.origin.x += 12;
		res_get_user_bitmap(array_num[weekday_sum[yesterday_index] % 60 % 10], &bmp_temp);
		create_layer_bmp(p_new_window, &bmp_temp, &frame_bmp);


		//如果有睡眠数据
		if (weekday_sum[yesterday_index])
		{
			//显示睡眠质量
			change_GRect(&frame_bmp, 57, 62, 26, 14);

			uint32_t res_bitmap_id = RES_BITMAP_LV_0; //case 3
			switch (week_info[yesterday_index].sleepQuality)
			{
				case 1: res_bitmap_id = RES_BITMAP_LV_2; break;
				case 0: res_bitmap_id = RES_BITMAP_LV_3; break;
				case 2: res_bitmap_id = RES_BITMAP_LV_1; 
			}

			res_get_user_bitmap(res_bitmap_id, &bmp_temp);
			create_layer_bmp(p_new_window, &bmp_temp, &frame_bmp);

			//显示睡眠时间
			change_GRect(&frame_bmp, 14, 93, 14, 100);
			sprintf(str_buf, "%d:%02d - %d:%02d", \
				week_info[yesterday_index].sleepTime.hour, \
				week_info[yesterday_index].sleepTime.min, \
				week_info[yesterday_index].wakeTime.hour, \
				week_info[yesterday_index].wakeTime.min);
			create_layer_text(p_new_window, str_buf, &frame_bmp, U_ASCII_ARIALBD_14);

			//显示睡眠数据
			change_GRect(&frame_bmp, 0, 110, 14, 128);
			sprintf(str_buf, "深睡%d%% 清醒%d次", \
				(100 * week_info[yesterday_index].deepSleep + 50) / weekday_sum[yesterday_index], \
				week_info[yesterday_index].wakeCount);
			create_layer_text(p_new_window, str_buf, &frame_bmp, U_ASCII_ARIAL_14);
		}

	}
	else //if (BGM_flag == 1)
	{
		uint8_t i = 0;
		uint16_t max_sleep_time = 1;
		ProgressType type = TYPE_CUR;

		//七天中有数据的天数
		uint8_t week_info_num = 0;
		//七天睡眠总数、平均数
		uint16_t week_time_sum = 0,
			week_time_avg = 0;


		for (i = 0; i < 7; i++)
		{
			if (weekday_sum[i])
			{
				week_time_sum += weekday_sum[i];
				week_info_num++;

				if (max_sleep_time < weekday_sum[i])
					max_sleep_time = weekday_sum[i];
			}
		}

		week_time_avg = week_info_num ? week_time_sum / week_info_num : 0;


		for (i = 0; i < 7; i++)
		{
			uint8_t percent = weekday_sum[i] * 100 / max_sleep_time;

			if (i < datetime.wday - 1)
				create_progress_bar(p_new_window, i, percent, TYPE_CUR);
			else
			{
				create_progress_bar(p_new_window, i, percent, TYPE_OLD_EX);
				create_progress_bar(p_new_window, i, percent, TYPE_OLD_IN);
			}
		}

		//GRect frame_bmp = {{0,114},{14,128}};
		*p_frame_bmp = 0x800e7100;
		if (week_info_num > 0)
			sprintf(str_buf, "%d日平均睡眠%d时%d分", week_info_num, week_time_avg / 60, week_time_avg % 60);
		else
			sprintf(str_buf, "暂无统计");

		create_layer_text(p_new_window, str_buf, &frame_bmp, U_GBK_SIMSUN_12);
	}


	/*添加窗口按键事件*/
	app_window_click_subscribe(p_new_window, ButtonIdDown, button_select_up_down);
	app_window_click_subscribe(p_new_window, ButtonIdUp,   button_select_up_down);
	app_window_click_subscribe(p_new_window, ButtonIdBack, button_select_back);

	g_window_id = app_window_stack_replace_window(p_window, p_new_window);
}


int main(void)
{
	g_window_id = app_window_stack_push(app_window_create());

	update_display();

	return 0;
}
