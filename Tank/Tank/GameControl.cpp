#include "stdafx.h"
#include "GameControl.h"

int GameControl::mCurrentStage = 2;	// [1-35]
GameControl::GameControl( Graphics* grap, HDC des_hdc, HDC image_hdc, BoxMarkStruct* bms)
{
	mGraphics = grap;
	mDes_hdc = des_hdc;
	mImage_hdc = image_hdc;
	mCenterImage.Resize( CENTER_WIDTH, CENTER_HEIGHT );
	mCenter_hdc = GetImageHDC(&mCenterImage);
	mBoxMarkStruct = bms;
	Init();
}

GameControl::~GameControl()
{
}

void GameControl::Init()
{
	loadimage( &mBlackBackgroundImage, _T("./res/big/bg_black.gif"		));		// 黑色背景
	loadimage( &mGrayBackgroundImage , _T("./res/big/bg_gray.gif"		));		// 灰色背景
	loadimage( &mStoneImage			 , _T("./res/big/stone.gif"			));		// 12*12的石头
	loadimage( &mForestImage		 , _T("./res/big/forest.gif"		));		// 树林
	loadimage( &mIceImage			 , _T("./res/big/ice.gif"			));		// 冰块
	loadimage( &mRiverImage[0]		 , _T("./res/big/river-0.gif"		));		// 河流
	loadimage( &mRiverImage[1]		 , _T("./res/big/river-1.gif"		));		//
	loadimage( &mWallImage			 , _T("./res/big/wall.gif"			));		// 泥墙
	loadimage( &mCamp				 , _T("./res/big/bird.gif"			));		// 大本营
	loadimage( &mEnemyTankIcoImage	 , _T("./res/big/enemytank-ico.gif"	));		// 敌机图标
	loadimage( &mFlagImage			 , _T("./res/big/flag.gif"			));		// 旗子
	loadimage( &mBlackNumberImage	 , _T("./res/big/black-number.gif"	));		// 0123456789 黑色数字

	mActiveEnemyTankNumber = 0;													// 已经出现在地图上的敌机数量,最多显示6架
	mRemainEnemyTankNumber = 20;												// 剩余未出现的敌机数量
}

// 存储玩家进链表
void GameControl::AddPlayer(int player_num)
{
	for ( int i = 0; i < player_num; i++ )
		PlayerList.push_back( *(new PlayerBase(i)) );	// 后面插入数据
}

/*
* 读取data数据绘制地图,
* 显示敌机数量\玩家生命\关卡\等信息
*/
void GameControl::LoadMap()
{
	// 读取地图文件数据
	FILE* fp = NULL;
	if ( 0 != fopen_s(&fp, "./res/data/map.txt", "rb") )
		throw _T("读取地图数据文件异常.");
	fseek(fp, sizeof(Map) * (mCurrentStage - 1), SEEK_SET );
	fread(&mMap, sizeof(Map), 1, fp);
	fclose(fp);

	int x = 0, y = 0;
	for ( int i = 0; i < 26; i++ )
	{
		for ( int j = 0; j < 26; j++ )
		{
			SignBoxMark( i, j, mMap.buf[i][j] - '0' );		// 标记 26*26 和 52*52 格子
			//printf("%c- ", mMap.buf[i][j] );
		}
		//printf("\n");
	}

	while (StartGame())
	{
		Sleep(40);
	}
}

