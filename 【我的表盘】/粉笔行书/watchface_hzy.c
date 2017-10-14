#include "maibu_res.h"
#include "maibu_sdk.h"


//顶层窗口指针
static P_Window pwindow;


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

	//图片资源定义
	//最后两个为分隔符bmp
    uint32_t bmpRes[12] = {RES_BITMAP_0, RES_BITMAP_1, RES_BITMAP_2, RES_BITMAP_3, RES_BITMAP_4, RES_BITMAP_5, RES_BITMAP_6, RES_BITMAP_7, RES_BITMAP_8, RES_BITMAP_9, RES_BITMAP_C, RES_BITMAP_Q};
    const uint8_t bmpH[12] = {25, 34, 32, 30, 35, 38, 35, 31, 33, 35, 11, 16};
    const uint8_t bmpW[12] = {22, 16, 26, 22, 22, 31, 23, 21, 19, 16, 7, 3};


    //确立表盘内容
    uint8_t bmpContentIndex[5];

    struct date_time datetime;
    app_service_get_datetime(&datetime);
    if(datetime.hour > 12)
        datetime.hour-=12;

    bmpContentIndex[0] = datetime.hour / 10;
    bmpContentIndex[1] = datetime.hour % 10;
    bmpContentIndex[3] = datetime.min / 10;
    bmpContentIndex[4] = datetime.min % 10;

    bmpContentIndex[2] = get_bt_status() ? 11:10;

    //字符总宽度
    uint8_t sumWidth = 0,
            i = 0;
    for(; i<5; i++)
        sumWidth += bmpW[ bmpContentIndex[i] ];

    //字符间隔宽度
    uint8_t spaceWidth = (128 - sumWidth) / 6;
    if(spaceWidth > 3) spaceWidth = 3;

    //屏幕左侧宽度
    uint8_t spaceLeft = (128 - sumWidth - spaceWidth * 4) / 2;


	//创建背景图层
	Layer *p_layer;

    GPoint polygonPoints[4] = {{0,0}, {0,128}, {128,128}, {128,0}};
    Polygon polygon = {4, polygonPoints};
    Geometry geometry = {GeometryTypePolygon, FillArea, GColorBlack, (void*)&polygon};
    Geometry *p_geometry_array = &geometry;
    LayerGeometry layergeometry = {1, &p_geometry_array};

    p_layer = app_layer_create_geometry(&layergeometry);
	app_window_add_layer(p_window, p_layer);


	//创建表盘
	GRect grect = {{0,0},{128,128}};
	LayerBitmap layerbitmap = {0};
    GBitmap gbitmap;

    uint8_t nowWidth = spaceLeft + 1;

    for(i=0; i<5; i++) {
        uint8_t bmpI = bmpContentIndex[i];

        res_get_user_bitmap(bmpRes[bmpI], &gbitmap);

        grect.origin.x = nowWidth;
        grect.origin.y = 80 - bmpH[bmpI];
        grect.size.h   = bmpH[bmpI];
        grect.size.w   = bmpW[bmpI];

        layerbitmap.bitmap = gbitmap;
        layerbitmap.frame = grect;
        //layerbitmap.alignment = GAlignCenter;
        p_layer = app_layer_create_bitmap(&layerbitmap);
        app_window_add_layer(p_window, p_layer);

        nowWidth += bmpW[bmpI] + spaceWidth;
    }


	return p_window;
}


//事件回调函数
void event_callback(enum SysEventType type, void *context)
{
	if (type == SysEventTypeTimeChange) {
	    P_Window newWin = init_window();
	    app_window_stack_replace_window(pwindow, newWin);
	    pwindow = newWin;
	}
}


int main()
{
    simulator_init();

    pwindow = init_window();
	app_window_stack_push( pwindow );
	
	/*注册一个事件通知回调（时间改变）*/
	maibu_service_sys_event_subscribe(event_callback);

    simulator_wait();
	return 0;
}
