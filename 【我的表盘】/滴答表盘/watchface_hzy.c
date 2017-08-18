//TODO: 手机任务修改是否会同步到手表，以及同步的时间
//TODO: 手表上完成当前任务
//TODO：协议代码精简重构

#include "maibu_res.h"
#include "maibu_sdk.h"


//默认显示的文字
#define DEFAULT_WORDS_HZY "~ nice day ~"
//低电量百分值和文字
#define LOW_BATTERY_LIMIT_HZY 20
#define LOW_BATTERY_WORDS_HZY "> no energy <"

//滴答清单中定时任务进入轮流显示的提前时间（秒）
#define DIDA_SHOW_BEFORE_TIME 1800 //30min


/*窗口ID, 通过该窗口ID获取窗口句柄*/
static int32_t g_app_window_id = -1;
//图层ID
static int8_t g_app_hour_id = -1,
			  g_app_min_id  = -1,
			  g_app_wday_id = -1,
			  g_app_date_id = -1,
			  g_app_str_id  = -1;

//是否需要刷新所有的时间日期图层
static bool is_need_refresh_datetime = true;

//上一次的蓝牙状态
static bool last_bt_status = true;


//函数声明
static void update_contents();


/*********************
	滴答清单相关
**********************/

/*滴答通讯协议*/
#define PROTOCOL_UPDATE_LIST			0x01	
#define PROTOCOL_CHECK_LIST				0x02
#define PROTOCOL_GET_CHECK_LIST			0x03
#define PROTOCOL_UPDATE_LIST_ACK		0x21	
#define PROTOCOL_CHECK_LIST_ACK			0x22
#define PROTOCOL_GET_LIST_ACK			0x23

/*通讯相关*/
/*通讯ID：更新任务应答*/
static int32_t g_comm_id_update_list_ack = -1;
static int32_t g_comm_id_check_ok = -1;
static int32_t g_comm_id_get_list_ack = -1;
/*Linkid*/
static char g_link_id[10] = "";

/*上一个check的任务ID*/
static int32_t g_task_id = -1;

/*应答错误码*/
static int8_t g_error_code_update_ack = 0;

/*任务项结构体*/
typedef struct tag_STaskItem
{
	uint32_t id;	//任务ID
	uint32_t date;	//任务日期时间
	char title[24];	//任务标题
}STaskItem;

/*任务项信息*/
typedef struct tag_STaskInfo
{
	int8_t checked;	//是否已check	0 未check 1 checking  2 checked
	int8_t used;	//是否已使用	0 未使用 1 已使用
	int8_t alarm;	//是否已提醒，只针对Android
	STaskItem item;
}STaskInfo;

#define TASK_LIST_MAX_SIZE	5
#define NOT_USED	0
#define USED	1
#define NOT_HAVE_CHECKED	0
#define CHECKING		1
#define HAVE_CHECKED		2
#define NOT_HAVE_ALARM		0
#define HAVE_ALARM		1

/*任务列表结构体*/
typedef struct tag_STaskInfoList
{
	uint8_t nums;	//当前任务项个数
	STaskInfo list[TASK_LIST_MAX_SIZE];	//任务项列表
}STaskInfoList;

/*任务列表*/
static STaskInfoList g_s_task_list = { 0 };

//即将显示到表盘上的任务名
static char dida_task_str[24] = {0};


///////////////////////////////////////////////////////

//发送更新任务列表应答包
static int32_t send_update_task_list_ack()
{
	uint8_t ack[5] = { 0xdd, 0x01, 0x21, 0x01, g_error_code_update_ack };

	return maibu_comm_send_msg(g_link_id, ack, sizeof(ack));
}


// 发送获取任务列表应答
static int32_t send_get_task_list_ack()
{
	uint8_t ack[30] = { 0xdd, 0x01, 0x23 };
	
	int8_t i = 0, count = 0;
	for (; i < TASK_LIST_MAX_SIZE; i++)
	{
		if (g_s_task_list.list[i].checked == CHECKING)
		{
			memcpy(&ack[4 + sizeof(int32_t)*count], (char *)&g_s_task_list.list[i].item.id, sizeof(int32_t));
			count++;
		}
	}
	ack[3] = sizeof(int32_t)*count;

	return maibu_comm_send_msg(g_link_id, ack, ack[3] + 4);
}