bool GameControl::StartGame()
{
	// 更新右边面板的数据, 待判断, 因为不需要经常更新 mImage_hdc
	RefreshRightPanel();

	// 更新中心游戏区域: mCenter_hdc
	RefreshCenterPanel();

	// 将中心画布印到主画布 mImage_hdc 上
	BitBlt( mImage_hdc, CENTER_X, CENTER_Y, CENTER_WIDTH, CENTER_HEIGHT, mCenter_hdc, 0, 0, SRCCOPY );
	
	// 整张画布缩放显示 image 到主窗口
	StretchBlt( mDes_hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, mImage_hdc, 0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, SRCCOPY );
	FlushBatchDraw();

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
///////////////////////// 私有函数,本类使用 //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

// 标记 26*26 和 52*52 的格子
void GameControl::SignBoxMark(int i, int j, int sign_val)
{
	mBoxMarkStruct->box_8[i][j] = sign_val;	// 26*26
	int temp_i[4] = { 2 * i, 2 * i + 1, 2 * i, 2 * i + 1 };
	int temp_j[4] = { 2 * j, 2 * j, 2 * j + 1, 2 * j + 1 };

	for ( int i = 0; i < 4; i++ )
		mBoxMarkStruct->box_4[ temp_i[i] ][ temp_j[i] ] = sign_val;
}

void GameControl::RefreshRightPanel()
{
	// 灰色背景
	StretchBlt(mImage_hdc, 0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, GetImageHDC(&mGrayBackgroundImage), 0, 0, 66, 66, SRCCOPY);

	// 显示敌机数量图标
	int x[2] = {233, 241};
	int n, index;
	for ( int i = 0; i < mRemainEnemyTankNumber; i++ )
	{
		n = i / 2;
		index = i % 2;

		TransparentBlt( mImage_hdc, x[index], 19 + n * 8, ENEMY_TANK_ICO_SIZE, ENEMY_TANK_ICO_SIZE, 
			GetImageHDC(&mEnemyTankIcoImage), 0, 0, ENEMY_TANK_ICO_SIZE, ENEMY_TANK_ICO_SIZE, 0xffffff );	// 注意这个图标有黑色部分
	}
	
	// 玩家1P\2P\坦克图标\生命数
	for (itor = PlayerList.begin(); itor != PlayerList.end(); itor++)
	{
		itor->DrawPlayerTankIco(mImage_hdc);		// 坦克图标
	}

	// 旗子
	TransparentBlt(mImage_hdc, 232, 177, FLAG_ICO_SIZE_X, FLAG_ICO_SIZE_Y,
		GetImageHDC(&mFlagImage), 0, 0, FLAG_ICO_SIZE_X, FLAG_ICO_SIZE_Y, 0xffffff);	// 注意图标内有黑色部分

	// 关卡
	if (mCurrentStage < 10)
		TransparentBlt(mImage_hdc, 238, 193, 7, 7, GetImageHDC(&mBlackNumberImage),
			BLACK_NUMBER_SIZE * mCurrentStage, 0, BLACK_NUMBER_SIZE, BLACK_NUMBER_SIZE, 0xffffff);
	else	// 10,11,12 .. 双位数关卡
	{
		TransparentBlt(mImage_hdc, 233, 193, 7, 7, GetImageHDC(&mBlackNumberImage),
			BLACK_NUMBER_SIZE * (mCurrentStage / 10), 0, BLACK_NUMBER_SIZE, BLACK_NUMBER_SIZE, 0xffffff);
		TransparentBlt(mImage_hdc, 241, 193, 7, 7, GetImageHDC(&mBlackNumberImage),
			BLACK_NUMBER_SIZE * (mCurrentStage % 10), 0, BLACK_NUMBER_SIZE, BLACK_NUMBER_SIZE, 0xffffff);
	}
}

// 更新中间游戏区域
void GameControl::RefreshCenterPanel()
{

	BitBlt(mCenter_hdc, 0, 0, CENTER_WIDTH, CENTER_HEIGHT, GetImageHDC(&mBlackBackgroundImage), 0, 0, SRCCOPY);// 中心黑色背景游戏区
																											  
	// 绘制坦克\玩家按键操作
	for (itor = PlayerList.begin(); itor != PlayerList.end(); itor++)
	{
		itor->DrawPlayerTank(mCenter_hdc);		// 坦克
		itor->PlayerControl(mBoxMarkStruct);
	}

	/* 开始根据数据文件绘制地图
	* 划分为 BOX_SIZE x BOX_SIZE 的格子
	* x坐标： j*BOX_SIZE
	* y坐标： i*BOX_SIZE
	*/
	int x = 0, y = 0;
	for (int i = 0; i < 26; i++)
	{
		for (int j = 0; j < 26; j++)
		{
			x = j * BOX_SIZE;// +CENTER_X;
			y = i * BOX_SIZE;// +CENTER_Y;
			switch (mBoxMarkStruct->box_8[i][j])
			{
			case _WALL:
				BitBlt(mCenter_hdc, x, y, BOX_SIZE, BOX_SIZE, GetImageHDC(&mWallImage), 0, 0, SRCCOPY);
				break;
			case _FOREST:
				BitBlt(mCenter_hdc, x, y, BOX_SIZE, BOX_SIZE, GetImageHDC(&mForestImage), 0, 0, SRCCOPY);
				break;
			case _ICE:
				BitBlt(mCenter_hdc, x, y, BOX_SIZE, BOX_SIZE, GetImageHDC(&mIceImage), 0, 0, SRCCOPY);
				break;
			case _RIVER:
				BitBlt(mCenter_hdc, x, y, BOX_SIZE, BOX_SIZE, GetImageHDC(&mRiverImage[0]), 0, 0, SRCCOPY);
				break;
			case _STONE:
				BitBlt(mCenter_hdc, x, y, BOX_SIZE, BOX_SIZE, GetImageHDC(&mStoneImage), 0, 0, SRCCOPY);
				break;
			default:
				break;
			}
		}
	}

	// 大本营
	TransparentBlt(mCenter_hdc, BOX_SIZE * 12, BOX_SIZE * 24, BOX_SIZE * 2, BOX_SIZE * 2,
		GetImageHDC(&mCamp), 0, 0, BOX_SIZE * 2, BOX_SIZE * 2, 0x000000);
}