//找到一个 统计全天型任务、快开始or已开始的任务
//将任务名存放到 dida_task_str[] 中
static void get_a_task_title_to_watchface()
{
	dida_task_str[0] = '\0';
	
	//TODO:ios
	if (maibu_get_phone_type() == PhoneTypeIOS)
		return;
		
	//获取当前时间秒数(从1970开始)
	struct date_time datetime;
	app_service_get_datetime(&datetime);
	uint32_t seconds = app_get_time(&datetime);
	
	
	uint8_t i0 = datetime.min, //random start position
			i1, i;
			
	for(i1=0; i1<TASK_LIST_MAX_SIZE; i1++)
	{
		i = (i0 + i1) % TASK_LIST_MAX_SIZE;
		
		if(g_s_task_list.list[i].used == NOT_USED)
			continue;
		
		if(g_s_task_list.list[i].item.date == 0 || g_s_task_list.list[i].item.date - seconds > DIDA_SHOW_BEFORE_TIME)
		{
			strcpy(dida_task_str, g_s_task_list.list[i].item.title);
			break;
		}
	}
}


/* 
	处理手机第三方APP协议数据 回调
*/
void msg_recv_callback(const char *link_id, const uint8_t *buff, uint16_t size)
{
	strcpy(g_link_id, link_id);

	/*是否滴答协议*/
	if ((size < 3) || (0xDD != buff[0]) || (0x01 != buff[1]))
		return;

	//todo
	maibu_service_vibes_pulse(VibesPulseTypeShort, 1); 

	/*如果协议是更新任务列表*/
	if (buff[2] == PROTOCOL_UPDATE_LIST)
	{
		/*长度不对，或者数据结构不对*/
		if ((size < 4) || (size != (4 + buff[3])))
		{
			/*发送错误应答*/
			g_error_code_update_ack = -2;
			g_comm_id_update_list_ack = send_update_task_list_ack();
			return;
		}

		/*保存任务列表,之前的信息全部清空*/
		memset(&g_s_task_list, 0, sizeof(STaskInfoList));
		g_s_task_list.nums = buff[3] / sizeof(STaskItem);
		int i = 0;
		for (i = 0; i < g_s_task_list.nums; i++)
		{
			memcpy(&g_s_task_list.list[i].item, &buff[4 + sizeof(STaskItem)*i], sizeof(STaskItem));
			g_s_task_list.list[i].used = USED;
		}

		/*发送成功应答*/
		g_error_code_update_ack = 0;
		g_comm_id_update_list_ack = send_update_task_list_ack();
	}
	/*完成任务命令应答*/
	else if (buff[2] == PROTOCOL_CHECK_LIST_ACK)
	{
		/*获取完成任务的ID*/
		int32_t id_checked = 0;
		int8_t id_nums = 0;
		id_nums = buff[3] / sizeof(int32_t);
		int8_t i = 0, j = 0;
		for (i = 0; i < id_nums; i++)
		{
			memcpy(&id_checked, &buff[4 + sizeof(int32_t)*i], sizeof(int32_t));
			for (j = 0; j < TASK_LIST_MAX_SIZE; j++)
				if (g_s_task_list.list[j].item.id == id_checked)
					memset(&g_s_task_list.list[j], 0, sizeof(STaskInfo));
		}
	}
	/*获取已check任务列表*/
	else if (buff[2] == PROTOCOL_GET_CHECK_LIST)
	{
		g_comm_id_get_list_ack = send_get_task_list_ack();
	}

	/*更新窗口, 获取已check任务列表也会更新菜单，但是必须在收到通讯成功应答后*/
	if ((buff[2] == PROTOCOL_CHECK_LIST_ACK) || (buff[2] == PROTOCOL_UPDATE_LIST))
	{
		//TODO
		//update_dida_menu();
	}
}


// 通讯结果回调
void msg_result_callback(enum ECommResult status, uint32_t comm_id, void *context)
{
	/*如果更新任务应答发送失败，则重发*/
	if ((ECommResultFail == status) && (comm_id == g_comm_id_update_list_ack))
	{
		send_update_task_list_ack();
	}

	/*如果任务完成发送失败，则重发*/
	if ((ECommResultFail == status) && (comm_id == g_comm_id_check_ok))
	{}

	/*如果任务完成发送失败，则重发*/
	if ((ECommResultFail == status) && (comm_id == g_comm_id_get_list_ack))
	{
		send_get_task_list_ack();
	}
	/*成功，则清除*/
	else if ((ECommResultSuccess == status) && (comm_id == g_comm_id_get_list_ack))
	{
#if 0
		int8_t i = 0;
		for (i = 0; i < TASK_LIST_MAX_SIZE; i++)
		{
			if (g_s_task_list.list[i].checked == CHECKING)
			{
				memset(&g_s_task_list.list[i], 0, sizeof(STaskInfo));
			}
		}

		/*更新菜单*/
		update_dida_menu();
#endif
	}
}








/*********************
	表盘相关
**********************/

//获取蓝牙是否开启
static bool get_bt_status()
{
	uint8_t bt_state = maibu_get_ble_status();  
	if(bt_state==BLEStatusConnected || bt_state==BLEStatusUsing)
		return true;
	return false;
}


void button_back_from_watchface(void *context)
{
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
	
	grect.origin.x = 11;		grect.origin.y = 4;
	grect.size.h   = 42;	    grect.size.w   = 46;
	layertext.frame = grect;
	p_layer = app_layer_create_text(&layertext);
	g_app_hour_id = app_window_add_layer(p_window, p_layer);
	
	
	//文字图层：分钟
	grect.origin.x = 69;		grect.origin.y = 51;
	grect.size.h   = 42;	    grect.size.w   = 46;
	layertext.frame = grect;
	p_layer = app_layer_create_text(&layertext);
	g_app_min_id = app_window_add_layer(p_window, p_layer);
	
	
	//图层：星期
	layertext.font_type = U_ASCII_ARIAL_20;
	layertext.alignment = GAlignTopRight;
	
	grect.origin.x = 24;		grect.origin.y = 56;
	grect.size.h   = 20;	    grect.size.w   = 33;
	layertext.frame = grect;
	p_layer = app_layer_create_text(&layertext);
	g_app_wday_id = app_window_add_layer(p_window, p_layer);
	
	
	//图层：日期
	layertext.alignment = GAlignTopLeft;
	
	grect.origin.x = 69;		grect.origin.y = 28;
	grect.size.h   = 20;	    grect.size.w   = 33;
	layertext.frame = grect;
	p_layer = app_layer_create_text(&layertext);
	g_app_date_id = app_window_add_layer(p_window, p_layer);
	
	
	//图层：底部文字
	layertext.alignment = GAlignCenter;
	layertext.font_type = U_ASCII_ARIAL_14;
	
	grect.origin.x = 0;			grect.origin.y = 107;
	grect.size.h   = 14;	    grect.size.w   = 128;
	layertext.frame = grect;
	p_layer = app_layer_create_text(&layertext);
	app_layer_set_bg_color(p_layer, GColorBlack);
	g_app_str_id = app_window_add_layer(p_window, p_layer);


	app_window_click_subscribe(p_window, ButtonIdBack, button_back_from_watchface);

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
	char str[20];
	
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
		const char wday[7][4]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"}; 
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
	
	
	//底部文字修改
	p_layer = app_window_get_layer_by_id(p_window, g_app_str_id);
	
	get_a_task_title_to_watchface();
	if(dida_task_str[0]!='\0')
		app_layer_set_text_text(p_layer, dida_task_str);
	else
	{
		sprintf(str, "%s", DEFAULT_WORDS_HZY);
		app_layer_set_text_text(p_layer, str);
	}
	
	//完成修改
	app_window_update(p_window);
	is_need_refresh_datetime = false;
}


int main()
{
    //simulator_init();

	g_app_window_id = app_window_stack_push( init_window() );
	
	//首次刷新内容
	is_need_refresh_datetime = true;
	update_contents();
	
	/*注册一个事件通知回调（时间改变）*/
	maibu_service_sys_event_subscribe(event_callback);
	
	/*注册接受数据回调函数（滴答清单）*/
	maibu_comm_register_msg_callback(msg_recv_callback);
	/*注册通讯结果状态回调（滴答清单）*/
	maibu_comm_register_result_callback(msg_result_callback);

    //simulator_wait();
	return 0;
}
